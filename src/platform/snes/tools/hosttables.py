#!/usr/bin/env python3
"""FX3 CPU host tables: 32-bit file offsets, independently banked payloads.

The 64 KiB *aggregate* staging limit belongs to the GSU-2 implementation,
not FX3. Individual tables still use 16-bit indices in the engine, so each
stays inside one CPU bank. Only SECTOR is mutable (movable-block floors).
The directory lives at CPU $E00000 / file $200000, using FX3's full-bank
FastROM map. Its last MiB is CPU-only; do not map it into GSU ROMBR.
"""

import argparse
import json
from pathlib import Path
import struct

BANK_SIZE = 0x10000
ROM_OFFSET = 0x200000
CPU_ADDRESS = 0xE00000
CAPACITY = 0x200000
HEADER_SIZE = 256
VERSION = 5
TABLES = ("ROOM", "SECTOR", "LINK", "MODEL", "ANIM_BOX", "ANIM_SPEED",
          "ANIM", "TRANS", "CMD", "TRIGGER", "ITEM", "BOX", "OVERLAP",
          "ZONE", "STATIC", "STATIC_FIRST", "AIM", "NAV")
COUNTS = ("items", "models", "anims", "trans", "triggers", "cmds", "boxes",
          "overlaps", "rooms", "sectors", "links", "statics")
# None denotes a self-indexed, variable-length table validated by its packer.
SHAPES = (("rooms", 10), ("sectors", 4), ("links", 8), ("models", 16),
          None, ("anims", 8), ("anims", 14), ("trans", 6),
          ("cmds", 8), ("triggers", 16), ("items", 16), ("boxes", 8),
          ("overlaps", 2), None, ("statics", 20),
          ("rooms", 2), None, None)


def integer(value, name, maximum):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError(f"{name}: expected integer in 0..{maximum}")
    return value


def validate_shape(name, length, counts):
    shape = SHAPES[TABLES.index(name)]
    if shape is not None:
        count, stride = shape
        expected = (counts[count] + (name == "STATIC_FIRST")) * stride
        if length != expected:
            raise ValueError(f"{name}: length {length}, expected {expected}")
    if length > BANK_SIZE:
        raise ValueError(f"{name}: 16-bit table indices require at most 65536 bytes")


def parse_zones(data, boxes):
    """Six indexed arrays; only identical whole-array aliases are allowed."""
    if len(data) < 12:
        raise ValueError("ZONE: truncated six-map directory")
    offsets = struct.unpack_from("<6H", data)
    regions = set()
    for offset in offsets:
        if offset < 12 or offset & 1 or offset + boxes * 2 > len(data):
            raise ValueError("ZONE: invalid map range")
        regions.add(offset)
    end = 12
    for offset in sorted(regions):
        if offset != end:
            raise ValueError("ZONE: overlapping maps or unaccounted bytes")
        end += boxes * 2
    if end != len(data):
        raise ValueError("ZONE: unaccounted map tail")
    return [list(struct.unpack_from(f"<{boxes}H", data, offset)) for offset in offsets]


def parse_frames(data, anims):
    """Union prefix, animation directory, tick maps and shared six-word boxes."""
    prefix = anims * 12
    end = anims * 14
    if len(data) < end or len(data) & 1:
        raise ValueError("ANIM_BOX: truncated/odd frame directory")
    regions = {}  # start -> (length, kind); only whole records may alias
    def region(start, size, kind):
        if start < end or start & 1 or start + size > len(data):
            raise ValueError("ANIM_BOX: invalid frame record range")
        if start in regions and regions[start] != (size, kind):
            raise ValueError("ANIM_BOX: aliased record types")
        regions[start] = (size, kind)
    result = []
    for a in range(anims):
        record, = struct.unpack_from("<H", data, prefix + a * 2)
        if not record:
            result.append([])
            continue
        if record < end or record & 1 or record + 2 > len(data):
            raise ValueError("ANIM_BOX: invalid frame map range")
        ticks, = struct.unpack_from("<H", data, record)
        if not 1 <= ticks <= 255:
            raise ValueError("ANIM_BOX: invalid tick count")
        region(record, 2 + ticks * 2, "map")
        offsets = struct.unpack_from(f"<{ticks}H", data, record + 2)
        for offset in offsets:
            if offset < prefix:
                if offset % 12:
                    raise ValueError("ANIM_BOX: invalid union alias")
            else:
                region(offset, 12, "box")
        result.append([struct.unpack_from("<6h", data, p) for p in offsets])
    cursor = end
    for offset, (size, _) in sorted(regions.items()):
        if offset != cursor:
            raise ValueError("ANIM_BOX: overlapping records or unaccounted bytes")
        cursor += size
    if cursor != len(data):
        raise ValueError("ANIM_BOX: unaccounted frame tail")
    return result


