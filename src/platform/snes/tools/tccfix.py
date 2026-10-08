#!/usr/bin/env python3
"""Retarget 816-tcc output to resident WRAM (GSU-2) or FastROM (FX3).

816-tcc emits one `.SECTION ".<fn>text_0x0" SUPERFREE` per function and a
`.SECTION ".rodata" SUPERFREE` for constants, and SUPERFREE means "put it
anywhere in ROM". On an ordinary SNES that is correct. On this target it is
not: while the GSU owns the ROM bus the 65816 reads dummy data from cartridge
ROM, so both the code *and* the constants it reads have to be sitting in WRAM
by the time the GSU starts.

So every SUPERFREE section is rewritten to land in one nominated ROM bank,
addressed through the WRAM code slot. That bank becomes a contiguous image
that the boot code copies to $7F:0000 in one DMA, and the labels inside it
already resolve to their WRAM addresses.

The `.bss` and APPENDTO sections stay in RAM. `--bss-slot` optionally selects
a separate WRAM allocation for a translation unit; the default is slot 2.

The labels also need the right bank byte. 816-tcc emits `jsr.l` for every call
between C functions, and wla takes that bank from the section's ROM bank --
which would send the call into cartridge ROM. `.BASE` adds an offset to the
bank byte used for long addresses, so an image in ROM bank 4 destined for WRAM
bank $7F needs `.BASE $7B`. That offset is derived from the two bank numbers
rather than given, because getting them out of step is silent.

Section names are also tagged with the source file, because 816-tcc puts the
file name in a function's section but not in its ".rodata" -- and two
translation units then collide on the label wla derives from that name.

Usage:
    tccfix.py in.asm out.asm --bank 4 --slot 4 --wrambank 0x7F

FX3 permits concurrent CPU ROM fetches. All engine images instead use
slot 0 and FastROM banks $84/$85/$BD/$BE/$BF: the same .BASE calculation,
with no boot code copies. $7F holds the host blob and Nav uses the former
$7E:5800+ code region via BSS slot 9. The historical
--wrambank argument therefore names the run bank, not necessarily WRAM.

--fast-sign-extend replaces the exact 15-iteration arithmetic shift idiom
with a shorter branch/load sequence. It preserves the result, registers,
flags (including carry/overflow), and anonymous-label targets, using two
temporary stack bytes. FX3 native builds trade extra ROM bytes for speed;
C_MOTION keeps the compiler's original sequence as a reference.
--compact-sign-extend uses the same proven body through one resident helper
per translation unit. A four-byte JSL replaces each ten-byte loop only when
at least four loops share the 23-byte helper, so resident GSU-2 code shrinks.
Its maximum extra stack use is five bytes (JSL return plus saved A).
--fast-wide-shifts similarly shortens exact 9..15-bit logical and 9..14-bit
arithmetic shift loops: byte-swap/mask (with sign fill for arithmetic),
the remaining shifts, then Y=0 to restore final N/Z.
At least one real shift remains, preserving the original final carry bit.

--wram-globals takes wramglobals.py's linker-owned object manifest. Under
TCC's DB=$7E/D=0 ABI, bounded constant-offset accesses can use absolute or
direct-page addressing. Unknown objects, ROM, indexed operands and accesses
crossing an object's extent retain their original long addressing.
"""

import argparse
import json
import os
import re
import sys

SUPERFREE = re.compile(r'^\.SECTION\s+"([^"]+)"\s+SUPERFREE\s*$')
SECTION = re.compile(r'^\.SECTION\b')
RAMSECTION = re.compile(r'^\.RAMSECTION\b')
BSS = re.compile(r'^\.RAMSECTION\s+"\.bss"')
SIGN_EXTEND = re.compile(r'^ldy(?:\.w)? #15\n-\ncmp #\$8000\nror a\ndey\nbne -\n\+\n', re.MULTILINE)
WIDE_SHIFT = re.compile(r'^ldy(?:\.w)? #(9|1[0-5])\n-\n(asl|lsr) a\ndey\nbne -\n\+\n', re.MULTILINE)
WIDE_SAR = re.compile(r'^ldy(?:\.w)? #(9|1[0-4])\n-\ncmp #\$8000\nror a\ndey\nbne -\n\+\n', re.MULTILINE)


