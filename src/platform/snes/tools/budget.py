#!/usr/bin/env python3
"""Report -- and where it is a hard limit, enforce -- the WRAM budgets.

FX3 is different: simultaneous ROM/SRAM access permits every C/native
section in FastROM. Its ROM banks are reported and validated separately;
the GSU-2 resident-code regions must not constrain that variant. FX3 Nav
and the room cache share a separate BSS allocation in $7E:5800-$FFFF (slot 9).

On GSU-2, runtime host code lives in WRAM, copied there at boot, and the
linker only knows about the ROM banks the images are stored in: a slot is
32 KB wide whether or not the WRAM behind it is free. So the numbers that
matter are read back out of the symbol file after the link:

  host asm     $7E:0110-$05FF   HostCodeEnd must stay below $0600; the fifth
                                C image starts there
  C image      $7E:0600-$1BFF   ROM bank $3D; the stack has $1C00-$1FFF
  C image      $7F:0000-$7FFF   ROM bank 4
  C image      $7F:8000-$FFFF   ROM bank 5
  C image      $7E:8000-$FFFF   ROM bank $3F
  C image      $7E:5800-$7FFF   ROM bank $3E
  C .bss       $7E:2000-$57FF   SLOT 2 in hdr.asm, which wlalink enforces

The three 32 KB images are enforced by wlalink -- a unit that does not fit
its bank is a link error -- so this only reports them, but reports them every
build, so that the day one is nearly full is not a surprise. The two small
regions in $7E are not: a slot cannot be narrower than its 32 KB ROM bank, so
a unit that outgrows $7E:5800-$7FFF or $7E:0600-$1BFF would link fine and
land on the image above it or on the stack. This fails the build instead --
and since wla is free to place a section anywhere in the 32 KB bank, it
also reads Make's resolved C_LAYOUT records in the adjacent config.inc for
which units those slots hold, and refuses any of their sections that landed
outside the region at all. Parsing Makefile assignments without evaluating
their conditionals mistakes FX3-only bank overrides for GSU-2 placements.

Usage: budget.py build/openlara.sym
"""

import re
from pathlib import Path
import sys


