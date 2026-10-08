"""Hardware-layout invariants for the FX3 variant (no game assets needed)."""
import struct
import unittest
from unittest.mock import patch

import fx3
import gsusim


class FX3LayoutTests(unittest.TestCase):
    def test_fx3_rom_window_is_three_megabytes_not_two_or_four(self):
        with patch.object(gsusim, "FX3", True):
            self.assertEqual(gsusim.rom_addr(0x60, 0), 0x200000)
            self.assertEqual(gsusim.rom_addr(0x6F, 65535), 0x2FFFFF)
            self.assertIsNone(gsusim.rom_addr(0x70, 0))
            self.assertIsNone(gsusim.rom_addr(0xF0, 0))  # CPU-only alias
        with patch.object(gsusim, "FX3", False):
            self.assertEqual(gsusim.rom_addr(0x5F, 65535), 0x1FFFFF)
            self.assertIsNone(gsusim.rom_addr(0x60, 0))

    def test_double_buffer_and_blank_fit_below_tilemap(self):
        self.assertEqual(fx3.FRAME_SIZE, 31104)
        self.assertLessEqual((fx3.BLANK_TILE + 1) * 64, fx3.MAP_BASE)
        self.assertEqual(fx3.MAP_BASE + 2048, 65536)
        for buffer in (0, 1):
            entries = struct.unpack("<1024H", fx3.tilemap(buffer))
            active = []
            for y in range(32):
                for x in range(32):
                    tile = entries[y * 32 + x]
                    if x < 27 and y < 18:
                        self.assertEqual(tile, buffer * 486 + x * 18 + y)
                        active.append(tile)
                    else:
                        self.assertEqual(tile, 972)
            self.assertEqual(len(set(active)), 486)
        with self.assertRaises(ValueError):
            fx3.tilemap(2)

    def test_upload_removes_only_the_unused_column_rows(self):
        ram = bytearray(0x20000)
        for col in range(27):
            start = fx3.FB_BASE + col * fx3.COL_STRIDE
            ram[start:start + 1152] = bytes([col]) * 1152
            ram[start + 1152:start + 1280] = b"\xff" * 128
        self.assertEqual(fx3.packed_frame(ram), b"".join(bytes([x]) * 1152 for x in range(27)))

    def test_seven_upload_fields_stay_within_vblank_budget(self):
        columns = [4] * 6 + [3]
        self.assertEqual(sum(columns), 27)
        uploads = [c * fx3.COL_BYTES for c in columns]
        uploads[-1] += 2048
        self.assertEqual(max(uploads), 5504)
        self.assertLess(max(uploads), 6144)

    def test_clear_commands_touch_only_their_nine_column_strip(self):
        for command in range(3, 6):
            with self.subTest(command=command), patch.object(gsusim, "FX3", True):
                gsu = gsusim.GSU(bytes([0xA0, command, 0x70, 0]), pbr=0x40)
                gsu.ram[:] = b"\x5a" * len(gsu.ram)
                self.assertTrue(gsu.run())
                expected = bytearray(b"\x5a" * len(gsu.ram))
                for x in range((command - 3) * 9, (command - 2) * 9):
                    for y in range(18):
                        start = fx3.FB_BASE + x * fx3.COL_STRIDE + y * 64
                        for p in range(8):
                            for row in range(8):
                                expected[start + p // 2 * 16 + row * 2 + (p & 1)] = 255 if p in (0, 7) else 0
                self.assertEqual(gsu.ram, expected)

    def test_eight_bit_plot_preserves_neighbour_pixels_and_all_planes(self):
        with patch.object(gsusim, "FX3", True):
            gsu = gsusim.GSU(b"\0", pbr=0x40, scbr=0x40)
            gsu.icycles = 0
            gsu.por = 1
            gsu.r[1], gsu.r[2] = 215, 143
            gsu.colr = 0xA5
            gsu.plot()
            base = fx3.FB_BASE + 26 * 1280 + 17 * 64 + 14
            self.assertEqual(gsu.r[1], 216)
            for p in range(8):
                self.assertEqual(gsu.ram[base + p // 2 * 16 + (p & 1)], (0xA5 >> p) & 1)


if __name__ == "__main__":
    unittest.main()
