#!/usr/bin/env python3
"""The near projection (FX3), checked against its arithmetic, on real frames.

Models a view with tools/gsusim.py and watches the three routines at the end
of mesh.gsu that project what lies between the clip plane and the table's
first step: TM_Near (the vertex loop's arm), PC_Near (ProjectClip's) and
ShearNear (the frame's pitch). Every point they hand back is compared with

    clamp(c * T[z >> 1] >> 10, -8192, 8191) + centre,  T[i] = min(65536 / i, $7FFF)

bit for bit -- the arithmetic the routines claim -- and, for the record, with
the exact 128 c / z it stands for, on the screen (within 256 pixels of the
centre) and off it: z >> 1 drops a unit of depth on odd z, 1/z of the
offset, which is a pixel on the screen and tens of pixels thousands off it.

    python3 tools/nearcheck.py build_fx3/openlara.sfc [--views close]

Each view is modelled a second time with TM_Near's side bits switched off
(NK_MASK held at zero): they only let OTBuild drop faces nothing of which
reaches the clip rectangle, so the two frames must be the same bytes.

Exit status 1 on any mismatch or if no near point was seen at all.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gsusim  # noqa: E402

SCREEN_CX, SCREEN_CY = 108, 72          # gsu.inc, FX3
FR_CY = 0xEE
ZNEAR, ZMIN = 64, 512

# (name, cam x, cam y, cam z, target y, angle, room[, shadow]): LEVEL1's
# start, her standing where `make sim` puts her, the camera closing in from
# her west and from her back, and turned along the wall to her right. A
# shadow view seeds GS_SHADOW with a record for her (Shadow_publish's, for a
# 200-wide box) and puts the camera low enough to see it: ShadowRender's
# corners at 64..511 are SP_Near's.
VIEWS = {
    "close": [
        ("west-400", 9728 - 400, 2400, 3584, 2491, 64, 0),
        ("west-300", 9728 - 300, 2400, 3584, 2491, 64, 0),
        ("west-200", 9728 - 200, 2400, 3584, 2491, 64, 0),
        ("west-120", 9728 - 120, 2450, 3584, 2491, 64, 0),
        ("north-300", 9728, 2400, 3584 - 300, 2491, 0, 0),
        ("wall-graze", 8400, 2300, 3200, 2400, 64, 0),
        ("high-200", 9728 - 200, 2000, 3584, 2491, 64, 0),
        ("shadow-300", 9728 - 300, 2960, 3584, 2960, 64, 0, True),
        ("shadow-450", 9728 - 450, 2900, 3584, 2900, 64, 0, True),
    ],
}


def table(i):
    return 0x7FFF if i < 2 else min(65536 // i, 0x7FFF)


def scale(c, z):
    p = c * table(z >> 1)
    v = p >> 10                          # arithmetic: Python floors
    return max(-8192, min(8191, v))


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


NK_MASK = 0xF660                         # gsu.inc
GS_SHADOW = 0xF734
T_CX, T_CY, T_CZ = 0xA6, 0xA8, 0xAA      # mesh.gsu


def shadow_record(data):
    """GS_SHADOW as Shadow_publish writes it for her alone: the font's bank
    and dark remap (the level's FONT.BIN), slot 0, on the floor, a box 200
    across (5/32 of it either way)."""
    font = open(os.path.join(data, "FONT.BIN"), "rb").read()
    dark = font[62] | font[63] << 8
    return [1, font[58], dark, 0xF800, 0, 0, 0, 31, 31]


def check_view(rom, sym, view, level, cull=True, png=None, data=None):
    name, cx, cy, cz, ty, angle, room = view[:7]
    data = data or os.path.join(os.path.dirname(rom), "data")
    shadow = shadow_record(data) if len(view) > 7 and view[7] else None
    labels = gsusim.read_symbols(sym)
    addr = {n: a for n, a in labels.items()} if isinstance(labels, dict) else None
    if addr is None:                     # read_symbols returns [(addr, name)]
        addr = {n: a for a, n in labels}
    want = {n: addr[n] for n in ("TN_Project", "TM_Store", "PC_Near", "PC_Stored",
                                 "ShearNear", "SN_Done", "TM_Near", "SP_Near", "SP_Out")}
    log = {"tm": [], "pc": [], "shear": [], "sh": [], "flag": 0}

    class Hooked(gsusim.GSU):
        def step(self):
            pc = self.pc
            if shadow and self.steps == 0:
                for k, v in enumerate(shadow):
                    self.write_ram_word(GS_SHADOW + k * 2, v)
            if pc == want["SP_Near"]:
                self._sp = tuple(s16(self.read_ram_word(a)) for a in (T_CX, T_CY, T_CZ))
            elif pc == want["SP_Out"] and getattr(self, "_sp", None):
                x, y, z = self._sp
                log["sh"].append((x, y, z, s16(self.r[1]), s16(self.r[2]),
                                  s16(self.read_ram_word(FR_CY))))
                self._sp = None
            if pc == want["TN_Project"]:
                self._tm = (s16(self.r[9]), s16(self.r[10]), s16(self.r[11]))
            elif pc == want["TM_Store"] and getattr(self, "_tm", None):
                x, y, z = self._tm
                fr = s16(self.read_ram_word(FR_CY))
                log["tm"].append((x, y, z, s16(self.r[5]), s16(self.r[8]), fr))
                self._tm = None
            elif pc == want["PC_Near"]:
                self._pc = (s16(self.r[9]), s16(self.r[11]), s16(self.r[3]), self.r[8])
            elif pc == want["PC_Stored"] and getattr(self, "_pc", None):
                x, y, z, link = self._pc
                fr = s16(self.read_ram_word(FR_CY))
                log["pc"].append((x, y, z, s16(self.read_ram_word(link)),
                                  s16(self.read_ram_word(link + 2)), fr))
                self._pc = None
            elif pc == want["ShearNear"]:
                self._sh = s16(self.r[5])
            elif pc == want["SN_Done"] and getattr(self, "_sh", None) is not None:
                log["shear"].append((self._sh, s16(self.read_ram_word(FR_CY))))
                self._sh = None
            if not cull and pc == want["TM_Near"]:
                self.write_ram_word(NK_MASK, 0)
            return super().step()

    real = gsusim.GSU
    gsusim.GSU = Hooked
    argv = sys.argv
    out = os.devnull
    stats = os.path.join(os.environ.get("TMPDIR", "/tmp"),
                         f"nearcheck.{os.getpid()}.stats")
    sys.argv = ["gsusim.py", rom, "--scbr", "4", "--fb", "0x1000", "--fx3",
                "--level", str(level), "--cam-x", str(cx), "--cam-y", str(cy),
                "--cam-z", str(cz), "--cam-ty", str(ty), "--angle", str(angle),
                "--room", str(room), "--probe-x", "9728", "--probe-y", "2560",
                "--probe-z", "3584", "-o", png or out, "--stats", stats]
    if png:
        sys.argv += ["--palette", os.path.join(data, "PALETTE.BIN")]
    try:
        stdout = sys.stdout
        sys.stdout = open(os.devnull, "w")
        try:
            gsusim.main()
        finally:
            sys.stdout.close()
            sys.stdout = stdout
    finally:
        gsusim.GSU = real
        sys.argv = argv
    with open(stats) as f:
        log["crc"] = dict(l.strip().split("=", 1) for l in f if "=" in l)["crc32"]
    os.remove(stats)
    return log


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("--sym", default=None,
                    help="gsu_sym.sym (default: beside the ROM)")
    ap.add_argument("--views", default="close")
    ap.add_argument("--level", type=int, default=0)
    ap.add_argument("--png", metavar="DIR", help="also write each view's frame here")
    ap.add_argument("--data", help="the level's packed data (FONT.BIN, PALETTE.BIN); "
                                   "default: data/ beside the ROM")
    args = ap.parse_args()
    sym = args.sym or os.path.join(os.path.dirname(args.rom), "gsu_sym.sym")

    bad = 0
    seen = 0
    for view in VIEWS[args.views]:
        png = os.path.join(args.png, view[0] + ".png") if args.png else None
        log = check_view(args.rom, sym, view, args.level, png=png, data=args.data)
        bare = check_view(args.rom, sym, view, args.level, cull=False, data=args.data)
        if bare["crc"] != log["crc"]:
            bad += 1
            print(f"  CULL {view[0]}: frame {log['crc']} with the side bits, "
                  f"{bare['crc']} without")
        worst_on = worst_off = 0.0
        clamps = 0
        for kind in ("tm", "pc", "sh"):
            for x, y, z, sx, sy, fr in log[kind]:
                zz = max(z, ZNEAR) if kind == "pc" else z
                ex, ey = scale(x, zz) + SCREEN_CX, scale(y, zz) + fr
                if (sx, sy) != (ex, ey):
                    bad += 1
                    if bad <= 10:
                        print(f"  MISMATCH {view[0]} {kind} c=({x},{y},{z}) "
                              f"got ({sx},{sy}) want ({ex},{ey})")
                if abs(ex - SCREEN_CX) >= 8191 or abs(ey - fr) >= 8191:
                    clamps += 1
                    continue
                for c, s, centre in ((x, sx, SCREEN_CX), (y, sy, fr)):
                    exact = 128 * c / zz
                    err = abs(s - centre - exact)
                    if abs(exact) <= 256:
                        worst_on = max(worst_on, err)
                    else:
                        worst_off = max(worst_off, err / abs(exact))
        for zc, fr in log["shear"]:
            if zc >= ZNEAR:
                seen += 1
        seen += len(log["tm"]) + len(log["pc"]) + len(log["sh"])
        if len(view) > 7 and view[7] and not log["sh"]:
            bad += 1
            print(f"  SHADOW {view[0]}: no corner went through SP_Near")
        print(f"{view[0]:<11} TM_Near {len(log['tm']):4}  PC_Near {len(log['pc']):4}  "
              f"SP_Near {len(log['sh']):2}  shear {len(log['shear'])}  clamped {clamps:3}  "
              f"vs 128c/z: on screen {worst_on:4.2f} px, off it "
              f"{worst_off * 100:4.2f}%")
    if bad:
        print(f"{bad} near points or frames differ")
        return 1
    if not seen:
        print("no near point in any view: the routines never ran")
        return 1
    print("near projection: every point matches its arithmetic; "
          "the side bits change no frame")
    return 0


if __name__ == "__main__":
    sys.exit(main())