def main():
    path = sys.argv[1]
    config = Path(path).with_name("config.inc")
    try:
        config_text = config.read_text()
    except OSError:
        sys.exit(f"{config}: missing resolved build configuration; rebuild the cartridge")
    fx3 = ".DEFINE FX3 1" in config_text
    # FX3 code is in $84-$89 (the Makefile moved GSU-2's $3C-$3F numbering
    # down to $86-$89, leaving the GSU's window above it to level data).
    # The C_MOTION reference ROM is an oracle, not a cartridge: its C
    # replacements fill $86, and the FX3 inventory moves to $BB, past the
    # one packed level's data (Makefile).
    fx3_banks = ('84', '85', '86', '87', '88', '89')
    if ".DEFINE C_MOTION 1" in config_text:
        fx3_banks += ('bb',)
    hi = {}
    host_end = None
    bss_end = 0
    nav_end = 0x5800
    r4_end = 0x5800
    r5_end = 0x0600
    data_units = set()
    starts = {}
    section_starts = {}
    section_ends = {}
    overflow = []

    # Which units Make puts in the two small regions, so a section
    # of theirs placed past the region -- wla packs a 32 KB bank as it
    # likes -- is caught, not only one that starts inside and runs over.
    region_of = {}
    layouts = re.findall(r'^; C_LAYOUT (\w+) (\d+) (\w+) (\d+)$', config_text, re.MULTILINE)
    if not layouts:
        sys.exit(f"{config}: missing resolved C_LAYOUT records; rebuild the cartridge")
    for unit, _, _, slot in layouts:
        if not fx3 and slot == '7':
            region_of[unit] = (0x5800, 0x8000)
        elif not fx3 and slot == '8':
            region_of[unit] = (0x0600, 0x1C00)

    def unit_of(sec):
        m = re.search(r'c_(\w+)\.asm', sec) or re.search(r'\.c_(\w+)$', sec)
        return m.group(1) if m else None
    for line in open(path):
        m = re.match(r'^([0-9a-fA-F]{2}):([0-9a-fA-F]{4})\s+(\S+)', line)
        if not m:
            continue
        bank, addr, name = m.group(1).lower(), int(m.group(2), 16), m.group(3)
        if name.startswith('SECTIONSTART_'):
            section_starts[name[len('SECTIONSTART_'):]] = (bank, addr)
        elif name.startswith('SECTIONEND_'):
            section_ends[name[len('SECTIONEND_'):]] = (bank, addr)
        if name == 'HostCodeEnd':
            host_end = addr
        m2 = re.match(r'SECTIONSTART_ram\..*/(c_\w+)\.asm\.data', name)
        if m2 and addr < 0x8000 and bank == '04':
            data_units.add(m2.group(1))
        key = (bank, addr >= 0x8000)
        if bank in fx3_banks:
            hi[key] = max(hi.get(key, 0x8000), addr)
        if bank in ('7f', '7e'):
            if name.startswith('SECTIONSTART'):
                sec = name[len('SECTIONSTART'):]
                unit = unit_of(sec)
                if unit in region_of and not sec.lstrip('_').startswith('.bss'):
                    lo, hi_ = region_of[unit]
                    if bank != '7e' or not (lo <= addr < hi_):
                        overflow.append(f"{name} placed at ${bank.upper()}:{addr:04X}, outside "
                                        f"{unit}'s region ${lo:04X}-${hi_ - 1:04X}")
            if bank == '7e' and addr < 0x8000:
                if name == 'SECTIONEND_.bss.c_nav_fx3':
                    continue  # separate FX3 data budget, even if it ends below $8000
                if name.startswith('SECTIONEND_.bss') or name.startswith('SECTIONEND_globram'):
                    bss_end = max(bss_end, addr)
                elif name.startswith('SECTIONSTART'):
                    starts[name[len('SECTIONSTART'):]] = addr
                elif name.startswith('SECTIONEND'):
                    start = starts.get(name[len('SECTIONEND'):], addr)
                    if 0x5800 <= start < 0x8000:
                        r4_end = max(r4_end, addr)
                        if addr > 0x8000:
                            overflow.append(f"{name} ends at $7E:{addr:04X}, past $8000")
                    elif 0x0600 <= start < 0x1C00:
                        r5_end = max(r5_end, addr)
                        if addr > 0x1C00:
                            overflow.append(f"{name} ends at $7E:{addr:04X}, past the stack floor $1C00")
                continue
            hi[key] = max(hi.get(key, 0), addr)

    if host_end is None:
        sys.exit(f"{path}: HostCodeEnd is not in the symbol file")

    # Validate whole sections, including one that starts in the small upper
    # region and ends in $8000+. Such an end is in a different reporting
    # bucket above, but is still an overflow of the original region.
    for sec, (bank, start) in section_starts.items():
        if sec == '.bss.c_nav_fx3' or fx3 and sec == 'room_query_cache':
            end_bank, end = section_ends.get(sec, (bank, start))
            if not fx3 or bank != '7e' or end_bank != bank or not 0x5800 <= start <= end <= 0x10000:
                overflow.append(f"FX3 data {sec} is outside $7E:5800-$FFFF")
            nav_end = max(nav_end, end)
            continue
        if sec.startswith('.bss') or sec.startswith('globram'):
            continue
        # CPU assembly sections use this suffix (apart from the two older
        # runtime/room sections). A manual list missed camera_fx3_native;
        # new native helpers must inherit the same placement guard. Native
        # interrupt vectors are fixed header data, not engine code.
        native = sec in ('libtcc', 'roomq') or (sec.endswith('_native') and sec != 'vectors_native')
        if fx3 and (unit_of(sec) or native):
            end_bank, end = section_ends.get(sec, (bank, start))
            if bank not in fx3_banks or end_bank != bank or \
                    not 0x8000 <= start <= end <= 0x10000:
                overflow.append(f"{sec} at ${bank.upper()}:{start:04X} is not in FX3 FastROM code allocation")
            continue
        region = (0x0600, 0x1C00) if sec == 'math_native' else region_of.get(unit_of(sec))
        if sec in ('bear_state_native', 'bear_update_native'):
            region = (0x5800, 0x8000)
        if region is None and bank == '7e':
            if 0x0600 <= start < 0x1C00:
                region = (0x0600, 0x1C00)
            elif 0x5800 <= start < 0x8000:
                region = (0x5800, 0x8000)
        if region is None:
            continue
        lo, limit = region
        end_bank, end = section_ends.get(sec, (bank, start))
        if bank != '7e' or end_bank != bank or not (lo <= start <= end <= limit):
            overflow.append(f"{sec} spans ${bank.upper()}:{start:04X}-${end_bank.upper()}:{end:04X}, "
                            f"outside $7E:{lo:04X}-${limit - 1:04X}")

    # An initialised static or global is a bug on this target: 816-tcc puts
    # it in a .data pair, there is no crt0 to copy the values into WRAM, and
    # the RAM half links at $04:0000 -- the direct page -- where it aliases
    # the host's own variables. Two of them once clobbered fps and keys.
    if data_units:
        sys.exit("initialised C data is not supported here (no crt0 copies "
                 "it, and it aliases the direct page at $04:0000): " +
                 ", ".join(sorted(data_units)) +
                 " -- initialise in code instead")

    def row(label, end, base, limit):
        used = end - base
        free = limit - end
        print(f"  {label:<22s} {used:6d} used  {free:6d} free")

    print("FX3 memory budget:" if fx3 else "WRAM budget:")
    row("host asm $7E:0110",   host_end, 0x0110, 0x0600)
    if fx3:
        defs = config.with_name("hostbank.inc").read_text()
        sectors = int(re.search(r"HOST_SECTOR_SIZE (\d+)", defs)[1])
        staging = ".DEFINE FX3_STAGE 1" in config_text
        row("FX3 sectors $7F:0000", sectors, 0, 0x8000 if staging else 0x10000)
        if staging:
            if sectors > 0x8000:
                overflow.append("FX3 mutable sectors overlap WRAM frame staging at $7F:8000")
            if (section_starts.get('fx3_frame_stage') != ('7f', 0x8000) or
                    section_ends.get('fx3_frame_stage') != ('7f', 0xF980)):
                overflow.append("FX3 frame staging must occupy exactly $7F:8000-$F97F")
            print("  FX3 frame staging: 31,104 bytes at $7F:8000-$F97F")
        for sec, lo, limit in [('host_ptrs_low', 0x28, 0x30),
                               ('host_ptrs_middle', 0x58, 0x60),
                               ('host_ptrs_high', 0xC0, 0xFA)]:
            if section_starts.get(sec) != ('00', lo) or section_ends.get(sec) != ('00', limit):
                overflow.append(f"FX3 table pointers {sec} escaped their direct-page gap")
        print("  FX3 table pointers: 72 bytes + 2-byte ready flag in direct-page gaps")
        for bank in fx3_banks:
            row(f"C ROM    ${bank.upper()}:8000", hi.get((bank, True), 0x8000), 0x8000, 0x10000)
        row("FX3 data $7E:5800", nav_end, 0x5800, 0x10000)
        print(f"  Former $7E code regions: {0x10000 - nav_end + 0x1600} bytes available; BSS/stack bounds unchanged")
    else:
        row("C image  $7E:0600",   r5_end, 0x0600, 0x1C00)
        row("C image  $7F:0000",   hi.get(('7f', False), 0), 0x0000, 0x8000)
        row("C image  $7F:8000",   hi.get(('7f', True), 0x8000), 0x8000, 0x10000)
        row("C image  $7E:8000",   hi.get(('7e', True), 0x8000), 0x8000, 0x10000)
        row("C image  $7E:5800",   r4_end, 0x5800, 0x8000)
    row("C .bss   $7E:2000",   bss_end, 0x2000, 0x5800)

    if host_end > 0x0600:
        sys.exit(f"host code ends at ${host_end:04X}: past the WRAM "
                 f"$0100-$05FF budget")
    if overflow:
        if fx3:
            sys.exit("invalid FX3 code layout:\n  " + "\n  ".join(overflow))
        sys.exit("a C image outgrew its WRAM region (the linker cannot see the "
                 "limit; see hdr.asm slots 7 and 8):\n  " + "\n  ".join(overflow))


if __name__ == "__main__":
    main()