def build(raw, manifest):
    counts = {n: integer(manifest["counts"][n], n, 65535) for n in COUNTS}
    if set(manifest["tables"]) != set(TABLES):
        raise ValueError("host manifest must name all 18 tables exactly once")
    image = bytearray(HEADER_SIZE)
    regions = []
    for index, name in enumerate(TABLES):
        record = manifest["tables"][name]
        start = integer(record["offset"], name + " offset", len(raw))
        length = integer(record["length"], name + " length", len(raw))
        if start + length > len(raw):
            raise ValueError(f"{name}: truncated raw payload")
        validate_shape(name, length, counts)
        if name == "ZONE":
            parse_zones(raw[start:start + length], counts["boxes"])
        if name == "ANIM_BOX":
            parse_frames(raw[start:start + length], counts["anims"])
        if length:
            for a, b in regions:
                if start < b and a < start + length:
                    raise ValueError(f"{name}: overlapping raw tables")
            regions.append((start, start + length))
        offset = (len(image) + 1) & ~1
        if (offset & 0xFFFF) + length > BANK_SIZE:
            offset = (offset + BANK_SIZE - 1) & -BANK_SIZE
        if offset + length > CAPACITY:
            raise ValueError("FX3 host tables exceed the CPU ROM allocation")
        image.extend(bytes(offset - len(image)))
        image.extend(raw[start:start + length])
        struct.pack_into("<IIHH", image, 40 + index * 12,
                         offset, length, int(name == "SECTOR"), 0)
    struct.pack_into("<4sHHIHH12H", image, 0, b"OLH2", VERSION, len(TABLES),
                     len(image), HEADER_SIZE, 0, *(counts[n] for n in COUNTS))
    parse(image)
    return bytes(image)


def parse(image):
    if len(image) < HEADER_SIZE:
        raise ValueError("truncated host directory")
    magic, version, count, size, header, reserved = struct.unpack_from("<4sHHIHH", image)
    if (magic, version, count, header, reserved) != (b"OLH2", VERSION, len(TABLES), HEADER_SIZE, 0):
        raise ValueError("unsupported host directory")
    if not HEADER_SIZE <= size <= CAPACITY or size != len(image):
        raise ValueError("invalid host image length")
    counts = dict(zip(COUNTS, struct.unpack_from("<12H", image, 16)))
    tables = {}
    end = HEADER_SIZE
    for index, name in enumerate(TABLES):
        offset, length, flags, reserved = struct.unpack_from("<IIHH", image, 40 + index * 12)
        if flags != int(name == "SECTOR") or reserved:
            raise ValueError(f"{name}: invalid flags/reserved field")
        if offset < end or offset >= CAPACITY or offset & 1 or offset + length > size:
            raise ValueError(f"{name}: invalid/overlapping table range")
        validate_shape(name, length, counts)
        if name == "ZONE":
            parse_zones(image[offset:offset + length], counts["boxes"])
        if name == "ANIM_BOX":
            parse_frames(image[offset:offset + length], counts["anims"])
        if (offset & 0xFFFF) + length > BANK_SIZE:
            raise ValueError(f"{name}: table crosses a CPU bank")
        tables[name] = {"offset": offset, "length": length, "flags": flags,
                        "address": CPU_ADDRESS + offset}
        end = offset + length
    if end != size:
        raise ValueError("unaccounted host image tail")
    return {"counts": counts, "tables": tables, "size": size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        image = build(args.raw.read_bytes(), json.loads(args.manifest.read_text()))
    except (ValueError, KeyError, TypeError) as exc:
        parser.exit(1, f"host tables: {exc}\n")
    args.output.write_bytes(image)
    sector_bytes = parse(image)["tables"]["SECTOR"]["length"]
    print(f"{args.output}: {len(image)} bytes in bank-aware FX3 ROM, "
          f"{sector_bytes} mutable sector bytes in WRAM")


if __name__ == "__main__":
    main()
