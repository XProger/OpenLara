#!/usr/bin/env python3
"""Check cartridge pistol poses, mesh swaps and shoulder bases in the GSU.

Exercise every packed arm frame at every joint, with asymmetric arm states.
Expected bytes come from the packer's source-data export, independently of
the GSU's pointer tables. This catches wrong joint/arm indices, stale ROM
buffers, mesh radii and accidental changes to the body or shoulder origin.
"""
import json
from pathlib import Path
import struct
import sys

from gsusim import GSU


def main():
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "build")
    rom = (root / "openlara.sfc").read_bytes()
    level = (root / "data/LEVEL.BIN").read_bytes()
    weapon = json.loads((root / "data/WEAPON.json").read_text())
    syms = {}
    for line in (root / "gsu_sym.sym").read_text().splitlines():
        parts = line.split()
        if len(parts) == 2 and ":" in parts[0]:
            syms[parts[1]] = int(parts[0].split(":")[1], 16)
    if not weapon["animations"]:
        print("weaponcheck: level has no pistol model")
        return
    probe = GSU(rom, pbr=0x41)
    stop = next(i for i in range(0x8000) if probe.rom_read(0x41, i) == 0)

    def put(g, addr, value):
        struct.pack_into("<H", g.ram, addr, value & 65535)

    def get(g, addr):
        return struct.unpack_from("<H", g.ram, addr)[0]

    def run(g, name):
        g.pc = syms[name]
        g.r[11] = stop
        assert g.run(limit=10000), f"{name} did not return"

    def machine(joint, state, arms):
        g = GSU(rom, pbr=0x41)
        put(g, 0xF626, 1)  # WA_LARA
        put(g, 0x1D8, 15 - joint)
        put(g, 0xF710, state)
        for i, arm in enumerate(arms):
            for n, v in enumerate(arm):
                put(g, 0xF712 + i * 12 + n * 2, v)
        put(g, 0x1DE, 0x1234)  # original mesh
        put(g, 0x1E0, 5)
        put(g, 0x1E2, 0x7603)  # original radius and push/pop flags
        for i, v in enumerate((21, 34, 55)):
            put(g, 0x1EA + i * 2, v)
        return g

    cases = 0
    for anim, record in enumerate(weapon["animations"]):
        for frame in range(record["last"] + 2):  # one past the range clamps
            for joint in range(15):
                arms = [[anim, frame, 0, 0, 0, frame & 1], [0, 3, 0, 0, 0, 1 - (frame & 1)]]
                g = machine(joint, 4, arms)
                run(g, "WeaponModelNode")
                pose = [21, 34, 55]
                if 8 <= joint <= 13:
                    a, f = arms[int(joint >= 11)][:2]
                    r = weapon["animations"][a]
                    if f > r["last"]:
                        f = 0
                    offset = r["offset"] + f * 51 + 6 + joint * 3
                    pose = list(level[offset:offset + 3])
                assert [get(g, 0x1EA + i * 2) for i in range(3)] == pose, (anim, frame, joint)
                if joint in (1, 4, 10, 13):
                    pair = (1, 4, 10, 13).index(joint)
                    drawn = arms[int(joint in (1, 13))][5]
                    r = weapon["meshes"][pair * 2 + (drawn if joint >= 8 else 1 - drawn)]
                    assert get(g, 0x1DE) == (r["offset"] & 65535)
                    assert get(g, 0x1E0) == r["offset"] >> 16
                    assert get(g, 0x1E2) == (r["radius"] << 8) | 3
                else:
                    assert [get(g, a) for a in (0x1DE, 0x1E0, 0x1E2)] == [0x1234, 5, 0x7603]
                cases += 1
    for state, anim in ((0, 0), (1, 0), (4, 4), (4, 65535)):
        g = machine(8, state, [[anim, 0, 0, 0, 1, 0]] * 2)
        run(g, "WeaponModelNode")
        assert [get(g, 0x1EA + i * 2) for i in range(3)] == [21, 34, 55]
    g = machine(8, 4, [[0, 4, 0, 0, 1, 1]] * 2)
    put(g, 0xF626, 0)
    run(g, "WeaponModelNode")
    assert [get(g, 0x1EA + i * 2) for i in range(3)] == [21, 34, 55], "another item read Lara's arms"

    # Save/restore only the nine basis terms; the translated shoulder must
    # retain the position obtained from the body's hips and torso.
    basis = [27000, -5000, 1000, 11, 3000, 24000, -2000, 22, -1500, 7000, 29000, 33]
    for joint in (8, 11):
        g = machine(joint, 4, [[0, 4, 0, 0, 1, 1]] * 2)
        put(g, 0x1D8, 15); put(g, 0x1CC, 0)
        for i, v in enumerate(basis):
            put(g, 0x80 + i * 2, v)
        run(g, "WeaponModelBegin")
        assert get(g, 0xF626) == 1
        for i in range(12):
            put(g, 0x80 + i * 2, 123 + i)
        put(g, 0x1D8, 15 - joint)
        run(g, "WeaponModelNode")
        run(g, "WeaponModelBasis")
        for i, v in enumerate(basis):
            expected = 123 + i if i % 4 == 3 else v & 65535
            assert get(g, 0x80 + i * 2) == expected, (joint, i, "shoulder basis")
        # At a quarter turn around Y, column X becomes -Z and Z becomes X.
        # MatrixRotate's 1.15 unit is 32767, so allow two rounding units.
        arm = 0xF712 + (12 if joint == 11 else 0)
        put(g, arm + 6, 0x4000)
        run(g, "WeaponModelBasis")
        for row in range(3):
            expected = [-basis[row * 4 + 2], basis[row * 4 + 1], basis[row * 4]]
            for col, v in enumerate(expected):
                actual = get(g, 0x80 + row * 8 + col * 2)
                actual = (actual + 32768) % 65536 - 32768
                assert abs(actual - v) <= 2, (joint, row, col, actual, v)
            assert get(g, 0x86 + row * 8) == 126 + row * 4
        # Both axes together distinguish Y-then-X from X-then-Y and catch
        # reading the wrong angle word out of the arm channel.
        put(g, arm + 4, 0x4000)
        run(g, "WeaponModelBasis")
        for row in range(3):
            expected = [-basis[row * 4 + 2], basis[row * 4], -basis[row * 4 + 1]]
            for col, v in enumerate(expected):
                actual = (get(g, 0x80 + row * 8 + col * 2) + 32768) % 65536 - 32768
                # Each 1.15 rotation can round by two units; this term
                # passes through both of them.
                assert abs(actual - v) <= 4, (joint, row, col, "YX", actual, v)
            assert get(g, 0x86 + row * 8) == 126 + row * 4
    # Look is a post-animation Y-then-X rotation at torso 7 and head 14.
    # Exercise a nonidentity matrix so a replaced basis or wrong composition
    # cannot pass. Translation and every other joint must remain untouched.
    look_cases = 0
    for lara in (0, 1):
        for joint in range(15):
            for torso, head in (((0, 0), (0, 0)), ((0x4000, 0x4000), (0, 0)),
                                ((0, 0), (0x4000, 0x4000)), ((0, 0xC000), (0xC000, 0))):
                g = machine(joint, 0, [[0, 0, 0, 0, 0, 0]] * 2)
                put(g, 0xF626, lara); put(g, 0xF624, joint)  # WA_NODE
                for i, v in enumerate(basis):
                    put(g, 0x80 + i * 2, v)
                for i, v in enumerate(torso + head):
                    put(g, 0xF72A + i * 2, v)
                run(g, "WeaponModelLook")
                angles = torso if joint == 7 else head if joint == 14 else (0, 0)
                if not lara:
                    angles = (0, 0)
                for row in range(3):
                    x, y, z, t = basis[row * 4:row * 4 + 4]
                    expected = (x, y, z, t)
                    if angles == (0x4000, 0x4000):
                        expected = (-z, x, -y, t)
                    elif angles == (0, 0xC000):
                        expected = (z, y, -x, t)
                    elif angles == (0xC000, 0):
                        expected = (x, -z, y, t)
                    for col, v in enumerate(expected):
                        actual = (get(g, 0x80 + row * 8 + col * 2) + 32768) % 65536 - 32768
                        tolerance = 4 if angles != (0, 0) and col != 3 else 0
                        assert abs(actual - v) <= tolerance, (lara, joint, angles, row, col, actual, v)
                look_cases += 1
    print(f"weaponcheck: {cases} joint/frame cases, mesh swaps, invalid poses, shoulder bases "
          f"and {look_cases} torso/head matrices agree")


if __name__ == "__main__":
    main()
