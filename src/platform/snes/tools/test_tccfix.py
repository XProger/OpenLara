"""FX3's data slot must be independent of the FastROM run-bank rewrite."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import tccfix
import wramglobals


class TccFixTests(unittest.TestCase):
    def test_wram_manifest_excludes_rom_other_banks_and_private_labels(self):
        source = ('.RAMSECTION ".bss" BANK $7e SLOT 2\nobject dsb 12\n'
                  'tccs_{WLA_FILENAME}_private dsb 2\n.ENDS\n'
                  '.SECTION ".rodata" SUPERFREE\nconstant dsb 12\n.ENDS\n'
                  '.RAMSECTION ".bss" BANK $7f SLOT 2\nother dsb 12\n.ENDS\n')
        self.assertEqual(wramglobals.collect([source]), {'object': {'size': 12, 'mode': 'w'}})
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            wramglobals.collect([source, source])

    def test_direct_page_aliases_share_only_their_declared_extent(self):
        source = ('.RAMSECTION "host_ptrs_low" BANK 0 SLOT 1 ORGA $28 FORCE PRIORITY 1000\n'
                  'pointer dsb 0\nalias dsb 4\nnext dsb 2\n.ENDS\n')
        values = wramglobals.collect([], source)
        self.assertEqual(values['pointer'], {'size': 4, 'mode': 'b', 'address': 0x28})
        self.assertEqual(values['alias'], values['pointer'])
        self.assertEqual(values['next'], {'size': 2, 'mode': 'b', 'address': 0x2C})
        with self.assertRaisesRegex(ValueError, 'outside direct page'):
            wramglobals.collect([], source.replace('$28', '$FE'))
        with self.assertRaisesRegex(ValueError, 'unbounded'):
            wramglobals.collect([], source.replace('next dsb 2', 'next dsb 0'))

    def test_global_rewrite_keeps_unknown_indexed_and_out_of_bounds_accesses(self):
        symbols = {'object': {'size': 4, 'mode': 'w'}, 'pointer': {'size': 4, 'mode': 'b'}}
        source = ('lda.l object + 0\nsta.l object + 0 + 2 ; high word\n'
                  'lda.l pointer + 2\nlda.l object + 3\nlda.l object - 1\n'
                  'lda.l object + 0,x\nlda.l constant + 0\nlda.l [pointer]\n')
        expected = source.replace('lda.l object + 0\n', 'lda.w object + 0\n')
        expected = expected.replace('sta.l object + 0 + 2', 'sta.w object + 0 + 2')
        expected = expected.replace('lda.l pointer + 2', 'lda.b pointer + 2')
        self.assertEqual(tccfix.fast_wram_globals(source, symbols), expected)

    def test_wide_shifts_keep_final_carry_and_anonymous_labels(self):
        for opcode, mask in [('asl', '$FF00'), ('lsr', '$00FF')]:
            for count in range(1, 17):
                old = f'ldy.w #{count}\n-\n{opcode} a\ndey\nbne -\n+\n'
                expected = ('-\nxba\nand #' + mask + '\n' + (opcode + ' a\n') * (count - 8)
                            + 'ldy #0\n+\n') if 9 <= count <= 15 else old
                self.assertEqual(tccfix.fast_wide_shifts(old), expected)
        for count in range(1, 17):
            arithmetic = f'ldy.w #{count}\n-\ncmp #$8000\nror a\ndey\nbne -\n+\n'
            fast = tccfix.fast_wide_shifts(arithmetic)
            if 9 <= count <= 14:
                self.assertIn('xba\nand #$00FF\ncmp #$0080\nbcc ', fast)
                self.assertEqual(fast.count('cmp #$8000\nror a\n'), count - 8)
                self.assertTrue(fast.endswith('ldy #0\n+\n'))
            else:
                self.assertEqual(fast, arithmetic)

    def test_sign_extension_is_exact_and_keeps_anonymous_labels(self):
        old = 'ldy.w #15\n-\ncmp #$8000\nror a\ndey\nbne -\n+\n'
        label = 'tccs_{WLA_FILENAME}_signext_0'
        fast = (f'-\ntay\nand #$8000\nbeq {label}_positive\nlda #$FFFF\nbra {label}_value\n'
                f'{label}_positive:\nlda #0\n{label}_value:\npha\ntya\nasl a\nasl a\npla\nldy #0\n+\n')
        self.assertEqual(tccfix.fast_sign_extend('sta.b tcc__r0\n' + old + 'sta.b tcc__r1\n'),
                         'sta.b tcc__r0\n' + fast + 'sta.b tcc__r1\n')
        self.assertEqual(tccfix.fast_sign_extend(old.replace('#15', '#14')), old.replace('#15', '#14'))

    def test_compact_sign_extension_only_changes_units_that_shrink(self):
        old = 'ldy.w #15\n-\ncmp #$8000\nror a\ndey\nbne -\n+\n'
        for count in range(4):
            self.assertEqual(tccfix.compact_sign_extend(old * count), old * count)
        rewritten = tccfix.compact_sign_extend(old * 4)
        helper = 'tccs_{WLA_FILENAME}_signext_shared'
        self.assertEqual(rewritten.count('-\njsr.l ' + helper + '\n+\n'), 4)
        self.assertEqual(rewritten.count(helper + ':\n'), 1)
        self.assertIn(tccfix.fast_sign_extend(old) + 'rtl\n.ENDS\n', rewritten)
        self.assertIn('.SECTION ".signext_shared" SUPERFREE', rewritten)

    def test_compact_helper_is_retargeted_to_the_same_resident_wram_image(self):
        old = 'ldy #15\n-\ncmp #$8000\nror a\ndey\nbne -\n+\n'
        with tempfile.TemporaryDirectory() as directory:
            source, target = Path(directory) / 'c_probe.asm', Path(directory) / 'fixed.asm'
            source.write_text('.SECTION ".probe" SUPERFREE\n' + old * 4 + '.ENDS\n'
                              '.RAMSECTION ".bss" BANK $7e SLOT 2\nx dsb 2\n.ENDS\n')
            subprocess.run([sys.executable, str(Path(__file__).with_name('tccfix.py')),
                            str(source), str(target), '--bank', '5', '--slot', '6',
                            '--wrambank', '0x7F', '--compact-sign-extend'],
                           check=True, capture_output=True)
            text = target.read_text()
            self.assertEqual(text.count('.BASE $7A\n.BANK 5 SLOT 6'), 2)
            self.assertIn('.SECTION ".signext_shared.c_probe" SEMIFREE', text)
            self.assertIn('.BASE $00\n.RAMSECTION', text)

    def rewrite(self, bss_slot=None):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "c_nav_fx3.asm"
            target = Path(directory) / "fixed.asm"
            source.write_text('.SECTION ".rodata" SUPERFREE\n.ENDS\n'
                              '.RAMSECTION ".bss" BANK $7e SLOT 2\n'
                              'olNav dsb 24720\n.ENDS\n'
                              '.SECTION ".helper" SUPERFREE\n.ENDS\n')
            command = [sys.executable, str(Path(__file__).with_name("tccfix.py")),
                       str(source), str(target), "--bank", "61", "--slot", "0",
                       "--wrambank", "0xBD"]
            if bss_slot is not None:
                command += ["--bss-slot", str(bss_slot)]
            subprocess.run(command, check=True, capture_output=True)
            return target.read_text()

    def test_fx3_data_slot_does_not_inherit_fastrom_base(self):
        text = self.rewrite(9)
        self.assertIn('.BASE $00\n.RAMSECTION ".bss.c_nav_fx3" BANK $7e SLOT 9', text)
        self.assertEqual(text.count('.BASE $80\n.BANK 61 SLOT 0'), 2)

    def test_default_data_remains_in_general_bss(self):
        self.assertIn('BANK $7e SLOT 2', self.rewrite())


if __name__ == "__main__":
    unittest.main()