def fast_sign_extend(source):
    # Arithmetic A >> 15. Preserve A, Y=0 and ALL original flags: the last
    # ROR leaves source bit 14 in C, DEY leaves Z=1/N=0, V is untouched.
    # The balanced PHA/PLA costs two temporary stack bytes; no tcc register
    # or X is borrowed. Keep the anonymous labels for neighbouring branches.
    serial = 0
    def replace(match):
        nonlocal serial
        label = f'tccs_{{WLA_FILENAME}}_signext_{serial}'
        serial += 1
        # Branch/load, not SBC: even SBC #0 would clear a previously set V.
        return (f'-\ntay\nand #$8000\nbeq {label}_positive\nlda #$FFFF\n'
                f'bra {label}_value\n{label}_positive:\nlda #0\n{label}_value:\n'
                'pha\ntya\nasl a\nasl a\npla\nldy #0\n+\n')
    return SIGN_EXTEND.sub(replace, source)


def fast_wide_shifts(source):
    def replace(match):
        count, opcode = int(match[1]), match[2]
        mask = '$FF00' if opcode == 'asl' else '$00FF'
        return '-\nxba\nand #' + mask + '\n' + (opcode + ' a\n') * (count - 8) + 'ldy #0\n+\n'
    source = WIDE_SHIFT.sub(replace, source)
    serial = 0
    def arithmetic(match):
        nonlocal serial
        label = f'tccs_{{WLA_FILENAME}}_sar_{serial}'
        serial += 1
        # Sign-extend the original high byte, then perform the remaining
        # shifts. A real final ROR retains the original bit n-1 in C. None
        # of these instructions changes V; Y=0 restores the loop's N/Z.
        return ('-\nxba\nand #$00FF\ncmp #$0080\nbcc ' + label + '\n'
                'ora #$FF00\n' + label + ':\n' +
                'cmp #$8000\nror a\n' * (int(match[1]) - 8) + 'ldy #0\n+\n')
    return WIDE_SAR.sub(arithmetic, source)


def compact_sign_extend(source):
    # Resident WRAM is scarce on GSU-2. Four call sites save 24 bytes, enough
    # to pay for the exact 22-byte fast body and its RTL. JSL/RTL preserve
    # the caller's flags, and the body preserves X without a scratch global.
    if len(SIGN_EXTEND.findall(source)) < 4:
        return source
    helper = 'tccs_{WLA_FILENAME}_signext_shared'
    result = SIGN_EXTEND.sub('-\njsr.l ' + helper + '\n+\n', source)
    body = fast_sign_extend('ldy #15\n-\ncmp #$8000\nror a\ndey\nbne -\n+\n')
    return (result + '\n.SECTION ".signext_shared" SUPERFREE\n' +
            helper + ':\n' + body + 'rtl\n.ENDS\n')


