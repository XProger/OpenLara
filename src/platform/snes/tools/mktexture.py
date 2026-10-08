#!/usr/bin/env python3
"""Generate a placeholder 256x256 texture page for the GSU.

A page is 8 bits per texel and occupies a full 64 KB GSU ROM bank, which
is what makes the texel address in the rasterizer's inner loop simply
(v << 8) | u -- one MERGE instruction, no arithmetic.

Texel values are 0-15 because the frame buffer is 4bpp; PLOT only looks at the
low nibble of COLR in 16-colour mode.

The pattern is chosen to make UV mistakes obvious rather than to look nice:
a coarse checkerboard so orientation and wrapping show up, grid lines every 32
texels so affine distortion is visible, and a distinct border so a page read
running off its edge is unmistakable.
"""

import sys

WIDTH = 256
HEIGHT = 256


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "texture.bin"
    data = bytearray(WIDTH * HEIGHT)

    for v in range(HEIGHT):
        for u in range(WIDTH):
            if u < 2 or v < 2 or u >= WIDTH - 2 or v >= HEIGHT - 2:
                c = 10                      # border
            elif u % 32 == 0 or v % 32 == 0:
                c = 15                      # grid
            else:
                # coarse checkerboard, two flat colours, plus a marker block
                # in the top-left cell so orientation is unambiguous
                c = 3 if (((u >> 5) + (v >> 5)) & 1) else 8
                if u < 30 and v < 30:
                    c = 13
            data[v * WIDTH + u] = c

    with open(out, "wb") as f:
        f.write(bytes(data))
    print(f"{out}: {WIDTH}x{HEIGHT}, {len(data)} bytes")


if __name__ == "__main__":
    main()
