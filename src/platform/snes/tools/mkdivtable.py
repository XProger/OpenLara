#!/usr/bin/env python3
"""Emit the reciprocal table the GSU rasterizer uses instead of a divide.

Same shape as divTable in src/fixed/common.cpp -- 65536 / i -- but only the
first 256 entries, since a polygon edge on a 160-line screen never spans more
than that. 512 bytes of ROM.

Entries are clamped to $7FFF rather than $FFFF. Every use on the GSU feeds
LMULT or FMULT, which are signed, so a $8000-and-up entry would come back
negative: divTable[2] alone would flip the slope of every two-line edge. The
clamp costs 1 part in 65536 on entry 2 and is only visible at all for h < 3,
where the step is either unused or applies to a single pixel.
"""

import struct
import sys

ENTRIES = 256


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else 'divtable.bin'
    data = bytearray()
    for i in range(ENTRIES):
        v = 0x7FFF if i < 2 else min(65536 // i, 0x7FFF)
        data += struct.pack('<H', v)
    with open(out, 'wb') as f:
        f.write(bytes(data))
    print(f"{out}: {ENTRIES} entries, {len(data)} bytes")


if __name__ == '__main__':
    main()
