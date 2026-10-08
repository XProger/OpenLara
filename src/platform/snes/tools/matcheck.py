#!/usr/bin/env python3
"""Check the GSU's matrix rotations against floating point.

The other oracles look at a whole frame, which is a poor way to find a sign
error in one of nine matrix elements: a wrong column still draws something,
and something is what a frame comparison sees. So this drives the routines on
their own -- seed the twelve words at M_00, set the angle, run from the entry
point to a STOP, read the matrix back -- and compares each element against the
same rotation done in floating point.

The seed matrix has every element distinct and no symmetry, because a matrix
with any is one a swapped column can survive. The reference reads the GSU's
own sine table out of the ROM rather than recomputing it: what is being
checked here is the arithmetic and the column pairs, not the table, and the
table has its own oracle in tools/fixedcheck.

Angles are the GSU's unit, 256 to the turn, which is the sine table's
resolution -- see "The three rotations" in the README.

    python3 tools/matcheck.py build/openlara.sfc build/gsu_sym.sym
"""

import sys

sys.path.insert(0, "tools")
from gsusim import GSU

PBR = 0x41              # the GSU program bank, as main.asm starts it
M_00 = 0x80             # the matrix, in game pak RAM
FIXED = 32768.0         # matrix elements are 1.15

# One element goes through two truncating FMULTs and a doubling, so three
# units of 32768 is the floor. Asserted rather than observed: if it grows,
# something changed that the frame comparison would not have caught.
TOLERANCE = 6

# Which columns each rotation mixes, from upstream's matrixRotateX/Y/Z_c:
#   matrixRotateX   X_ROTXY(e02, e01)
#   matrixRotateY   X_ROTXY(e00, e02)
#   matrixRotateZ   X_ROTXY(e01, e00)
PAIRS = {
    "MatrixRotateX": (2, 1),
    "MatrixRotateY": (0, 2),
    "MatrixRotateZ": (1, 0),
}

SEED = [3000, -5000,  9000, 111,
        -7000, 2000,  4500, 222,
        1500,  8000, -3500, 333]

ANGLES = (0, 1, 17, 32, 64, 100, 128, 192, 233, 255)


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def main():
    rom_path = sys.argv[1] if len(sys.argv) > 1 else "build/openlara.sfc"
    sym_path = sys.argv[2] if len(sys.argv) > 2 else "build/gsu_sym.sym"

    syms = {}
    for line in open(sym_path):
        parts = line.split()
        if len(parts) == 2 and ":" in parts[0]:
            syms[parts[1]] = int(parts[0].split(":")[1], 16)

    rom = open(rom_path, "rb").read()

    # The sine table the GSU itself reads, so this checks the multiply and the
    # column pairs rather than re-deriving the table.
    probe = GSU(rom, scbr=0, r3=0, pbr=PBR, entry=0)
    table = []
    for i in range(256):
        a = syms["ModelData"] + i * 2
        table.append(s16(probe.rom_read(PBR, a) | (probe.rom_read(PBR, a + 1) << 8)))

    # Somewhere to return to. Any $00 byte in the program bank is a STOP.
    stop_at = next(a for a in range(0x8000) if probe.rom_read(PBR, a) == 0x00)

    def run(routine, angle):
        gsu = GSU(rom, scbr=0, r3=0, pbr=PBR, entry=syms[routine])
        gsu.r[11] = stop_at
        for i, v in enumerate(SEED):
            gsu.ram[M_00 + i * 2] = v & 0xFF
            gsu.ram[M_00 + i * 2 + 1] = (v >> 8) & 0xFF
        gsu.r[0] = angle & 0xFF
        if not gsu.run():
            sys.exit(f"{routine}: no STOP within the step limit")
        return [s16(gsu.ram[M_00 + i * 2] | (gsu.ram[M_00 + i * 2 + 1] << 8))
                for i in range(12)]

    def reference(routine, angle):
        s = table[angle & 0xFF] / FIXED
        c = table[(angle + 64) & 0xFF] / FIXED
        ca, cb = PAIRS[routine]
        out = list(SEED)
        for row in range(3):
            a, b = SEED[row * 4 + ca], SEED[row * 4 + cb]
            out[row * 4 + ca] = a * c - b * s
            out[row * 4 + cb] = a * s + b * c
        return out

    print("matcheck: the GSU's matrix rotations against floating point")
    failures = 0
    worst = 0.0

    for routine in PAIRS:
        for angle in ANGLES:
            got = run(routine, angle)
            want = reference(routine, angle)
            for i in range(12):
                err = abs(got[i] - want[i])
                worst = max(worst, err)
                if err > TOLERANCE:
                    failures += 1
                    if failures <= 12:
                        print(f"  FAIL {routine} angle {angle} element {i}: "
                              f"got {got[i]} want {want[i]:.1f}")
        print(f"  {routine:14s} {len(ANGLES)} angles")

    print(f"  worst element error {worst:.1f} of {TOLERANCE} tolerance, "
          f"{FIXED:.0f} = 1.0")
    if failures:
        print(f"  {failures} failure(s)")
        return 1
    print("  all clear")
    return 0


if __name__ == "__main__":
    sys.exit(main())
