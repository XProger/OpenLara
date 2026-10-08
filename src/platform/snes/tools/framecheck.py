#!/usr/bin/env python3
"""Hold the opening frame's modelled cost against a recorded baseline.

`make sim` interprets the GSU program and models its cycles; those numbers
scrolled past and were gone. A renderer change that made the frame ten per
cent dearer was found weeks later, by someone wondering why the frame rate
had moved. So `make check` now compares what the interpreter measured with
what was measured last time -- and refuses when the picture or its price has
moved, until `make baseline` records the new numbers. The baseline file is
tracked, so the change shows up where it should: in the diff.

The picture itself is verified elsewhere, byte for byte against the emulator;
this only notices that it changed.

Usage:
    framecheck.py STATS BASELINE [--tolerance PCT]   compare, fail on drift
    framecheck.py STATS BASELINE --record            accept what STATS says
"""

import argparse
import os
import sys

GSU_HZ = 21_400_000          # what tools/gsusim.py models


def read_kv(path):
    values = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            k, _, v = line.partition("=")
            values[k.strip()] = v.strip()
    return values


def describe(stats):
    cycles = int(stats["cycles"])
    if stats.get("target") == "fx3":
        return (f"{cycles:,} reference GSU cycles (FX3 hardware timing unmodelled), "
                f"cache {stats.get('cache_hit', '?')}%, crc {stats.get('crc32', '?')}")
    return (f"{cycles:,} GSU cycles ({GSU_HZ / cycles:.2f} fps modelled), "
            f"cache {stats.get('cache_hit', '?')}%, crc {stats.get('crc32', '?')}")


def record(stats, path):
    with open(path, "w") as f:
        f.write("# The opening frame, as tools/gsusim.py models it. Written by\n"
                "# `make baseline`, compared by `make check`; a change here is\n"
                "# a renderer change that cost or saved something, and belongs\n"
                "# in the same commit as the code that made it.\n")
        for k in ("target", "cycles", "instructions", "plots", "cache_hit", "crc32"):
            if k in stats:
                f.write(f"{k}={stats[k]}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("stats", help="key=value file written by gsusim.py --stats")
    ap.add_argument("baseline", help="the tracked record to compare against")
    ap.add_argument("--record", action="store_true",
                    help="overwrite the baseline with the stats")
    ap.add_argument("--tolerance", type=float, default=2.0,
                    help="percent the cycle count may move before this fails")
    args = ap.parse_args()

    stats = read_kv(args.stats)
    if "cycles" not in stats:
        sys.exit(f"{args.stats}: no cycles= line; was gsusim.py run with --stats?")

    if args.record or not os.path.exists(args.baseline):
        record(stats, args.baseline)
        why = "recorded" if args.record else "no baseline yet; recorded"
        print(f"[openlara] frame model: {describe(stats)}  -- {why} in {args.baseline}")
        return

    base = read_kv(args.baseline)
    cycles = int(stats["cycles"])
    was = int(base["cycles"])
    delta = 100.0 * (cycles - was) / was
    crc_moved = stats.get("crc32") != base.get("crc32")

    verdict = f"{delta:+.2f}% against {args.baseline}"
    if crc_moved:
        verdict += f", picture changed (crc was {base.get('crc32')})"
    print(f"[openlara] frame model: {describe(stats)}  -- {verdict}")

    if abs(delta) > args.tolerance or crc_moved:
        sys.exit(f"[openlara] the opening frame moved beyond the recorded "
                 f"baseline; if the renderer change is intended, `make baseline` "
                 f"and commit {args.baseline} with it")


if __name__ == "__main__":
    main()
