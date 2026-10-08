#!/usr/bin/env python3
"""Exercise the ROM's real span writers against an independent pixel oracle.

No gameplay boot or frame snapshot oracle: inject a synthetic texture page,
seed both framebuffers, run one span, and compare every framebuffer/shade byte.
The same fixtures run in TypeSNES to cover its buffered hardware PLOT path.
"""
import argparse
import json
from pathlib import Path
import re
import zlib

import gsusim


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('rom', type=Path)
    ap.add_argument('symbols', type=Path)
    ap.add_argument('--mode7', action='store_true')
    ap.add_argument('--fx3', action='store_true')
    ap.add_argument('--shading', action='store_true')
    ap.add_argument('--fixtures', type=Path, required=True)
    args = ap.parse_args()
    if args.fx3:
        gsusim.configure_fx3()
    elif args.mode7:
        gsusim.configure_mode7()
    syms = {}
    for line in args.symbols.read_text().splitlines():
        parts = line.split()
        if len(parts) == 2 and ':' in parts[0]:
            syms[parts[1]] = int(parts[0].split(':')[1], 16)
    assert syms['Raster_Common'] % 16 == 0, 'retune RASTER_PAD after moving the cache anchor'

    width, height = (128, 64) if args.mode7 else (216, 144) if args.fx3 else (256, 160)
    buffers = [0x10000] if args.fx3 else [0x1000, 0x6000]
    # Include unused padding rows/columns and the other framebuffer in guards.
    size = 0xA000 if args.fx3 else 0x2000 if args.mode7 else 0x5000
    ranges = [(fb, size) for fb in buffers] + [(0xEA00, 0x280), (0xED00, 0x280)]
    texels = bytes([0x80, 0, 5, 0x80, 15, 9, 0, 0x80][(u + v * 3) % 8]
                   if v < 254 else 0 if v == 254 else 0x80
                   for v in range(256) for u in range(256))
    patches = [(0x1F0000, texels.hex()),
               (0x10000 + syms['RFT_Advance'], '3d4c0001')]  # RPIX; STOP; NOP
    original = bytearray(args.rom.read_bytes())
    for offset, data in patches:
        original[offset:offset + len(bytes.fromhex(data))] = bytes.fromhex(data)

    def set_pixel(ram, fb, x, y, color):
        if args.mode7:
            ram[fb + (x // 8) * 512 + y * 8 + x % 8] = color
        else:
            planes = 8 if args.fx3 else 4
            tile = (x // 8) * 20 + y // 8
            for plane in range(planes):
                a = fb + tile * planes * 8 + (plane // 2) * 16 + (y % 8) * 2 + plane % 2
                mask = 0x80 >> (x % 8)
                ram[a] = (ram[a] & ~mask) | (mask if color & (1 << plane) else 0)

    specs = [
        ('single hole', 0, 0, 1, 0, 0, 256, 0),
        ('single opaque black', 7, 7, 1, 256, 0, 256, 0),
        ('odd tile crossing', 6, 9, 3, 0, 0, 256, 0),
        ('several tiles', 5, 15, 27, 0, 0, 256, 0),
        ('right bottom edge', width - 7, height - 1, 7, 0, 0, 256, 0),
        ('fractional UV wrap', 3, 8, 23, 0xFF80, 0x0180, 0x01C0, 0xFFC0),
        ('all holes', 5, 9, 27, 0, 0xFF00, 256, 0),
        ('all opaque black', 5, 9, 27, 0, 0xFE00, 256, 0),
    ]
    cases = []
    for fb in buffers:
        for keyed in (False, True):
            entry = syms['SFTA_Span' if keyed else 'SFT_Span']
            # RAMB=0, ROMB=$5F, CMODE=1 (draw opaque zero), jump to actual writer.
            wrapper = bytes([0xA0, 0, 0x3E, 0xDF, 0xA0, 0x5F, 0x3F, 0xDF,
                             0xA0, 1, 0x3D, 0x4E, 0xFD, entry & 255, entry >> 8,
                             0x9D, 1])
            rom = bytearray(original)
            rom[0x17F00:0x17F00 + len(wrapper)] = wrapper
            for name, x, y, count, u, v, du, dv in specs:
                name = f'{"keyed" if keyed else "opaque"} ${fb:05x}: {name}'
                regs = {1: x, 2: y, 5: count, 7: v, 8: u, 9: dv, 10: du}
                shade = 0xEA00 if fb != 0x6000 else 0xED00
                g = 20 << (10 if args.mode7 or args.fx3 else 8)
                words = [(0x0E, g), (0xEA, shade), (0xFC, 0), (0xFE, g),
                         (0x100, x), (0x102, count)]
                if args.mode7:
                    words += [(0x3000 + i * 2, fb + (i // 8) * 512 + i % 8)
                              for i in range(width)]
                initial = bytearray([0xA6]) * 0x20000
                for a, value in words:
                    initial[a:a + 2] = value.to_bytes(2, 'little')
                expected = bytearray(initial)
                for i in range(count):
                    texel = texels[((v + i * dv) & 0xFF00) | (((u + i * du) >> 8) & 255)]
                    if keyed and texel & 0x80:
                        continue
                    color = texel | (0x50 if args.shading and (args.mode7 or args.fx3) else 0)
                    set_pixel(expected, fb, x + i, y, color)
                    if args.shading and not (args.mode7 or args.fx3):
                        a = shade + (y // 8) * 32 + (x + i) // 8
                        expected[a] = (expected[a] & 3) | 20
                checks = [(a, n, zlib.crc32(expected[a:a + n])) for a, n in ranges]
                gsu = gsusim.GSU(rom, scbr=fb // 0x400, pbr=0x41, entry=0x7F00)
                gsu.ram[:] = initial
                for reg, value in regs.items():
                    gsu.r[reg] = value
                assert gsu.run(limit=100000), f'{name}: no STOP'
                for a, n, crc in checks:
                    assert zlib.crc32(gsu.ram[a:a + n]) == crc, f'{name}: RAM ${a:05x} differs'
                cases.append(dict(name=name, scbr=fb // 0x400,
                                  scmr=0x1F if args.fx3 else 0x1D,
                                  wrapper=wrapper.hex(), regs=regs, words=words, checks=checks,
                                  rombr=0x5F, patches=[]))

    # Whole raster entry, not just a patched one-line span. A zero-height
    # portal window can skip an above-screen edge to Y=bottom, start a strip
    # with R3=0, then wrap its countdown to 65535. Restore RFT_Advance so this
    # fixture must actually return, and guard every framebuffer/padding byte.
    advance = 0x10000 + syms['RFT_Advance']
    restore_advance = (advance, args.rom.read_bytes()[advance:advance + 4].hex())
    stop_return = (0x17EF0, '3d4c0001')  # flush pixels; STOP; NOP
    for fb in buffers:
        for entry_name in ('RasterF', 'RasterFT', 'RasterFTA'):
            for name, top, bottom in [('empty at zero', 0, 0),
                                      ('empty inside screen', 20, 20),
                                      ('reversed window', 20, 10),
                                      ('one visible row', 0, 1)]:
                entry = syms[entry_name]
                wrapper = bytes([0xA0, 0, 0x3E, 0xDF, 0xA0, 0x5F, 0x3F, 0xDF,
                                 0xA0, 1, 0x3D, 0x4E, 0xA0, 5, 0x4E,
                                 0xFB, 0xF0, 0x7E, 0xFD, entry & 255, entry >> 8, 0x9D, 1])
                regs = {2: 0xFFFC, 6: 5 if entry_name == 'RasterF' else 0x5F,
                        8: 0x440, 9: 0x400, 12: 0}
                shade = 0xEA00 if fb != 0x6000 else 0xED00
                g = 20 << (10 if args.mode7 or args.fx3 else 8)
                words = [(0x106, 0), (0x108, width), (0x10A, top), (0x10C, bottom),
                         (0xEA, shade), (0x480 + 18, 0xFFFF)]
                for cell, x in [(0x400, 8), (0x440, 16)]:
                    words += [(cell, x), (cell + 2, 0xFFFC), (cell + 4, 20),
                              (cell + 8, 2), (cell + 16, 0x480), (cell + 18, 40),
                              *[(cell + k, 0) for k in range(20, 30, 2)], (cell + 30, g)]
                if args.mode7:
                    words += [(0x3000 + i * 2, fb + (i // 8) * 512 + i % 8)
                              for i in range(width)]
                initial = bytearray([0xA6]) * 0x20000
                for a, value in words:
                    initial[a:a + 2] = value.to_bytes(2, 'little')
                expected = bytearray(initial)
                if bottom > top:
                    for x in range(8, 17):  # the shared edge walker includes the right edge
                        color = 5 | (0x50 if args.shading and (args.mode7 or args.fx3) else 0)
                        set_pixel(expected, fb, x, 0, color)
                        if args.shading and not (args.mode7 or args.fx3):
                            a = shade + x // 8
                            expected[a] = (expected[a] & 3) | 20
                checks = [(a, n, zlib.crc32(expected[a:a + n])) for a, n in ranges]
                overrides = [restore_advance, stop_return]
                rom = bytearray(original)
                for a, hex_data in overrides:
                    rom[a:a + len(bytes.fromhex(hex_data))] = bytes.fromhex(hex_data)
                rom[0x17F00:0x17F00 + len(wrapper)] = wrapper
                gsu = gsusim.GSU(rom, scbr=fb // 0x400, pbr=0x41, entry=0x7F00)
                gsu.ram[:] = initial
                for reg, value in regs.items():
                    gsu.r[reg] = value
                label = f'{entry_name} ${fb:05x}: {name}'
                assert gsu.run(limit=100000), f'{label}: no STOP (empty strip countdown wrapped)'
                for a, n, crc in checks:
                    assert zlib.crc32(gsu.ram[a:a + n]) == crc, f'{label}: RAM ${a:05x} differs'
                cases.append(dict(name=label, scbr=fb // 0x400,
                                  scmr=0x1F if args.fx3 else 0x1D,
                                  wrapper=wrapper.hex(), regs=regs, words=words, checks=checks,
                                  rombr=0x5F, patches=overrides))

    # Exercise the real batch dispatch too: both face flags must select the
    # right writer, and the key bit must not leak into the texture ROM bank.
    # Stop at RasterInit, after writer selection and ROMB but before edge setup.
    bank_text = (args.symbols.parent / 'texbank.inc').read_text()
    tex_bank = int(re.search(r'TEXTURE_BANK_BASE\s+\$([0-9A-Fa-f]+)', bank_text)[1], 16)
    for keyed in (False, True):
        for page in (0, 3, 11):
            entry = syms['BF_Texture']
            wrapper = bytes([0xFD, entry & 255, entry >> 8, 0x9D, 1])
            stop = (0x10000 + syms['RasterInit'], '0001')
            rom = bytearray(original)
            rom[stop[0]:stop[0] + 2] = bytes.fromhex(stop[1])
            rom[0x17F00:0x17F00 + len(wrapper)] = wrapper
            regs = {6: page | (0x40 if keyed else 0)}
            writer = syms['SFTA_Span' if keyed else 'SFT_Span'].to_bytes(2, 'little')
            gsu = gsusim.GSU(rom, pbr=0x41, entry=0x7F00)
            gsu.r[6] = regs[6]
            assert gsu.run(limit=1000), 'batch dispatch did not stop'
            assert gsu.rombr == tex_bank + page, 'face flags corrupted ROM bank'
            assert gsu.ram[0x38:0x3A] == writer, 'wrong span writer selected'
            cases.append(dict(name=f'face dispatch: key={keyed}, page={page}', scbr=4,
                              scmr=0x1F if args.fx3 else 0x1D, wrapper=wrapper.hex(),
                              regs=regs, words=[], checks=[(0x38, 2, zlib.crc32(writer))],
                              rombr=tex_bank + page, patches=[stop]))
    args.fixtures.write_text(json.dumps(dict(hostBase=0x7000 if args.fx3 else 0x3000,
                                            patches=patches, cases=cases)))
    print(f'keycheck: {len(cases)} span/clip/dispatch cases passed; fixtures: {args.fixtures}')


if __name__ == '__main__':
    main()
