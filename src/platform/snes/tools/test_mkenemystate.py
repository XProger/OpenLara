"""The species oracle must keep original code and refuse missing coverage."""
from pathlib import Path
import unittest

from mkenemystate import braced, extract


class EnemyStateTests(unittest.TestCase):
    def test_original_bear_enums_and_method_are_verbatim(self):
        source = (Path(__file__).parents[3] / 'fixed/enemy.h').read_text()
        result = extract(source, 'Bear')
        method = result[result.index('    virtual int32 updateState()'):].rstrip()
        self.assertIn(method, source)
        self.assertIn('HIT_MASK = 0x2406C', result)
        self.assertIn('STATE_WALK   = 0', result)
        self.assertIn('DAMAGE_ATTACK = 400', result)
        self.assertNotIn('Bear(Room*', result)
        self.assertNotIn('SND_HIT_BEAR', result)

    def test_missing_and_duplicate_classes_fail(self):
        for source in ['', 'struct Bear : Enemy\n{}\nstruct Bear : Enemy\n{}']:
            with self.assertRaisesRegex(ValueError, 'expected one original class'):
                extract(source, 'Bear')

    def test_missing_method_fails(self):
        with self.assertRaisesRegex(ValueError, 'exactly one updateState'):
            extract('struct Bear : Enemy\n{\n    enum { X };\n}', 'Bear')

    def test_unterminated_block_fails(self):
        with self.assertRaisesRegex(ValueError, 'unterminated'):
            braced('void method() { if (x) { }', 0)


if __name__ == '__main__':
    unittest.main()