def fast_wram_globals(source, symbols):
    # TCC's fixed ABI is DB=$7E/D=0. Only direct, constant-offset accesses
    # wholly inside known RAM objects qualify. Keep indexing, ROM symbols,
    # pointer arithmetic and potentially crossing word accesses unchanged.
    access = re.compile(r'^(lda|sta|adc|sbc|cmp|and|ora|eor)\.l (\w+)((?: \+ \d+)*)([ \t]*(?:;[^\n]*)?)$', re.MULTILINE)
    def replace(match):
        record = symbols.get(match[2])
        offset = sum(map(int, re.findall(r'\d+', match[3])))
        if record is None or offset + 2 > record['size']:
            return match[0]
        mode = record['mode']
        if mode not in ('w', 'b'):
            raise ValueError(f'unsupported RAM access mode {mode}')
        return f'{match[1]}.{mode} {match[2]}{match[3]}{match[4]}'
    return access.sub(replace, source)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--bank", type=int, required=True,
                    help="ROM bank holding the image copied to WRAM")
    ap.add_argument("--slot", type=int, required=True,
                    help="memory map slot whose base is the WRAM run address")
    ap.add_argument("--wrambank", type=lambda v: int(v, 0), required=True,
                    help="run bank, e.g. 0x7F for WRAM or 0x84 for FX3 FastROM")
    ap.add_argument("--tag", default=None,
                    help="suffix for section names that 816-tcc does not make "
                         "unique per file; defaults to the input's stem")
    ap.add_argument("--bss-slot", type=int, default=2,
                    help="WRAM slot for this unit's BSS (FX3 Nav uses slot 9)")
    sign_group = ap.add_mutually_exclusive_group()
    sign_group.add_argument("--fast-sign-extend", action="store_true",
                    help="replace the 15-iteration sign-extension idiom (FX3 native build)")
    sign_group.add_argument("--compact-sign-extend", action="store_true",
                    help="share a faster sign-extension helper only when resident code shrinks")
    ap.add_argument("--fast-wide-shifts", action="store_true",
                    help="replace 9..15-bit logical shift loops (FX3 native build)")
    ap.add_argument("--wram-globals", help="verified C WRAM/direct-page symbol manifest (native FX3)")
    args = ap.parse_args()

    if args.tag is None:
        args.tag = os.path.basename(args.input).split(".")[0]

    # .BASE is an offset added to the bank byte, not the bank byte itself, so
    # it depends on which ROM bank holds the image. Deriving it here rather
    # than passing it in: getting the two out of step silently sends every
    # call into the wrong bank, which looks exactly like a dead machine.
    args.base = args.wrambank - args.bank
    if not 0 <= args.base <= 0xFF:
        sys.exit(f"wram bank ${args.wrambank:02X} unreachable from ROM bank {args.bank}")

    out = []
    moved = 0
    base = None

    def set_base(value):
        # .BASE is positional and applies to whatever is parsed next, RAM
        # sections included -- leaving it set while a .RAMSECTION is read put
        # .bss in the wrong bank as well. So it is toggled per
        # section kind rather than set once for the file.
        nonlocal base
        if base != value:
            out.append(f".BASE ${value:02X}\n")
            base = value

    with open(args.input) as f:
        source = f.read()
    if args.compact_sign_extend:
        source = compact_sign_extend(source)
    for line in source.splitlines(keepends=True):
        stripped = line.rstrip("\n")

        if RAMSECTION.match(stripped):
            set_base(0x00)
            # 816-tcc names the uninitialised-data section ".bss" in every
            # translation unit, so the moment a second one has a static the
            # linker sees the same label twice. Tagged for the same reason
            # ".rodata" is, and found the same way: by adding one.
            m = BSS.match(stripped)
            if m:
                line = line.replace('".bss"', f'".bss.{args.tag}"', 1)
                line = re.sub(r'\bSLOT\s+2\b', f'SLOT {args.bss_slot}', line)
            out.append(line)
            continue

        m = SUPERFREE.match(stripped)
        if m:
            set_base(args.base)
            # 816-tcc puts the source file's name in a function's section but
            # not in ".rodata", so two translation units collide on the label
            # wla derives from it. Tagging every section is simpler than
            # special-casing the one that needs it.
            name = f"{m.group(1)}.{args.tag}"
            # SEMIFREE keeps wla free to pack sections within the bank, while
            # .BANK/.SLOT pin which bank and which address space they use.
            out.append(f".BANK {args.bank} SLOT {args.slot}\n")
            out.append(f'.SECTION "{name}" SEMIFREE\n')
            moved += 1
            continue

        if SECTION.match(stripped):
            # APPENDTO groups inherit their parent's bank; they still need the
            # WRAM bank byte because the host reads them as ordinary data.
            set_base(args.base)

        out.append(line)

    result = ''.join(out)
    if args.fast_sign_extend:
        result = fast_sign_extend(result)
    if args.fast_wide_shifts:
        result = fast_wide_shifts(result)
    if args.wram_globals:
        with open(args.wram_globals) as f:
            result = fast_wram_globals(result, json.load(f))
    with open(args.output, "w") as f:
        f.write(result)

    print(f"{args.input}: {moved} section(s) -> bank {args.bank} slot {args.slot}",
          file=sys.stderr)


if __name__ == "__main__":
    main()
