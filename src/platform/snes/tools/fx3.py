"""FX3 v1.0 framebuffer geometry and the host's packed Mode 3 VRAM layout.

Firmware covers 27 x 18 tiles inside a 32 x 20, 8bpp GSU bitmap at $710000.
The CPU strips the unused two rows per column during DMA; two packed frames
then fit below one tilemap in the SNES's 64 KB of VRAM.
"""
import struct
import sys

WIDTH, HEIGHT = 216, 144
COLS, ROWS = WIDTH // 8, HEIGHT // 8
TILE_BYTES = 64
COL_BYTES = ROWS * TILE_BYTES
COL_STRIDE = 20 * TILE_BYTES
FRAME_SIZE = COLS * COL_BYTES
FB_BASE = 0x10000
TILES = COLS * ROWS
BLANK_TILE = TILES * 2
MAP_BASE = 0xF800  # VRAM byte address
CLEAR_COLOUR = 0x81  # FX3 emulator/firmware command reference


def tilemap(buffer):
    if buffer not in (0, 1):
        raise ValueError("frame buffer must be 0 or 1")
    return b"".join(struct.pack("<H", buffer * TILES + x * ROWS + y
                                if x < COLS and y < ROWS else BLANK_TILE)
                    for y in range(32) for x in range(32))


def packed_frame(ram):
    return b"".join(ram[FB_BASE + x * COL_STRIDE:
                        FB_BASE + x * COL_STRIDE + COL_BYTES]
                    for x in range(COLS))


if __name__ == "__main__":
    with open(sys.argv[1], "wb") as f:
        f.write(tilemap(0) + tilemap(1))
