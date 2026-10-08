#!/usr/bin/env python3
"""Check that the host's per-entity channel is actually per entity.

The frame comparison in `make check` cannot see this. It stages one phase per
entity and the host computes the same ones, so the two agree byte for byte
whether or not the renderer reads the slot belonging to the entity it is
drawing -- if DrawRoomItems ignored the index and read slot zero for
everything, both sides would ignore it together and the comparison would still
pass. A regression there would be silent, and it is exactly the sort of
regression a refactor produces.

So this asks the questions the comparison cannot:

  * putting every other entity on a different animation must change the
    picture, which is only true if each entity reads its own slot
  * two different phases must give different frames, which is animation
    happening at all
  * choosing a different animation must give a different frame, which is the
    animation table being read at all
  * a phase and an animation far past what a model has must still render,
    which is the clamping doing its job

Room 14 is the subject because it has fourteen entities in it, several of the
same model, which is the case a per-entity slot exists for.

    python3 tools/animcheck.py build/openlara.sfc
"""

import sys
import zlib

sys.path.insert(0, "tools")
import gsusim
from gsusim import GSU

# The Mode 7 build (MODE7=1) has a different frame: `--mode7` switches the
# interpreter's layout, and the frame's size is read from it after that.
MODE7 = "--mode7" in sys.argv
if MODE7:
    sys.argv.remove("--mode7")
    gsusim.configure_mode7()
FX3 = "--fx3" in sys.argv
if FX3:
    sys.argv.remove("--fx3")
    gsusim.configure_fx3()

# Two views, because no single one answers everything. Room 14 has fourteen
# entities in it, several of the same model, which is what the per-entity slot
# is for -- but their first animations are one frame long, so a frame index
# cannot be seen there at all. Room 0 has Lara, whose first animation is a
# walk cycle, and nobody else.
CROWD = dict(room=14, x=22016, y=6152, z=-17456, angle=0)
LARA  = dict(room=0,  x=9728,  y=2560, z=5200,   angle=128)

FB = 0x1000
LEVEL_BANK = 0x43           # LEVEL_BANK_BASE in build/texbank.inc
HOST_OFFSET = None         # FX3 multi-level cartridges relocate host tables
FRAME_BYTES = gsusim.FRAME_SIZE


