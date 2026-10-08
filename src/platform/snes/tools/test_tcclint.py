import unittest

import tcclint


class TccLintTest(unittest.TestCase):
    def found(self, text):
        return [operand for _, operand in tcclint.sites(text)]

    def test_a_not_after_an_or_is_refused_wherever_it_stands(self):
        self.assertEqual(self.found("if (s != 9 || !n) return;"), ["n"])
        self.assertEqual(self.found("if (a || !(f & 4) || c) x();"), ["(f & 4)"])
        self.assertEqual(self.found("while (a ||\n       !p->h) y();"), ["p->h"])
        self.assertEqual(self.found("return a || !b;"), ["b"])

    def test_the_shapes_816_tcc_gets_right_pass(self):
        for text in ("if (!a || b) return;", "if (a || b == 0) return;",
                     "if (a || (b && !c)) return;", "if (a && !b) x();",
                     "if (a || b != c) return;", "/* a || !b */ x();", 'puts("a || !b");'):
            self.assertEqual(self.found(text), [], text)

    def test_line_numbers_survive_comments(self):
        self.assertEqual([line for line, _ in tcclint.sites("/*\n\n*/\nif (a || !b) c();")], [4])


if __name__ == "__main__":
    unittest.main()
