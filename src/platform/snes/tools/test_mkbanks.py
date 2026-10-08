"""The active level, not the build directory's history, selects ROM pages."""
import json
import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import mkbanks
import hosttables
from test_hosttables import fixture


class TextureInventoryTests(unittest.TestCase):
    def test_host_blob_includes_room_directory_in_source_bank_budget(self):
        # LEVEL2's 94-room directory precedes the blob. A blob that fits in
        # destination RAM can still wrap the GSU's source ROM address.
        offset = 4 + 94 * 32
        mkbanks.check_host_blob(offset, mkbanks.GSU_BANK - offset)
        with self.assertRaisesRegex(SystemExit, "one 64 KB ROM bank"):
            mkbanks.check_host_blob(offset, mkbanks.GSU_BANK - offset + 1)

    def test_host_blob_length_is_nonzero_and_leaves_word_read_headroom(self):
        mkbanks.check_host_blob(0, 65534)
        for size in [0, 65535, 65536]:
            with self.subTest(size=size):
                with self.assertRaisesRegex(SystemExit, "one 64 KB ROM bank"):
                    mkbanks.check_host_blob(0, size)

    def test_packed_banks_stop_before_boot_code(self):
        mkbanks.check_bank_space(6 * mkbanks.GSU_BANK, 21)  # exclusive end = $3C
        with self.assertRaisesRegex(SystemExit, r"ROM bank \$3C"):
            mkbanks.check_bank_space(6 * mkbanks.GSU_BANK, 22)

    def test_partial_bank_space_is_rounded_to_gsu_banks(self):
        with self.assertRaisesRegex(SystemExit, r"ROM bank \$3C"):
            mkbanks.check_bank_space(6 * mkbanks.GSU_BANK + 1, 21)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def manifest(self, count):
        (self.root / "TEXTURES.json").write_text(json.dumps({"pages": count}))

    def page(self, number, size=mkbanks.PAGE_SIZE):
        (self.root / f"TEX{number:02d}.BIN").write_bytes(bytes(size))

    def test_fx3_emits_banked_host_tables_above_the_gsu2_ceiling(self):
        self.manifest(1)
        self.page(0)
        level = bytearray(65536)
        struct.pack_into('<HH', level, 0, 1, 0)
        (self.root / 'LEVEL.BIN').write_bytes(level)
        (self.root / 'WEAPON.json').write_text('{"animations":[],"meshes":[]}')
        (self.root / 'SOUND.BIN').write_bytes(bytes(40000))
        image = hosttables.build(*fixture(sectors=16384, anims=3000))
        (self.root / 'HOST.BIN').write_bytes(image)
        out = self.root / 'textures.inc'
        with patch('sys.argv', ['mkbanks.py', str(self.root), str(out), '--fx3']), contextlib.redirect_stdout(io.StringIO()):
            mkbanks.main()
        assembly = out.read_text()
        # FX3's APU image goes whole with the level's CPU-only data, after the
        # host tables, starting a bank (main.asm reads it through olLevel)
        self.assertIn('.SECTION "sound_0" FORCE\n    .INCBIN "SOUND.BIN" SKIP 0 READ 32768', assembly)
        self.assertIn('.SECTION "sound_1" FORCE\n    .INCBIN "SOUND.BIN" SKIP 32768 READ 7232', assembly)
        self.assertNotIn('SoundImage0:', assembly)
        defines = (self.root / 'texbank.inc').read_text()
        self.assertIn('.DEFINE SOUND_IMAGE_SIZE 40000\n', defines)
        self.assertIn('.DEFINE SOUND_IMAGE_SIZE_0 40000\n', defines)
        self.assertIn('.DEFINE SOUND_IMAGE_SIZE_1 0\n', defines)
        self.assertIn('.DEFINE FLASH_MESH_BANK $FF\n', defines)        # no pistols here
        self.assertIn('.BANK 64\n', assembly)
        self.assertIn('.BANK 68\n', assembly)
        self.assertIn(f'SKIP {len(image) // 32768 * 32768} READ {len(image) % 32768}', assembly)
        self.assertIn('.DEFINE HOST_DIRECTORY_ADDR $E00000', (self.root / 'hostbank.inc').read_text())
        self.assertIn('.DEFINE HOST_SECTOR_SIZE 65536', (self.root / 'hostbank.inc').read_text())
        self.assertIn('#define OL_HOST_', (self.root / 'olhost.h').read_text())
        # the level table: OlLevelDesc's 32 words, four .DW lines; no FX.BIN
        table = (self.root / 'leveltable.inc').read_text()
        rows = [line for line in table.splitlines() if line.strip().startswith('.DW')]
        self.assertEqual(len(rows), 4)
        self.assertEqual(sum(len(r.split(',')) for r in rows) * 2, mkbanks.LEVEL_ENTRY)
        self.assertEqual(rows[3].replace(' ', ''), '.DW$0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000')
        # no TITLE.BIN: no title screen, and main.asm boots the level
        self.assertIn('.DEFINE OL_TITLE_SIZE 0\n', (self.root / 'hostbank.inc').read_text())

    def test_fx3_title_screen_goes_with_the_first_levels_tables_within_one_bank(self):
        self.manifest(1)
        self.page(0)
        level = bytearray(65536)
        struct.pack_into('<HH', level, 0, 1, 0)
        (self.root / 'LEVEL.BIN').write_bytes(level)
        (self.root / 'WEAPON.json').write_text('{"animations":[],"meshes":[]}')
        (self.root / 'SOUND.BIN').write_bytes(bytes(40000))
        (self.root / 'HOST.BIN').write_bytes(hosttables.build(*fixture(sectors=16384, anims=3000)))
        (self.root / 'TITLE.BIN').write_bytes(bytes(mkbanks.TITLE_SIZE))
        out = self.root / 'textures.inc'
        with patch('sys.argv', ['mkbanks.py', str(self.root), str(out), '--fx3']), contextlib.redirect_stdout(io.StringIO()):
            mkbanks.main()
        defines = (self.root / 'hostbank.inc').read_text()
        self.assertIn(f'.DEFINE OL_TITLE_SIZE {mkbanks.TITLE_SIZE}\n', defines)
        offset = int(defines.split('.DEFINE OL_TITLE_OFFSET $')[1][:4], 16)
        bank = int(defines.split('.DEFINE OL_TITLE_BANK $')[1][:2], 16)
        address = int(defines.split('.DEFINE OL_TITLE_ADDR $')[1][:6], 16)
        self.assertEqual(address, bank << 16 | offset)
        self.assertGreaterEqual(bank, 0xC0)
        self.assertLessEqual(offset + mkbanks.TITLE_SIZE, 0x10000)      # one MVN
        self.assertIn('.INCBIN "TITLE.BIN"', out.read_text())
        (self.root / 'TITLE.BIN').write_bytes(bytes(100))
        with patch('sys.argv', ['mkbanks.py', str(self.root), str(out), '--fx3']), \
                contextlib.redirect_stdout(io.StringIO()), self.assertRaises(SystemExit):
            mkbanks.main()

    def test_sound_image_must_fit_the_sound_cpu(self):
        # $0400 to the echo bytes at $FF00: the directory, pitches and samples
        (self.root / 'SOUND.BIN').write_bytes(bytes(mkbanks.SOUND_MAX))
        self.assertEqual(mkbanks.sound_image(self.root), mkbanks.SOUND_MAX)
        (self.root / 'SOUND.BIN').write_bytes(bytes(mkbanks.SOUND_MAX + 1))
        with self.assertRaisesRegex(SystemExit, "SOUND.BIN is"):
            mkbanks.sound_image(self.root)
        (self.root / 'SOUND.BIN').unlink()
        with self.assertRaisesRegex(SystemExit, "SOUND.BIN not found"):
            mkbanks.sound_image(self.root)

    def test_sound_banks_count_against_the_boot_code(self):
        mkbanks.check_bank_space(6 * mkbanks.GSU_BANK, 20, 2 * mkbanks.WLA_BANK)
        with self.assertRaisesRegex(SystemExit, r"ROM bank \$3C"):
            mkbanks.check_bank_space(6 * mkbanks.GSU_BANK, 20, 2 * mkbanks.WLA_BANK + 1)

    def test_ignores_pages_from_a_previous_larger_level(self):
        self.manifest(2)
        for i in range(4):
            self.page(i)
        self.assertEqual(mkbanks.texture_pages(self.root), ["TEX00.BIN", "TEX01.BIN"])

    def test_missing_page_does_not_renumber_later_pages(self):
        self.manifest(3)
        self.page(0)
        self.page(2)
        with self.assertRaisesRegex(SystemExit, "TEX01.BIN: missing"):
            mkbanks.texture_pages(self.root)

    def test_truncated_page_is_rejected(self):
        self.manifest(1)
        self.page(0, 32768)
        with self.assertRaisesRegex(SystemExit, "not 65536 bytes"):
            mkbanks.texture_pages(self.root)

    def test_invalid_counts_are_rejected(self):
        for count in [0, -1, 33, True, "2", 1.5]:
            with self.subTest(count=count):
                self.manifest(count)
                with self.assertRaisesRegex(SystemExit, "invalid texture page count"):
                    mkbanks.texture_pages(self.root)


if __name__ == "__main__":
    unittest.main()
