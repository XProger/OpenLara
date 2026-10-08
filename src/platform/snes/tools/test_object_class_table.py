#!/usr/bin/env python3
"""Classification source changes must regenerate the map or reject its guard."""
from pathlib import Path
import shutil
import tempfile
import unittest

from object_class_table import compile_map

SOURCE = Path(__file__).resolve().parent.parent / "src"


class ObjectClassTableTests(unittest.TestCase):
    def fixture(self, mutate):
        with tempfile.TemporaryDirectory(prefix="openlara-class-test-") as directory:
            source = Path(directory)
            for name in ("object.c", "object.h", "enemy.c", "item.h"):
                shutil.copyfile(SOURCE / name, source / name)
            mutate(source)
            return compile_map(source)

    def test_original_source_and_species(self):
        none, table = compile_map(SOURCE)
        self.assertEqual(none, 0)
        self.assertEqual(table[0], 1)
        self.assertEqual([i for i in range(7, 35) if table[i] == 2], [7, 8, 9, 12, 13, 14, 19])
        self.assertEqual(sum(value != none for value in table), 66)

    def test_gsu2_original_source_and_species(self):
        none, table = compile_map(SOURCE, fx3=False)
        self.assertEqual(none, 0)
        self.assertEqual(table[0], 1)
        self.assertEqual([i for i in range(7, 35) if table[i] == 2], [7, 8, 9])
        self.assertEqual([table[i] for i in [12, 13, 14, 19, 36, 39, 40, 41, 153, 155, 158, 160, 164]], [none] * 13)
        self.assertEqual(sum(value != none for value in table), 53)

    def test_enemy_support_change_regenerates(self):
        def mutate(source):
            path = source / "enemy.c"
            path.write_text(path.read_text().replace("case ITEM_WOLF: return 0;", "case 10: return 0;"))
        _, table = self.fixture(mutate)
        self.assertEqual(table[7], 0)
        self.assertEqual(table[10], 2)

    def test_changed_enum_values_regenerate(self):
        def mutate(source):
            path = source / "object.h"
            path.write_text(path.read_text().replace("CLASS_NONE,", "CLASS_NONE = 20,"))
        none, table = self.fixture(mutate)
        self.assertEqual(none, 20)
        self.assertEqual(table[0], 21)
        self.assertEqual(table[255], 20)

    def test_future_high_type_rejects_guard(self):
        def mutate(source):
            path = source / "object.c"
            path.write_text(path.read_text().replace("if (type == ITEM_LARA)",
                            "if (type == 256) return CLASS_ENEMY;\n    if (type == ITEM_LARA)"))
        with self.assertRaisesRegex(ValueError, "type 256 resolves to class"):
            self.fixture(mutate)


if __name__ == "__main__":
    unittest.main()
