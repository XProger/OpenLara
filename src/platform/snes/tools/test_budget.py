"""Link-time guards for FX3 FastROM and code sharing GSU-2 host/stack WRAM."""
import contextlib
import io
from pathlib import Path
import unittest
from unittest.mock import patch

import budget


class BudgetTests(unittest.TestCase):
    def run_budget(self, symbols, fx3=False, layout=True, pointers=True, staging=False, sectors=16104):
        if fx3 and pointers:
            for name, begin, end in [('low', 0x28, 0x30), ('middle', 0x58, 0x60), ('high', 0xC0, 0xFA)]:
                symbols += (f"00:{begin:04x} SECTIONSTART_host_ptrs_{name}\n"
                            f"00:{end:04x} SECTIONEND_host_ptrs_{name}\n")
        def read(path, *args, **kwargs):
            self.assertNotEqual(str(path), 'Makefile', 'use the resolved configuration, not conditional assignments')
            return io.StringIO(symbols)

        config = ".DEFINE FX3 1\n" if fx3 else ""
        if staging:
            config += ".DEFINE FX3_STAGE 1\n"
        if layout:
            config += ("; C_LAYOUT math 61 0x7E 8\n; C_LAYOUT enemy 62 0x7E 7\n"
                       "; C_LAYOUT item 4 0x7F 4\n; C_LAYOUT bat 5 0x7F 6\n")

        out = io.StringIO()
        with patch("sys.argv", ["budget.py", "build_variant/openlara.sym"]), \
                patch("builtins.open", side_effect=read), \
                patch.object(Path, "read_text", lambda path: config if path.name == "config.inc"
                             else f".DEFINE HOST_SECTOR_SIZE {sectors}\n"), \
                contextlib.redirect_stdout(out):
            budget.main()
        return out.getvalue()

    def test_fx3_staging_is_checked_against_mutable_sector_extent(self):
        symbols = ("01:0412 HostCodeEnd\n"
                   "7f:8000 SECTIONSTART_fx3_frame_stage\n"
                   "7f:f980 SECTIONEND_fx3_frame_stage\n")
        report = self.run_budget(symbols, fx3=True, staging=True, sectors=32768)
        self.assertIn("31,104 bytes at $7F:8000-$F97F", report)
        with self.assertRaisesRegex(SystemExit, "mutable sectors overlap"):
            self.run_budget(symbols, fx3=True, staging=True, sectors=32769)
        with self.assertRaisesRegex(SystemExit, "staging must occupy exactly"):
            self.run_budget(symbols.replace("f980", "f981"), fx3=True, staging=True)

    def test_fx3_code_is_budgeted_in_fastrom_not_gsu2_wram_slots(self):
        report = self.run_budget("""01:0412 HostCodeEnd
87:8000 SECTIONSTART_.rodata.c_math
87:c000 SECTIONEND_.rodata.c_math
87:c000 SECTIONSTART_math_native
87:c109 SECTIONEND_math_native
88:8000 SECTIONSTART_.Enemy_updatetext_0x0.c_enemy
88:9000 SECTIONEND_.Enemy_updatetext_0x0.c_enemy
7e:2000 SECTIONSTART_.bss.c_enemy
7e:2100 SECTIONEND_.bss.c_enemy
""", fx3=True)
        self.assertIn("C ROM    $87:8000", report)
        self.assertIn("48640 bytes available", report)
        self.assertNotIn("C image  $7E", report)

    def test_fx3_rejects_accidentally_resident_c_and_native_code(self):
        for name in ['.rodata.c_math', 'math_native', 'nav_sector_native',
                     'nav_fx3_native', 'enemy_move_native', 'enemy_update_native', 'wolf_update_native', 'collision_fx3_native',
                     'bear_state_native', 'bear_update_native', 'raptor_state_native',
                     'raptor_update_native', 'contact_fx3_native', 'render_sync_native', 'host_tables_native',
                     'camera_fx3_native', 'future_controller_native']:
            with self.subTest(name=name):
                with self.assertRaisesRegex(SystemExit, "not in FX3 FastROM"):
                    self.run_budget(f"01:0412 HostCodeEnd\n"
                                    f"7e:0600 SECTIONSTART_{name}\n"
                                    f"7e:0700 SECTIONEND_{name}\n", fx3=True)

    def test_fx3_native_camera_must_fit_one_allocated_fastrom_code_bank(self):
        report = self.run_budget("01:0412 HostCodeEnd\n"
                                 "85:8000 SECTIONSTART_camera_fx3_native\n"
                                 "85:8700 SECTIONEND_camera_fx3_native\n", fx3=True)
        self.assertIn("C ROM    $85:8000        1792 used   30976 free", report)
        for begin, end in [('05:8000', '05:8700'), ('85:7f00', '85:8700'),
                           ('85:8000', '86:8700'), ('85:8700', '85:8000')]:
            with self.subTest(begin=begin, end=end):
                with self.assertRaisesRegex(SystemExit, "not in FX3 FastROM"):
                    self.run_budget("01:0412 HostCodeEnd\n"
                                    f"{begin} SECTIONSTART_camera_fx3_native\n"
                                    f"{end} SECTIONEND_camera_fx3_native\n", fx3=True)

    def test_fx3_native_vectors_are_fixed_header_data_not_engine_code(self):
        report = self.run_budget("01:0412 HostCodeEnd\n"
                                 "00:ffe4 SECTIONSTART_vectors_native\n"
                                 "00:fff0 SECTIONEND_vectors_native\n"
                                 "85:8000 SECTIONSTART_camera_fx3_native\n"
                                 "85:8700 SECTIONEND_camera_fx3_native\n", fx3=True)
        self.assertIn("C ROM    $85:8000        1792 used   30976 free", report)

    def test_fx3_table_pointers_must_fit_all_three_direct_page_gaps(self):
        with self.assertRaisesRegex(SystemExit, "direct-page gap"):
            self.run_budget("01:0412 HostCodeEnd\n", fx3=True, pointers=False)
        with self.assertRaisesRegex(SystemExit, "direct-page gap"):
            self.run_budget("01:0412 HostCodeEnd\n00:00c0 SECTIONSTART_host_ptrs_high\n"
                            "00:0102 SECTIONEND_host_ptrs_high\n", fx3=True, pointers=False)

    def test_gsu2_bear_native_code_cannot_escape_its_resident_region(self):
        for name in ['bear_state_native', 'bear_update_native']:
            with self.subTest(name=name):
                with self.assertRaisesRegex(SystemExit, r'outside \$7E:5800-\$7FFF'):
                    self.run_budget(f"01:0412 HostCodeEnd\n"
                                    f"7e:8100 SECTIONSTART_{name}\n"
                                    f"7e:8200 SECTIONEND_{name}\n")

    def test_fx3_nav_has_an_independent_wram_allocation(self):
        report = self.run_budget("""01:0412 HostCodeEnd
7e:2000 SECTIONSTART_.bss.c_game
7e:3000 SECTIONEND_.bss.c_game
7e:5800 SECTIONSTART_.bss.c_nav_fx3
7e:b8c2 SECTIONEND_.bss.c_nav_fx3
""", fx3=True)
        self.assertIn("FX3 data $7E:5800       24770 used   18238 free", report)
        self.assertIn("C .bss   $7E:2000        4096 used   10240 free", report)

    def test_fx3_nav_cannot_overlap_stack_bss_or_host_blob(self):
        for fx3, bank, start, end in [(True, '7e', '0600', '1c00'),
                                      (True, '7e', '5700', 'b800'),
                                      (True, '7f', '5800', 'b800'),
                                      (False, '7e', '5800', 'b800')]:
            with self.subTest(fx3=fx3, bank=bank, start=start):
                with self.assertRaisesRegex(SystemExit, "FX3 data"):
                    self.run_budget(f"01:0412 HostCodeEnd\n"
                        f"{bank}:{start} SECTIONSTART_.bss.c_nav_fx3\n"
                        f"{bank}:{end} SECTIONEND_.bss.c_nav_fx3\n", fx3=fx3)

    def test_small_fx3_nav_allocation_is_not_counted_as_general_bss(self):
        report = self.run_budget("""01:0412 HostCodeEnd
7e:2000 SECTIONSTART_.bss.c_game
7e:3000 SECTIONEND_.bss.c_game
7e:5800 SECTIONSTART_.bss.c_nav_fx3
7e:6000 SECTIONEND_.bss.c_nav_fx3
""", fx3=True)
        self.assertIn("FX3 data $7E:5800        2048 used   40960 free", report)
        self.assertIn("C .bss   $7E:2000        4096 used   10240 free", report)

    def test_fx3_room_cache_counts_against_the_shared_upper_wram_budget(self):
        symbols = ("01:0412 HostCodeEnd\n"
                   "7e:5800 SECTIONSTART_.bss.c_nav_fx3\n"
                   "7e:baca SECTIONEND_.bss.c_nav_fx3\n"
                   "7e:baca SECTIONSTART_room_query_cache\n"
                   "7e:bed0 SECTIONEND_room_query_cache\n")
        report = self.run_budget(symbols, fx3=True)
        self.assertIn("FX3 data $7E:5800       26320 used   16688 free", report)
        for start, end in [('7e:5700', '7e:5b06'), ('7f:baca', '7f:bed0'),
                           ('7e:fefa', '7f:0300')]:
            with self.subTest(start=start, end=end):
                with self.assertRaisesRegex(SystemExit, "room_query_cache is outside"):
                    self.run_budget("01:0412 HostCodeEnd\n"
                                    f"{start} SECTIONSTART_room_query_cache\n"
                                    f"{end} SECTIONEND_room_query_cache\n", fx3=True)

    def test_reclaimed_region_and_bss_are_independent(self):
        report = self.run_budget("""01:0412 HostCodeEnd
7e:0600 SECTIONSTART_.rodata.c_math
7e:1c00 SECTIONEND_.rodata.c_math
7e:5300 SECTIONSTART_.bss.c_math
7e:5304 SECTIONEND_.bss.c_math
""")
        self.assertIn("C image  $7E:0600        5632 used       0 free", report)

    def test_resolved_gsu2_layout_does_not_use_fx3_bank_overrides(self):
        report = self.run_budget("""01:0412 HostCodeEnd
7f:6000 SECTIONSTART_.rodata.c_item
7f:7000 SECTIONEND_.rodata.c_item
7f:8000 SECTIONSTART_.Bat_updatetext_0x0.c_bat
7f:9000 SECTIONEND_.Bat_updatetext_0x0.c_bat
""")
        self.assertIn("C image  $7F:0000", report)
        self.assertIn("C image  $7F:8000", report)

    def test_missing_resolved_layout_is_not_silently_accepted(self):
        with self.assertRaisesRegex(SystemExit, "missing resolved C_LAYOUT"):
            self.run_budget("01:0412 HostCodeEnd\n", layout=False)

    def test_host_cannot_overlap_new_code_start(self):
        with self.assertRaisesRegex(SystemExit, "host code ends"):
            self.run_budget("01:0601 HostCodeEnd\n")
        self.run_budget("01:0600 HostCodeEnd\n")  # end is exclusive

    def test_code_cannot_reach_stack(self):
        with self.assertRaisesRegex(SystemExit, "outgrew its WRAM region"):
            self.run_budget("""01:0412 HostCodeEnd
7e:0600 SECTIONSTART_.rodata.c_math
7e:1c01 SECTIONEND_.rodata.c_math
""")

    def test_code_cannot_be_placed_below_its_region(self):
        with self.assertRaisesRegex(SystemExit, "outside"):
            self.run_budget("""01:0412 HostCodeEnd
7e:0500 SECTIONSTART_.rodata.c_math
7e:0550 SECTIONEND_.rodata.c_math
""")

    def test_code_cannot_be_placed_in_another_bank(self):
        with self.assertRaisesRegex(SystemExit, "outside"):
            self.run_budget("""01:0412 HostCodeEnd
7f:0600 SECTIONSTART_.rodata.c_math
7f:0800 SECTIONEND_.rodata.c_math
""")

    def test_upper_region_cannot_cross_into_resident_bank(self):
        with self.assertRaisesRegex(SystemExit, "outside"):
            self.run_budget("""01:0412 HostCodeEnd
7e:7f00 SECTIONSTART_.Enemy_updatetext_0x0.c_enemy
7e:8010 SECTIONEND_.Enemy_updatetext_0x0.c_enemy
""")

    def test_initialized_data_guard_works_in_variant_directory(self):
        with self.assertRaisesRegex(SystemExit, "initialised C data"):
            self.run_budget("""01:0412 HostCodeEnd
04:0010 SECTIONSTART_ram../build_variant/c_math.asm.data
""")

    def test_native_math_cannot_be_placed_past_the_low_image(self):
        with self.assertRaisesRegex(SystemExit, "outside"):
            self.run_budget("""01:0412 HostCodeEnd
7e:1c00 SECTIONSTART_math_native
7e:1d09 SECTIONEND_math_native
""")


if __name__ == "__main__":
    unittest.main()
