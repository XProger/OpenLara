#!/usr/bin/env python3
"""Emit the wla include that drops the level and the texture pages into ROM.

GSU assets use linear 64 KB banks, two wla banks per bank. The current
geometry/texture allocation stops before the host code at file $1E0000;
that is a layout reservation, not FX3's 3 MB GSU addressing limit.
Three things need placing:

  LEVEL.BIN   the room table, every room's geometry and its portals. The table
              is at offset 0 of the first bank, which is what the GSU code
              hard-codes; rooms are placed inside banks by the packer so none
              straddles a boundary.
  TEXnn.BIN   one 256x256 page per GSU bank, so that a texel address is exactly
              (v << 8) | u -- one MERGE in the inner loop. Half-pages would
              need an offset per pixel and would split TR tiles down the middle.
  HOST.BIN    FX3 only: independently banked CPU tables at file $200000,
              in the full-bank CPU FastROM window $E0-$FF. GSU-2 retains
              its aggregate host blob inside LEVEL.BIN and a 2 MB image.
  SOUND.BIN   the sound CPU's image -- directory, pitch words and BRR
              samples -- placed after the textures. Only the 65816 reads it,
              once, at boot, through the IPL protocol; the GSU never does.

Also emits the bank bases, so the GSU code and the packed data agree without
either hard-coding the other's layout.
"""

import os
import json
import struct
import sys
import hosttables

FIRST_BANK = 6          # wla ROM bank of the first placed blob; 0-5 are code
CODE_BANK = 60          # $3C boot/oracle ROM, followed by resident images $3D-$3F
# FX3 keeps its code in wla 0-9 ($84-$89; the Makefile moves GSU-2's $3C-$3F
# numbering down there), so its data starts at GSU $45 and may run to the
# host tables -- the GSU's window, unbroken, is what a second level needs.
FX3_FIRST_BANK = 10
WLA_BANK = 32 * 1024
GSU_BANK = 64 * 1024
PAGE_SIZE = GSU_BANK


