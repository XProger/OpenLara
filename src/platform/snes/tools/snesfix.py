#!/usr/bin/env python3
"""Patch the complement/checksum pair in a SNES ROM's registration header.

wlalink does not fill these in for a hand rolled header, and a Super FX header
has to be hand rolled because .SNESHEADER has no field for the expansion RAM
size at $FFBD.

Algorithm per SNES Development Manual Book I:
  1. preset $FFDC/$FFDD (complement) to $FFFF and $FFDE/$FFDF (checksum) to 0
  2. sum every byte of the ROM, padding non power of two sizes by re-adding
     the trailing part until the total is 2^n
  3. checksum  = low 16 bits of the sum
     complement = checksum XOR $FFFF
"""

import sys

LOROM_HEADER = 0x7FB0
HIROM_HEADER = 0xFFB0


def rom_sum(data: bytes) -> int:
    """Byte sum with the manual's power-of-two padding rule."""
    size = len(data)
    if size & (size - 1) == 0:
        return sum(data) & 0xFFFF

    # Largest power of two that fits, then the remainder repeated until the
    # total reaches the next power of two.
    base = 1 << (size.bit_length() - 1)
    head = sum(data[:base])
    tail_len = size - base
    tail = sum(data[base:])
    reps = base // tail_len if tail_len else 0
    return (head + tail * reps) & 0xFFFF


def patch(path: str, hirom: bool = False) -> None:
    data = bytearray(open(path, "rb").read())
    base = HIROM_HEADER if hirom else LOROM_HEADER

    if len(data) < base + 0x30:
        sys.exit(f"{path}: too small to hold a registration header")

    data[base + 0x2C] = 0xFF
    data[base + 0x2D] = 0xFF
    data[base + 0x2E] = 0x00
    data[base + 0x2F] = 0x00

    checksum = rom_sum(bytes(data))
    complement = checksum ^ 0xFFFF

    data[base + 0x2C] = complement & 0xFF
    data[base + 0x2D] = complement >> 8
    data[base + 0x2E] = checksum & 0xFF
    data[base + 0x2F] = checksum >> 8

    open(path, "wb").write(data)

    title = bytes(data[base + 0x10:base + 0x25]).decode("ascii", "replace")
    print(f"{path}: {len(data) // 1024} KB  \"{title.strip()}\"  "
          f"checksum ${checksum:04X}  complement ${complement:04X}")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) != 1:
        sys.exit("usage: snesfix.py [--hirom] rom.sfc")
    patch(args[0], hirom="--hirom" in sys.argv[1:])