def render(rom, view, phase_of, anim_of=lambda i: 0, weapons=None, look=None):
    gsu = GSU(rom, scbr=0x40 if FX3 else 4, r3=FB, pbr=0x41,
              entry=0x100 if FX3 else 0)
    # Positions from the level, as the interpreter's own seeding does, then
    # the animation and frame overridden per entity -- which is the thing
    # being tested.
    gsusim.seed_items(gsu, 0, LEVEL_BANK, host_offset=HOST_OFFSET)
    if weapons is not None:
        # State word followed by the two independent 12-byte arm records.
        import struct
        for i, word in enumerate(weapons):
            struct.pack_into('<H', gsu.ram, 0xF710 + i * 2, word & 0xFFFF)
    if look is not None:
        import struct
        for i, word in enumerate(look):
            struct.pack_into('<H', gsu.ram, 0xF72A + i * 2, word & 0xFFFF)
    # Every entity, Lara included: the host hides her because she is at the
    # camera, and that is the host's policy rather than the renderer's. What
    # is being tested here is the renderer, and she is the only thing in room
    # 0 with an animation long enough to see a frame index in.
    for i in range(gsusim.GS_ITEM_MAX):
        p = gsusim.GS_ITEM + i * gsusim.GS_ITEM_SIZE
        gsu.ram[p] = anim_of(i) & 0xFF
        gsu.ram[p + 1] = phase_of(i) & 0xFF
    gsu.r[6] = view["room"] & 0xFFFF
    gsu.r[7] = view["x"] & 0xFFFF
    gsu.r[8] = view["angle"] & 0xFFFF
    gsu.r[9] = view["z"] & 0xFFFF
    gsu.r[10] = view["y"] & 0xFFFF
    # FrameStart now takes the target height in R11. These original pose
    # views are level (no pitch); leaving R11 at zero tilts toward world Y=0
    # and crops Lara at the bottom instead of checking her complete pose.
    gsu.r[11] = view["y"] & 0xFFFF
    if not gsu.run():
        sys.exit("GSU did not reach STOP within the step limit")
    if FX3:
        # Normalize the FX3 crop to the same 256x160 coordinate system for
        # inspection. Its near-plane/rendering path has separate pose records
        # below; discarding the shade planes does not make it the 4bpp renderer.
        px = gsusim.decode_frame(gsu.ram, 0x10000)
        normal = bytearray(0x5000)
        for y, row in enumerate(px):
            for x, colour in enumerate(row):
                dx, dy = x + 20, y + 8
                a = ((dx // 8) * 20 + dy // 8) * 32 + (dy & 7) * 2
                for p in range(4):
                    if colour & (1 << p):
                        normal[a + (p // 2) * 16 + (p & 1)] |= 1 << (7 - (dx & 7))
        return bytes(normal), gsu.plots
    return bytes(gsu.ram[FB:FB + FRAME_BYTES]), gsu.plots


def differ(a, b):
    return sum(1 for x, y in zip(a, b) if x != y)


def main():
    global HOST_OFFSET
    rom_path = sys.argv[1] if len(sys.argv) > 1 else "build/openlara.sfc"
    rom = open(rom_path, "rb").read()
    levels = gsusim.level_table(rom_path) if FX3 else []
    if levels:
        HOST_OFFSET = levels[0][0]  # the fixed views below exercise LEVEL1

    print("animcheck: the per-entity animation channel")
    failures = 0

    crowd, plots = render(rom, CROWD, lambda i: 0)
    if plots == 0:
        print("  FAIL nothing drew in room 14 at all")
        return 1
    print(f"  room 14: {plots} pixels with every entity on animation 0")

    # Per entity, through the animation byte rather than the frame: room 14's
    # first animations are one frame long, so a frame spread is invisible
    # there however well the slots work.
    # The narrower FX3 frustum crops the odd-index animated subject out of
    # this scene. Use the even-index subset there, explicitly keeping slot
    # zero fixed so an accidental "read slot zero for everything" still fails.
    subset = (lambda i: int(i != 0 and (i & 1) == 0)) if FX3 else (lambda i: i & 1)
    alternating, _ = render(rom, CROWD, lambda i: 0, subset)
    n = differ(crowd, alternating)
    print(f"  every other entity on animation 1:  {n} bytes differ")
    if n == 0:
        print("  FAIL the renderer is not reading a slot per entity")
        failures += 1

    other, _ = render(rom, CROWD, lambda i: 0, lambda i: 1)
    n = differ(crowd, other)
    print(f"  animation 0 against animation 1:    {n} bytes differ")
    if n == 0:
        print("  FAIL the animation index is not choosing an animation")
        failures += 1

    # And the frame within an animation, which needs something that has more
    # than one. Lara's first animation is a walk.
    lara0, plots = render(rom, LARA, lambda i: 0)
    lara3, _ = render(rom, LARA, lambda i: 3)
    n = differ(lara0, lara3)
    print(f"  room 0: frame 0 against frame 3:    {n} bytes differ")
    if n == 0:
        print("  FAIL the frame index is not choosing a frame")
        failures += 1

    # A byte-for-byte interpreter/cartridge comparison can prove that both
    # execute the same renderer and still miss a shared packer bug.  These
    # masks isolate Lara from the room behind her in a close front view and
    # lock representative run, idle and roll silhouettes after comparing
    # them visually against the TR model.  In particular they retain the
    # flat-coloured faces the GBA packer labels FACE_TYPE_F; dropping those
    # used to leave holes between her limb meshes while every other oracle
    # stayed green.
    #
    # Each renderer needs its own records. The FX3 poses were reviewed with
    # R11 initialized (the full figure, not the accidentally pitched/cropped
    # view), against the unchanged A071 GSU program and packed LEVEL1 assets.
    # The 4bpp and Mode 7 records remain unchanged.
    hidden, _ = render(rom, LARA, lambda i: 0,
                       lambda i: 0xFF if i == 0 else 0)
    if FX3:
        visuals = (
            ("run 0", 0,   0, 0x01B70DBE, 471),
            ("run 3", 0,   3, 0x8D40ADA1, 715),
            ("stand", 103, 0, 0x0C85336B, 640),
            ("roll", 146, 2, 0x9110A2AC, 581),
        )
    elif MODE7:
        visuals = (
            ("run 0", 0,   0, 0x0328B982, 180),
            ("run 3", 0,   3, 0x5CB44B48, 195),
            ("stand", 103, 0, 0x72672539, 246),
            ("roll", 146, 2, 0x366D2485, 111),
        )
    else:
        visuals = (
            ("run 0", 0,   0, 0x90D5FB51, 474),
            ("run 3", 0,   3, 0x1C60ED1E, 496),
            ("stand", 103, 0, 0x8FA14800, 641),
            ("roll", 146, 2, 0x9903326E, 303),
        )
    for name, anim, frame, expected_crc, expected_bytes in visuals:
        image, _ = render(rom, LARA, lambda i, f=frame: f,
                          lambda i, a=anim: a if i == 0 else 0)
        mask = bytes(a ^ b for a, b in zip(image, hidden))
        crc = zlib.crc32(mask)
        changed = sum(v != 0 for v in mask)
        print(f"  Lara {name:5s}: {changed:3d} bytes, CRC {crc:08x}")
        if crc != expected_crc or changed != expected_bytes:
            print(f"  FAIL Lara's {name} silhouette/colour coverage changed")
            failures += 1

    # The full node walk must consume the weapon channel, not just the
    # standalone helpers tested by weaponcheck. Hold the body pose fixed.
    lower = [0, 0, 0, 0, 0, 1]
    raised = [0, 4, 0, 8192, 1, 1]
    right, _ = render(rom, LARA, lambda i: 0, lambda i: 103 if i == 0 else 0,
                      weapons=[4] + raised + lower)
    left, _ = render(rom, LARA, lambda i: 0, lambda i: 103 if i == 0 else 0,
                     weapons=[4] + lower + raised)
    down, _ = render(rom, LARA, lambda i: 0, lambda i: 103 if i == 0 else 0,
                     weapons=[4] + lower + lower)
    if not differ(right, left) or not differ(right, down) or not differ(left, down):
        print("  FAIL independent pistol poses did not reach the picture")
        failures += 1
    else:
        print("  right/left pistol poses change the picture independently")

    # The full hierarchy must consume both look records after the body's
    # animation. Include armed and holstered poses to catch state gating and
    # the scratch pointer shared with the next shoulder's basis helper.
    for state, arm in ((0, lower), (4, raised)):
        pictures = []
        for angles in ((0, 0, 0, 0), (4096, 8192, 0, 0),
                       (0, 0, 4096, 8192), (4096, 8192, 4096, 8192)):
            picture, _ = render(rom, LARA, lambda i: 0, lambda i: 103 if i == 0 else 0,
                                weapons=[state] + arm + arm, look=angles)
            pictures.append(picture)
        if any(not differ(a, b) for i, a in enumerate(pictures) for b in pictures[i + 1:]):
            print(f"  FAIL torso/head look did not independently change weapon state {state}")
            failures += 1
        else:
            print(f"  torso/head look changes the picture independently, weapon state {state}")

    # Neither index is trusted on the GSU: out of range has to clamp, because
    # a frame address off the end reads a mesh pointer out of unrelated bytes.
    _, plots = render(rom, CROWD, lambda i: 250, lambda i: 250)
    print(f"  animation 250 frame 250 draws {plots} pixels")
    if plots == 0:
        print("  FAIL an out-of-range animation drew nothing")
        failures += 1

    if failures:
        print(f"  {failures} failure(s)")
        return 1
    print("  all clear")
    return 0


if __name__ == "__main__":
    sys.exit(main())