def wla_banks(size):
    """Whole GSU banks, as a count of wla banks -- always even."""
    return ((size + GSU_BANK - 1) // GSU_BANK) * 2


def check_bank_space(level_size, page_count, sound_size=0, fx3=False):
    first = FX3_FIRST_BANK if fx3 else FIRST_BANK
    banks = first + wla_banks(level_size) + page_count * 2
    if sound_size:
        banks += (sound_size + WLA_BANK - 1) // WLA_BANK
    if fx3 and banks > hosttables.ROM_OFFSET // WLA_BANK:
        sys.exit("level and textures overlap the FX3 host tables at file $200000")
    if not fx3 and banks > CODE_BANK:
        sys.exit("level and textures overlap the host code in ROM bank $3C")


SOUND_MAX = 0xFF00 - 0x0400     # the APU image, from $0400 to the echo bytes


def sound_image(data_dir):
    """SOUND.BIN's size, checked against the sound CPU's RAM."""
    path = os.path.join(data_dir, "SOUND.BIN")
    if not os.path.isfile(path):
        sys.exit("SOUND.BIN not found; run the packer first")
    size = os.path.getsize(path)
    if not 0 < size <= SOUND_MAX:
        sys.exit(f"SOUND.BIN is {size} bytes, not within 1..{SOUND_MAX}")
    return size


# FX3: the level table's shape. Every word of an entry is one of OlLevelDesc's
# (src/level.h); main.asm reads the ones it needs by these offsets.
LEVEL_ENTRY = 64
LEVEL_FIELDS = {"OLD_GSU_ENTRY": 4, "OLD_FONT": 20, "OLD_FONT_SIZE": 24,
                "OLD_PALETTE": 26, "OLD_HUD": 30, "OLD_SNDTAB": 34,
                "OLD_SOUND": 38, "OLD_SOUND_SIZE": 42, "OLD_GUN": 54, "OLD_GUN_SIZE": 58}
FONT_LEVEL = 56                 # FONT.BIN's LevelID byte (inventory_text.c)
FX3_GSU_ENTRY = 0x0100          # frame.gsu: the page-zero idle probe below it
SMALL_TABLES = ("CAMERA", "INV", "FONT", "PALETTE", "HUD", "SNDTAB", "FX", "GUNSND")
TITLE_SIZE = 64 * 2 + 216 * 144 # TITLE.BIN: its 64 colours and the 216 x 144 picture


def incbin(lines, label, path, file_offset, size):
    """size bytes of path at ROM file offset file_offset, split at wla banks."""
    done = part = 0
    while done < size:
        offset = file_offset + done
        n = min(size - done, WLA_BANK - offset % WLA_BANK)
        lines += [f".BANK {offset // WLA_BANK}", f".ORG {offset % WLA_BANK}",
                  f'.SECTION "{label}{part}" FORCE',
                  f'    .INCBIN "{path}" SKIP {done} READ {n}', ".ENDS", ""]
        done += n
        part += 1


def place_cpu_level(lines, data_dir, inc, base, tag, title=False):
    """One FX3 level's CPU-only data from file offset base: the host tables
    bank-aligned (olHostBind binds them by their bank), the small tables in
    the host image's tail -- none crossing a bank, as their readers bind a
    bank and a 16-bit offset -- then the sphere data and the APU image, each
    starting a bank. The cartridge's first level also carries the title
    screen (title=True), with the small tables: main.asm copies it with one
    MVN, so it too stays within a bank. Returns {name: (file offset, size)}
    and the next bank."""
    def size_of(name):
        path = os.path.join(data_dir, f"{name}.BIN")
        return os.path.getsize(path) if os.path.exists(path) else 0
    if base & 0xFFFF:
        sys.exit("FX3 host tables must start a CPU bank")
    with open(os.path.join(data_dir, "HOST.BIN"), "rb") as f:
        try:
            host = hosttables.parse(f.read())
        except ValueError as exc:
            sys.exit(f"invalid FX3 host tables: {exc}")
    at = {"HOST": (base, host["size"])}
    incbin(lines, f"host{tag}_", inc + "HOST.BIN", base, host["size"])
    cursor = (base + host["size"] + 1) & ~1
    for name in SMALL_TABLES + (("TITLE",) if title else ()):
        size = size_of(name)
        if name == "TITLE" and size not in (0, TITLE_SIZE):
            sys.exit(f"TITLE.BIN is {size} bytes, not {TITLE_SIZE}")
        if size > 0x10000:
            sys.exit(f"{name}.BIN ({size} bytes) is larger than a CPU bank")
        if size and (cursor & 0xFFFF) + size > 0x10000:
            cursor = (cursor + 0xFFFF) & ~0xFFFF
        at[name] = (cursor, size)
        if size:
            incbin(lines, f"{name.lower()}{tag}_", inc + f"{name}.BIN", cursor, size)
            cursor = (cursor + size + 1) & ~1
    cursor = (cursor + 0xFFFF) & ~0xFFFF
    at.setdefault("TITLE", (0, 0))
    sphere = size_of("SPHERE")
    at["SPHERE"] = (cursor if sphere else 0, sphere)
    if sphere:
        incbin(lines, f"sphere{tag}_", inc + "SPHERE.BIN", cursor, sphere)
        cursor = (cursor + sphere + 0xFFFF) & ~0xFFFF
    sound = sound_image(data_dir)
    at["SOUND"] = (cursor, sound)
    incbin(lines, f"sound{tag}_", inc + "SOUND.BIN", cursor, sound)
    cursor = (cursor + sound + 0xFFFF) & ~0xFFFF
    if cursor > 0x400000:
        sys.exit(f"level {tag}: CPU-only data runs past the 4 MiB image")
    return at, host, cursor


def write_title(f, offset, size):
    """hostbank.inc: where the title screen is, as main.asm's MVN wants it
    (a bank operand and an offset), or a zero size for none."""
    cpu = 0xC00000 + offset if size else 0
    f.write(f".DEFINE OL_TITLE_ADDR ${cpu:06X}\n")
    f.write(f".DEFINE OL_TITLE_BANK ${cpu >> 16:02X}\n")
    f.write(f".DEFINE OL_TITLE_OFFSET ${cpu & 0xFFFF:04X}\n")
    f.write(f".DEFINE OL_TITLE_SIZE {size}\n")


def write_title_c(f, offset, size):
    """olhost.h: the title screen for C (settings.c's gamma over its colours)."""
    f.write(f"#define OL_TITLE_ADDR 0x{0xC00000 + offset if size else 0:06X}LL\n")
    f.write(f"#define OL_TITLE_SIZE {size}\n")


def level_entry(at, level_gsu, gsu_entry, data_dir, name):
    """OlLevelDesc's 32 words for one level."""
    def far(key):
        offset, size = at[key]
        cpu = 0xC00000 + offset if size else 0
        return [cpu & 0xFFFF, cpu >> 16, size]
    font = os.path.join(data_dir, "FONT.BIN")
    level_id = 0
    if os.path.exists(font) and os.path.getsize(font) > FONT_LEVEL:
        with open(font, "rb") as f:
            level_id = f.read()[FONT_LEVEL]
    words = [(0xC00000 + at["HOST"][0]) >> 16, 0x80 + level_gsu, gsu_entry, level_id]
    for key in ("CAMERA", "INV", "FONT"):
        words += far(key)
    for key in ("PALETTE", "HUD", "SNDTAB"):
        words += far(key)[:2]
    words += far("SOUND") + [far("SPHERE")[1], 0]
    words += far("FX") + far("GUNSND") + [0] * 2
    assert len(words) * 2 == LEVEL_ENTRY
    return ([f"    ; {name}: LevelID {level_id}"] +
            ["    .DW " + ", ".join(f"${w:04X}" for w in words[i:i + 8]) for i in (0, 8, 16, 24)])


def check_host_blob(blob_offset, blob_size):
    """Staging cannot wrap R14 past the room directory's source ROM bank."""
    if not 0 < blob_size <= 65534 or blob_offset + blob_size > GSU_BANK:
        sys.exit("host level blob must fit in one 64 KB ROM bank")


def texture_pages(data_dir):
    """Read the selected level's inventory, never stale neighbouring files."""
    with open(os.path.join(data_dir, "TEXTURES.json")) as f:
        count = json.load(f)["pages"]
    if type(count) is not int or not 0 < count <= 32:
        sys.exit(f"invalid texture page count: {count!r}")
    pages = [f"TEX{i:02d}.BIN" for i in range(count)]
    for name in pages:
        path = os.path.join(data_dir, name)
        if not os.path.isfile(path) or os.path.getsize(path) != PAGE_SIZE:
            sys.exit(f"{name}: missing or not {PAGE_SIZE} bytes; repack the level")
    return pages


def write_texbank(path, level_gsu, tex_gsu, page_count, weapon, enabled,
                  blob_offset=0, blob_size=0, sound_size=0, sound_halves=()):
    """One level's layout for its GSU program (and GSU-2's host)."""
    with open(path, "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write(f".DEFINE LEVEL_BANK_BASE ${level_gsu:02X}\n")
        f.write(f".DEFINE TEXTURE_BANK_BASE ${tex_gsu:02X}\n")
        f.write(f".DEFINE TEXTURE_PAGES {page_count}\n")
        f.write(f".DEFINE HOST_BLOB_BANK ${level_gsu:02X}\n")
        f.write(f".DEFINE HOST_BLOB_OFFSET ${blob_offset:04X}\n")
        f.write(f".DEFINE HOST_BLOB_SIZE ${blob_size:04X}\n")
        f.write(f".DEFINE PISTOL_AVAILABLE {int(enabled)}\n")
        f.write(f".DEFINE SOUND_IMAGE_SIZE {sound_size}\n")
        for i in range(2):
            size = sound_halves[i] if i < len(sound_halves) else 0
            f.write(f".DEFINE SOUND_IMAGE_SIZE_{i} {size}\n")
        animations = weapon["animations"] if enabled else [{"offset": 0, "last": 0}] * 4
        meshes = weapon["meshes"] if enabled else [{"offset": -1, "radius": 0}] * 8
        for i, anim in enumerate(animations):
            f.write(f".DEFINE PISTOL_ANIM_{i}_ADDR ${anim['offset'] & 0xFFFF:04X}\n")
            f.write(f".DEFINE PISTOL_ANIM_{i}_BANK ${level_gsu + (anim['offset'] >> 16):02X}\n")
            f.write(f".DEFINE PISTOL_ANIM_{i}_LAST {anim['last']}\n")
        for i, mesh in enumerate(meshes):
            offset = mesh["offset"]
            f.write(f".DEFINE PISTOL_MESH_{i}_ADDR ${max(0, offset) & 0xFFFF:04X}\n")
            f.write(f".DEFINE PISTOL_MESH_{i}_BANK ${(offset >> 16) if offset >= 0 else 255:02X}\n")
            f.write(f".DEFINE PISTOL_MESH_{i}_RADIUS {mesh['radius']}\n")
        # drawFlash's mesh (FX3's DrawMeshFlash), bank 255 for none
        flash = weapon.get("flash", {"offset": -1}) if enabled else {"offset": -1}
        offset = flash["offset"]
        f.write(f".DEFINE FLASH_MESH_ADDR ${max(0, offset) & 0xFFFF:04X}\n")
        f.write(f".DEFINE FLASH_MESH_BANK ${(offset >> 16) if offset >= 0 else 255:02X}\n")
        # FX3's other guns (out_SNES.h writeWeaponKit): the swap table's
        # meshes, 1-based, and the shotgun's three animations
        kit = weapon.get("kit") if enabled else None
        f.write(f".DEFINE WEAPON_KIT {int(kit is not None)}\n")
        if kit:
            for i, mesh in enumerate(kit["meshes"], 1):
                f.write(f".DEFINE KIT_MESH_{i}_ADDR ${mesh['offset'] & 0xFFFF:04X}\n")
                f.write(f".DEFINE KIT_MESH_{i}_BANK ${mesh['offset'] >> 16:02X}\n")
                f.write(f".DEFINE KIT_MESH_{i}_RADIUS {mesh['radius']}\n")
            for i, anim in enumerate(kit["shotgun"]):
                f.write(f".DEFINE SHOTGUN_ANIM_{i}_ADDR ${anim['offset'] & 0xFFFF:04X}\n")
                f.write(f".DEFINE SHOTGUN_ANIM_{i}_BANK ${level_gsu + (anim['offset'] >> 16):02X}\n")
                f.write(f".DEFINE SHOTGUN_ANIM_{i}_LAST {anim['last']}\n")


def write_olweapon(path, weapon, enabled, note):
    """The CPU program's view of the weapon animations: their lengths, and
    FX3's shotgun -- its lengths and its torso frames (drawLaraNodes)."""
    with open(path, "w") as f:
        f.write(f"/* {note} */\n")
        f.write(f"#define PISTOL_AVAILABLE {int(enabled)}\n")
        for i, name in enumerate(("AIM", "PICK", "DRAW", "FIRE")):
            last = weapon["animations"][i]["last"] if enabled else 0
            f.write(f"#define PISTOL_{name}_LAST {last}\n")
        kit = weapon.get("kit") if enabled else None
        f.write(f"#define WEAPON_KIT {int(kit is not None)}\n")
        if kit:
            first = 0
            for name, anim in zip(("AIM", "DRAW", "FIRE"), kit["shotgun"]):
                f.write(f"#define SHOTGUN_{name}_LAST {anim['last']}\n")
                f.write(f"#define SHOTGUN_{name}_TORSO {first}\n")
                first += anim["last"] + 1
            f.write("#define SHOTGUN_TORSO_FRAMES { \\\n")
            torso = kit["torso"]
            for i in range(0, len(torso), 12):
                f.write("    " + ", ".join(str(v) for v in torso[i:i + 12]) + ", \\\n")
            f.write("}\n")


def main():
    if len(sys.argv) < 3:
        sys.exit("usage: mkbanks.py DATA OUTPUT [--fx3]")
    data_dir, out = sys.argv[1], sys.argv[2]
    fx3 = sys.argv[3:] == ["--fx3"]
    if sys.argv[3:] and not fx3:
        sys.exit("usage: mkbanks.py DATA OUTPUT [--fx3]")

    level = os.path.join(data_dir, "LEVEL.BIN")
    if not os.path.exists(level):
        sys.exit("LEVEL.BIN not found; run the packer first")
    level_size = os.path.getsize(level)
    with open(level, "rb") as f:
        header = f.read(4)
        if len(header) != 4:
            sys.exit("truncated LEVEL.BIN header")
        blob_offset = struct.unpack_from("<H", header, 2)[0]
        f.seek(blob_offset)
        blob_size = struct.unpack("<H", f.read(2))[0] if not fx3 else 0
    if not fx3:
        check_host_blob(blob_offset, blob_size)
    elif blob_offset:
        sys.exit("FX3 LEVEL.BIN still contains a legacy host blob; repack with --fx3")
    if level_size % GSU_BANK:
        sys.exit(f"LEVEL.BIN is {level_size} bytes, not a whole number of "
                 f"{GSU_BANK}-byte GSU banks")

    with open(os.path.join(data_dir, "WEAPON.json")) as f:
        weapon = json.load(f)
    enabled = len(weapon["animations"]) == 4 and len(weapon["meshes"]) == 8

    pages = texture_pages(data_dir)
    sound_size = sound_image(data_dir)
    check_bank_space(level_size, len(pages), 0 if fx3 else sound_size, fx3)
    first_bank = FX3_FIRST_BANK if fx3 else FIRST_BANK

    lines = ["; generated by tools/mkbanks.py -- do not edit", ""]

    # --- level ---------------------------------------------------------------
    bank = first_bank
    level_gsu = 0x40 + bank // 2
    lines += [f"; LEVEL.BIN -> GSU banks ${level_gsu:02X}.."
              f"${level_gsu + level_size // GSU_BANK - 1:02X}"]
    for i in range(level_size // WLA_BANK):
        lines += [
            f".BANK {bank + i}",
            ".ORG 0",
            f'.SECTION "level{i}" FORCE',
            f'    .INCBIN "LEVEL.BIN" SKIP {i * WLA_BANK} READ {WLA_BANK}',
            ".ENDS",
            "",
        ]
    bank += wla_banks(level_size)

    # --- textures ------------------------------------------------------------
    tex_gsu = 0x40 + bank // 2
    for i, name in enumerate(pages):
        b = bank + i * 2
        gsu = 0x40 + b // 2
        size = os.path.getsize(os.path.join(data_dir, name))
        if size != PAGE_SIZE:
            sys.exit(f"{name}: expected {PAGE_SIZE} bytes, got {size}")
        lines += [
            f"; page {i} -> GSU bank ${gsu:02X}",
            f".BANK {b}",
            ".ORG 0",
            f'.SECTION "tex{i}a" FORCE',
            f'    .INCBIN "{name}" READ {WLA_BANK}',
            ".ENDS",
            f".BANK {b + 1}",
            ".ORG 0",
            f'.SECTION "tex{i}b" FORCE',
            f'    .INCBIN "{name}" SKIP {WLA_BANK} READ {WLA_BANK}',
            ".ENDS",
            "",
        ]
    last_bank = bank + len(pages) * 2

    # --- sound ---------------------------------------------------------------
    # Two halves at most, each a LoROM bank the CPU reads at boot with a long
    # address; main.asm sends them to the sound CPU as consecutive blocks.
    # FX3's goes with the level's other CPU-only data, whole (below).
    sound_bank = last_bank
    sound_halves = []
    for i in range(0 if fx3 else (sound_size + WLA_BANK - 1) // WLA_BANK):
        size = min(WLA_BANK, sound_size - i * WLA_BANK)
        sound_halves.append(size)
        lines += [
            f"; SOUND.BIN half {i}, {size} bytes",
            f".BANK {sound_bank + i}",
            ".ORG 0",
            f'.SECTION "sound{i}" FORCE',
            f"SoundImage{i}:",
            f'    .INCBIN "SOUND.BIN" SKIP {i * WLA_BANK} READ {size}',
            ".ENDS",
            "",
        ]
    last_bank = sound_bank + len(sound_halves)

    sphere_file = sphere_size = 0
    camera_file = camera_size = 0
    font_file = font_size = 0
    title_file = title_size = 0
    host = None
    table = []
    if fx3:
        at, host, _ = place_cpu_level(lines, data_dir, "", hosttables.ROM_OFFSET, "", title=True)
        sphere_file, sphere_size = at["SPHERE"]
        camera_file, camera_size = at["CAMERA"]
        font_file, font_size = at["FONT"]
        title_file, title_size = at["TITLE"]
        sound_halves = [sound_size]
        table = level_entry(at, level_gsu, FX3_GSU_ENTRY, data_dir,
                            os.path.basename(os.path.normpath(data_dir)))

    with open(out, "w") as f:
        f.write("\n".join(lines) + "\n")

    # Defines only, so the GSU build can agree on the layout without pulling
    # the INCBINs (which belong to the 65816 object) in with them.
    write_texbank(os.path.join(os.path.dirname(out), "texbank.inc"), level_gsu, tex_gsu,
                  len(pages), weapon, enabled, blob_offset, blob_size, sound_size, sound_halves)
    write_olweapon(os.path.join(os.path.dirname(out), "olweapon.h"), weapon, enabled,
                   "Generated from the level's weapon animations.")

    with open(os.path.join(os.path.dirname(out), "hostbank.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write(f".DEFINE HOST_DIRECTORY_ADDR ${0xC00000 + hosttables.ROM_OFFSET:06X}\n")
        f.write(f".DEFINE HOST_DIRECTORY_SIZE {host['size'] if fx3 else 0}\n")
        f.write(f".DEFINE HOST_SECTOR_SIZE {host['tables']['SECTOR']['length'] if fx3 else 0}\n")
        f.write(f".DEFINE OL_SPHERE_BANK ${(0xC00000 + sphere_file) >> 16 if sphere_size else 0:02X}\n")
        f.write(f".DEFINE OL_SPHERE_SIZE {sphere_size if fx3 else 0}\n")
        f.write(f".DEFINE OL_CAMERA_BANK ${(0xC00000 + camera_file) >> 16 if camera_size else 0:02X}\n")
        f.write(f".DEFINE OL_CAMERA_SIZE {camera_size if fx3 else 0}\n")
        f.write(f".DEFINE OL_FONT_BANK ${(0xC00000 + font_file) >> 16 if font_size else 0:02X}\n")
        f.write(f".DEFINE OL_FONT_SIZE {font_size if fx3 else 0}\n")
        write_title(f, title_file, title_size)
    with open(os.path.join(os.path.dirname(out), "olhost.h"), "w") as f:
        f.write("/* Generated FX3 host directory offsets. Where a level's data is\n"
                "   is olLevel's (src/level.h), at run time. */\n")
        for i, name in enumerate(hosttables.COUNTS):
            f.write(f"#define OL_HOST_{name.upper()} {16 + i * 2}\n")
        write_title_c(f, title_file, title_size)
    with open(os.path.join(os.path.dirname(out), "levels.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write(f".DEFINE LEVEL_COUNT {1 if fx3 else 0}\n")
        f.write(f".DEFINE LEVEL_ENTRY {LEVEL_ENTRY}\n")
        for name, offset in LEVEL_FIELDS.items():
            f.write(f".DEFINE {name} {offset}\n")
        sndtab = os.path.join(data_dir, "SNDTAB.BIN")
        f.write(f".DEFINE SNDTAB_SIZE {os.path.getsize(sndtab) if os.path.exists(sndtab) else 0}\n")
    with open(os.path.join(os.path.dirname(out), "leveltable.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write("\n".join(table) + "\n")

    where = (f"host tables and APU image ({sound_size} bytes) from CPU "
             f"${0xC00000 + hosttables.ROM_OFFSET:06X}" if fx3 else
             f"sound {sound_size} bytes in wla bank {sound_bank}")
    print(f"{out}: level {level_size // 1024} KB at GSU ${level_gsu:02X}, "
          f"{len(pages)} texture pages at GSU ${tex_gsu:02X}, {where}, "
          f"wla banks {first_bank}..{last_bank - 1}")


# --- several levels in one FX3 cartridge ---------------------------------------

FX_END_BANK = 96                # wla bank past the GSU's 3 MiB window ($6F)
CPU_ONLY = 0x300000             # file offset of CPU $F0, which the GSU cannot see
GSU_ENTRIES = (FX3_GSU_ENTRY, 0x8000 + FX3_GSU_ENTRY)   # $41:0000 and $41:8000


def main_multi(build, specs):
    """mkbanks.py --fx3-multi BUILD NAME=PREFIX...: each level's packed data in
    BUILD/PREFIX (INCBIN paths are relative to BUILD). The GSU's window holds
    every level's geometry and texture pages, one level after the other from
    GSU $45; the CPU-only last MiB holds each level's tables and APU image.
    The levels share one CPU program, so what it fixes at assembly must agree
    across them: the pistols' animation lengths, and the one level whose
    swinging blades read sphere data (trap_fx3.asm binds its bank)."""
    if not 2 <= len(specs) <= len(GSU_ENTRIES):
        sys.exit(f"a multi-level cartridge carries 2..{len(GSU_ENTRIES)} levels: one GSU program each, "
                 "at $41:0000 and $41:8000")
    levels = []
    for spec in specs:
        name, _, prefix = spec.partition("=")
        if not name or not prefix:
            sys.exit(f"bad level spec {spec!r}: NAME=PREFIX")
        prefix = prefix.rstrip("/") + "/"
        levels.append((name, prefix, os.path.join(build, prefix)))

    lines = ["; generated by tools/mkbanks.py -- do not edit", ""]
    bank = FX3_FIRST_BANK
    layout = []
    for index, (name, prefix, data_dir) in enumerate(levels):
        level = os.path.join(data_dir, "LEVEL.BIN")
        if not os.path.exists(level):
            sys.exit(f"{name}: LEVEL.BIN not found; run the packer first")
        level_size = os.path.getsize(level)
        if level_size % GSU_BANK:
            sys.exit(f"{name}: LEVEL.BIN is not a whole number of GSU banks")
        with open(level, "rb") as f:
            if struct.unpack_from("<H", f.read(4), 2)[0]:
                sys.exit(f"{name}: LEVEL.BIN still contains a legacy host blob; repack with --fx3")
        with open(os.path.join(data_dir, "WEAPON.json")) as f:
            weapon = json.load(f)
        enabled = len(weapon["animations"]) == 4 and len(weapon["meshes"]) == 8
        pages = texture_pages(data_dir)
        level_gsu = 0x40 + bank // 2
        lines += [f"; {name}: LEVEL.BIN -> GSU banks ${level_gsu:02X}.."
                  f"${level_gsu + level_size // GSU_BANK - 1:02X}"]
        for i in range(level_size // WLA_BANK):
            lines += [f".BANK {bank + i}", ".ORG 0", f'.SECTION "level{index}_{i}" FORCE',
                      f'    .INCBIN "{prefix}LEVEL.BIN" SKIP {i * WLA_BANK} READ {WLA_BANK}',
                      ".ENDS", ""]
        bank += wla_banks(level_size)
        tex_gsu = 0x40 + bank // 2
        for i, page in enumerate(pages):
            b = bank + i * 2
            lines += [f"; {name}: page {i} -> GSU bank ${0x40 + b // 2:02X}",
                      f".BANK {b}", ".ORG 0", f'.SECTION "tex{index}_{i}a" FORCE',
                      f'    .INCBIN "{prefix}{page}" READ {WLA_BANK}', ".ENDS",
                      f".BANK {b + 1}", ".ORG 0", f'.SECTION "tex{index}_{i}b" FORCE',
                      f'    .INCBIN "{prefix}{page}" SKIP {WLA_BANK} READ {WLA_BANK}', ".ENDS", ""]
        bank += len(pages) * 2
        if bank > FX_END_BANK:
            sys.exit(f"{name}: the levels' geometry and textures overrun the GSU's 3 MiB window "
                     f"(wla bank {bank} of {FX_END_BANK})")
        layout.append(dict(name=name, prefix=prefix, data=data_dir, level_gsu=level_gsu,
                           tex_gsu=tex_gsu, pages=len(pages), weapon=weapon, enabled=enabled))

    cpu = max(CPU_ONLY, (bank * WLA_BANK + 0xFFFF) & ~0xFFFF)
    table, spheres, sector_max = [], [], 0
    for index, level in enumerate(layout):
        at, host, cpu = place_cpu_level(lines, level["data"], level["prefix"], cpu, str(index),
                                        title=index == 0)
        level["at"] = at
        sector_max = max(sector_max, host["tables"]["SECTOR"]["length"])
        if at["SPHERE"][1]:
            spheres.append((index, at["SPHERE"]))
        table += level_entry(at, level["level_gsu"], GSU_ENTRIES[index], level["data"], level["name"])
    if len(spheres) > 1:
        sys.exit("more than one level carries sphere data; trap_fx3.asm binds one bank at assembly")

    with open(os.path.join(build, "textures.inc"), "w") as f:
        f.write("\n".join(lines) + "\n")
    lasts = {tuple(a["last"] for a in level["weapon"]["animations"]) if level["enabled"] else None
             for level in layout}
    if len(lasts) != 1:
        sys.exit("the levels' pistol animations differ in length; the CPU program fixes them")
    kits = {json.dumps([[a["last"] for a in level["weapon"]["kit"]["shotgun"]],
                        level["weapon"]["kit"]["torso"]]) if level["enabled"] and "kit" in level["weapon"]
            else None for level in layout}
    if len(kits) != 1:
        sys.exit("the levels' shotgun animations differ; the CPU program fixes them")
    first = layout[0]
    write_texbank(os.path.join(build, "texbank.inc"), first["level_gsu"], first["tex_gsu"],
                  first["pages"], first["weapon"], first["enabled"])
    with open(os.path.join(build, "config.inc")) as f:
        config = f.read()
    for index, level in enumerate(layout[1:], 1):
        gsu_dir = os.path.join(build, f"gsu_{index}")
        os.makedirs(gsu_dir, exist_ok=True)
        write_texbank(os.path.join(gsu_dir, "texbank.inc"), level["level_gsu"], level["tex_gsu"],
                      level["pages"], level["weapon"], level["enabled"])
        with open(os.path.join(gsu_dir, "config.inc"), "w") as f:
            f.write(config + f".DEFINE GSU_ORIGIN ${GSU_ENTRIES[index] - FX3_GSU_ENTRY:04X}\n")
    write_olweapon(os.path.join(build, "olweapon.h"), first["weapon"], first["enabled"],
                   "Generated from the levels' weapon animations (all alike).")
    sphere_file, sphere_size = spheres[0][1] if spheres else (0, 0)
    with open(os.path.join(build, "hostbank.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write(f".DEFINE HOST_DIRECTORY_ADDR ${0xC00000 + first['at']['HOST'][0]:06X}\n")
        f.write(f".DEFINE HOST_DIRECTORY_SIZE {first['at']['HOST'][1]}\n")
        f.write(f".DEFINE HOST_SECTOR_SIZE {sector_max}\n")
        f.write(f".DEFINE OL_SPHERE_BANK ${(0xC00000 + sphere_file) >> 16 if sphere_size else 0:02X}\n")
        f.write(f".DEFINE OL_SPHERE_SIZE {sphere_size}\n")
        for key in ("CAMERA", "FONT"):
            offset, size = first["at"][key]
            f.write(f".DEFINE OL_{key}_BANK ${(0xC00000 + offset) >> 16 if size else 0:02X}\n")
            f.write(f".DEFINE OL_{key}_SIZE {size}\n")
        write_title(f, *first["at"]["TITLE"])
    with open(os.path.join(build, "olhost.h"), "w") as f:
        f.write("/* Generated FX3 host directory offsets. Where a level's data is\n"
                "   is olLevel's (src/level.h), at run time. */\n")
        for i, name in enumerate(hosttables.COUNTS):
            f.write(f"#define OL_HOST_{name.upper()} {16 + i * 2}\n")
        write_title_c(f, *first["at"]["TITLE"])
    sndtabs = {os.path.getsize(os.path.join(level["data"], "SNDTAB.BIN")) for level in layout}
    if len(sndtabs) != 1:
        sys.exit("the levels' SNDTAB.BIN sizes differ")
    with open(os.path.join(build, "levels.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write(f".DEFINE LEVEL_COUNT {len(layout)}\n")
        f.write(f".DEFINE LEVEL_ENTRY {LEVEL_ENTRY}\n")
        for name, offset in LEVEL_FIELDS.items():
            f.write(f".DEFINE {name} {offset}\n")
        f.write(f".DEFINE SNDTAB_SIZE {sndtabs.pop()}\n")
    with open(os.path.join(build, "leveltable.inc"), "w") as f:
        f.write("; generated by tools/mkbanks.py -- do not edit\n")
        f.write("\n".join(table) + "\n")
    for index, level in enumerate(layout):
        print(f"{level['name']}: level at GSU ${level['level_gsu']:02X}, {level['pages']} texture pages at "
              f"GSU ${level['tex_gsu']:02X}, host tables at CPU ${0xC00000 + level['at']['HOST'][0]:06X}, "
              f"GSU program at $41:{GSU_ENTRIES[index]:04X}")
    print(f"GSU window: wla banks {FX3_FIRST_BANK}..{bank - 1} of {FX_END_BANK}; "
          f"CPU-only data to file ${cpu:06X} of $400000")


if __name__ == "__main__":
    if sys.argv[1:2] == ["--fx3-multi"]:
        if len(sys.argv) < 4:
            sys.exit("usage: mkbanks.py --fx3-multi BUILD NAME=PREFIX...")
        main_multi(sys.argv[2], sys.argv[3:])
    else:
        main()
