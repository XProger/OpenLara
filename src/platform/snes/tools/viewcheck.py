#!/usr/bin/env python3
"""Model a set of views on two ROMs and say whether each picture changed.

The frame baseline holds one view, the opening one. A renderer change is
verified on more than that by modelling the same views on the ROM from
before the change and the one after, comparing the raw frame buffers
byte for byte, and reporting the cycles of each:

    python3 tools/viewcheck.py before.sfc build/openlara.sfc
    python3 tools/viewcheck.py --mode7 before-m7.sfc build_m7/openlara.sfc
    python3 tools/viewcheck.py --fx3 before-fx3.sfc build_fx3/openlara.sfc

Add views to VIEWS as the levels and the scenes grow; the numbers are the
runner's (world x, y, z, angle in the GSU's 256-to-the-turn units, room).
"""

import hashlib
import os
import re
import subprocess
import sys
import tempfile

VIEWS = [
    ("opening",       8215, 2228,  3584,  64,  0),
    ("entrance-side", 8192, 2493,  3584,  64,  0),
    ("start-back",    9728, 2560,  3584, 192,  0),
    ("start-fwd",     9728, 2560,  3584,   0,  0),
    ("start-turn",    9728, 2560,  3584, 128,  0),
    ("room14",       22016, 6144, 48080,   0, 14),
    ("room9-switch", 49664, 7168, 58206,   0,  9),
    ("bridges",      26624, 4352, 52736,  96, 14),
    ("room2-bats",   11776, -1280, 40448, 32,  2),
]


def model(rom, view, raw, palette, mode7, fx3=False):
    name, x, y, z, a, room = view
    mode = ["--fx3"] if fx3 else ["--mode7"] if mode7 else []
    # Match Makefile's settled opening camera and Lara placement. Other
    # views retain their horizontal camera and camera-position probe.
    camera = (["--cam-ty", "2491"] if name == "opening" else [])
    probe = (9728, 2560, 3584) if name == "opening" else (x, y, z)
    shade = raw + ".shade"
    r = subprocess.run(
        [sys.executable, "tools/gsusim.py", rom, "--scbr", "4", "--fb", "0x1000",
         *mode, *camera, "--cam-x", str(x), "--cam-y", str(y), "--cam-z", str(z),
         "--angle", str(a), "--room", str(room), "--tick", "0",
         "--probe-x", str(probe[0]), "--probe-y", str(probe[1]), "--probe-z", str(probe[2]),
         "--raw", raw, "--raw-shade", shade,
         "--palette", palette, "-o", os.devnull],
        capture_output=True, text=True)
    m = re.search(r"(\d+)(?: reference GSU)? cycles", r.stdout)
    px = re.search(r"(\d+) pixels plotted", r.stdout)
    if r.returncode or not m or not px:
        sys.exit(f"{name} on {rom}: {r.stdout}{r.stderr}")
    # A 4bpp frame also depends on the tile-map palette bytes. Comparing
    # only its pixels silently accepted lighting regressions.
    digest = hashlib.sha256()
    with open(raw, "rb") as f:
        digest.update(f.read())
    if not mode7 and not fx3:
        with open(shade, "rb") as f:
            digest.update(f.read())
    return (int(m.group(1)), int(px.group(1)),
            digest.hexdigest())


def main():
    mode7 = "--mode7" in sys.argv
    fx3 = "--fx3" in sys.argv
    if mode7:
        sys.argv.remove("--mode7")
    if fx3:
        sys.argv.remove("--fx3")
    if mode7 and fx3:
        sys.exit("--mode7 and --fx3 select different framebuffer formats")
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    before, after = sys.argv[1:]
    palette = os.path.join(os.path.dirname(after), "data", "PALETTE.BIN")
    changed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for view in VIEWS:
            b = model(before, view, os.path.join(tmp, "b.bin"), palette, mode7, fx3)
            n = model(after, view, os.path.join(tmp, "n.bin"), palette, mode7, fx3)
            same = "IDENTICAL" if b[2] == n[2] else "DIFFERS"
            changed += same == "DIFFERS"
            print(f"{view[0]:13s} {b[0]:>9} -> {n[0]:>9} cycles "
                  f"({100 * (n[0] - b[0]) / max(1, b[0]):+.1f}%), "
                  f"pixels {b[1]} -> {n[1]}  {same}")
    print(f"{len(VIEWS) - changed} of {len(VIEWS)} views identical")
    return int(changed != 0)


if __name__ == "__main__":
    sys.exit(main())
