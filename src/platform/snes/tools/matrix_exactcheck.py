#!/usr/bin/env python3
"""Exercise every rotation angle with signed matrix edges and random inputs.

The oracle preserves the renderer's four separately truncated signed FMULTs,
16-bit wrap and doubling. It also checks untouched columns, translations,
matrix guards and the registers which MatrixRotate promises not to clobber.
An optional previous ROM checks exact output and records reference cycle costs.
These interpreter counts are not physical FX3 timing measurements.
"""
import argparse
import json
import random
import struct
from pathlib import Path

from gsusim import GSU, read_symbols
from matcheck import PAIRS, SEED


def s16(v):
    return (v + 32768) % 65536 - 32768


class Runner:
    def __init__(self, rom, symbols):
        self.rom = Path(rom).read_bytes()
        self.symbols = {name: at for at, name in read_symbols(symbols)}
        probe = GSU(self.rom, pbr=0x41)
        self.stop = next(at for at in range(0x8000) if probe.rom_read(0x41, at) == 0)
        base = self.symbols['ModelData']
        self.sine = [s16(probe.rom_read(0x41, base + i * 2) |
                         probe.rom_read(0x41, base + i * 2 + 1) << 8)
                     for i in range(256)]

    def run(self, routine, angle, seed):
        g = GSU(self.rom, pbr=0x41, entry=self.symbols[routine])
        preserved = (1, 2, 3, 4, 9, 12)  # GFCALL/GFJMP use R13
        for r in preserved:
            g.r[r] = 0xA500 + r
        g.r[0], g.r[11] = angle, self.stop
        g.ram[0x70:0x80] = bytes([0x5A]) * 16
        g.ram[0x98:0xA8] = bytes([0xA5]) * 16
        struct.pack_into('<12h', g.ram, 0x80, *seed)
        assert g.run(limit=2000), (routine, angle, 'no return')
        assert all(g.r[r] == 0xA500 + r for r in preserved), (routine, 'registers')
        assert g.ram[0x70:0x80] == bytes([0x5A]) * 16, 'lower matrix guard'
        assert g.ram[0x98:0xA8] == bytes([0xA5]) * 16, 'upper matrix guard'
        return list(struct.unpack_from('<12h', g.ram, 0x80)), g.cycles, g.steps

    def expected(self, routine, angle, seed):
        out = list(seed)
        sin, cos = self.sine[angle], self.sine[(angle + 64) & 255]
        ca, cb = PAIRS[routine]
        for row in range(3):
            a, b = seed[row * 4 + ca], seed[row * 4 + cb]
            out[row * 4 + ca] = s16(2 * ((a * cos >> 16) - (b * sin >> 16)))
            out[row * 4 + cb] = s16(2 * ((a * sin >> 16) + (b * cos >> 16)))
        return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom')
    parser.add_argument('symbols')
    parser.add_argument('--baseline-rom')
    parser.add_argument('--baseline-symbols')
    parser.add_argument('--report')
    args = parser.parse_args()
    if bool(args.baseline_rom) != bool(args.baseline_symbols):
        parser.error('both baseline paths are required')
    runner = Runner(args.rom, args.symbols)
    before = Runner(args.baseline_rom, args.baseline_symbols) if args.baseline_rom else None
    edges = [-32768, -32767, -16385, -16384, -1, 0, 1, 16383, 16384, 32766, 32767]
    seeds = [SEED] + [[edges[(i + shift) % len(edges)] for i in range(12)]
                      for shift in range(len(edges))]
    rng = random.Random(0x65816)
    seeds += [[rng.randrange(-32768, 32768) for _ in range(12)] for _ in range(20)]
    cases = 0
    cycles, steps = [0, 0], [0, 0]
    for routine in PAIRS:
        for angle in range(256):
            for seed in seeds:
                got, c, n = runner.run(routine, angle, seed)
                want = runner.expected(routine, angle, seed)
                assert got == want, (routine, angle, seed, got, want)
                cycles[1] += c
                steps[1] += n
                if before:
                    old, c, n = before.run(routine, angle, seed)
                    assert got == old, (routine, angle, 'baseline mismatch')
                    cycles[0] += c
                    steps[0] += n
                cases += 1
    result = dict(cases=cases, matrixElements=cases * 12, baseline=bool(before),
                  referenceCycles=cycles, instructions=steps)
    print('matrix_exactcheck: ' + json.dumps(result))
    if args.report:
        Path(args.report).write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
