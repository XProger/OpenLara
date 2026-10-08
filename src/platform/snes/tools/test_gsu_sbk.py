"""SBK address/flag semantics used by the rasterizer's accumulator updates.

SNES Development Manual Book II, Super FX 9.79: store Sreg to the last
register/RAM transfer address; preserve arithmetic flags. Pixel/ROM accesses
do not select this address, and odd word addresses pair backward.
"""
import unittest

import gsusim


class StoreBackTests(unittest.TestCase):
    def test_every_transfer_form_selects_the_store_back_address(self):
        for name, code, address in [
            ("LDW", [0x41], 0x121),
            ("LDB", [0x3D, 0x41], 0x121),
            ("STW", [0x31], 0x121),
            ("STB", [0x3D, 0x31], 0x121),
            ("LM", [0x3D, 0xF0, 0x21, 0x01], 0x121),
            ("SM", [0x3E, 0xF0, 0x21, 0x01], 0x121),
            ("LMS", [0x3D, 0xA0, 0x90], 0x120),
            ("SMS", [0x3E, 0xA0, 0x90], 0x120),
        ]:
            for bank in (0, 1):
                with self.subTest(transfer=name, bank=bank):
                    # Select address, overwrite the pointer, then use a
                    # prefixed source for SBK rather than the default R0.
                    prefix = code + [0xF1, 0x00, 0x02]
                    gsu = gsusim.GSU(bytes(prefix + [0xB5, 0x90, 0]), pbr=0x40)
                    gsu.ram[:] = b"\xA6" * len(gsu.ram)
                    gsu.rambr = bank
                    gsu.r[1] = 0x121
                    gsu.r[5] = 0x1234
                    gsu.z, gsu.s, gsu.cy, gsu.ov = True, False, True, True
                    while gsu.pc < len(prefix):
                        gsu.step()
                    expected = bytearray(gsu.ram)
                    expected[(bank << 16) | address] = 0x34
                    expected[(bank << 16) | (address ^ 1)] = 0x12
                    self.assertTrue(gsu.run())
                    self.assertEqual(gsu.ram, expected)
                    self.assertEqual((gsu.z, gsu.s, gsu.cy, gsu.ov), (True, False, True, True))
                    self.assertEqual((gsu.sreg, gsu.dreg, gsu.b), (0, 0, False))

    def test_fractional_carry_survives_store_back_and_next_load(self):
        # LMS R5,step; LMS R0,fraction; ADD R5; SBK;
        # LMS R0,integer; ADC #0; SBK. The fraction wraps, carrying once.
        program = [0x3D, 0xA5, 0x10, 0x3D, 0xA0, 0x11, 0x55, 0x90,
                   0x3D, 0xA0, 0x12, 0x3F, 0x50, 0x90, 0]
        gsu = gsusim.GSU(bytes(program), pbr=0x40)
        gsu.ram[0x20:0x26] = bytes.fromhex("0200 ffff 3412")
        self.assertTrue(gsu.run())
        self.assertEqual(gsu.ram[0x20:0x26], bytes.fromhex("0200 0100 3512"))

    def test_rom_and_plot_accesses_do_not_redirect_store_back(self):
        # LMS R0,$120; IWT R14,$40; GETB; PLOT; IWT R0,$1234; SBK.
        program = [0x3D, 0xA0, 0x90, 0xFE, 0x40, 0, 0xEF, 0x4C,
                   0xF0, 0x34, 0x12, 0x90, 0]
        gsu = gsusim.GSU(bytes(program), pbr=0x40, scbr=4)
        gsu.colr = 5
        self.assertTrue(gsu.run())
        self.assertEqual(gsu.ram[0x120:0x122], b"\x34\x12")


if __name__ == "__main__":
    unittest.main()
