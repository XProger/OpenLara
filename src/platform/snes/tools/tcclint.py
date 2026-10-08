#!/usr/bin/env python3
"""Refuse C that 816-tcc compiles wrong.

`A || !x` -- an operand of an || chain, past the first, that is a logical
not (`!x`, `!p->f`, `!(x & m)`) -- loses the earlier operands' "true":
their jumps land on the path that goes on as if they were false.
`if (s != 9 || !n) return;` returns only when n is 0, whatever s is;
`A || !x || C` decides on C; `while`, `return` and `?:` alike. Found
2026-10-06 (the inventory's drawOptions drew its box border under every
ring frame). `x == 0` in its place, `!x || A`, `A || (B && !C)` and
`A && !x` all compile right; split the test, or write `== 0`.

    tcclint.py FILE.c ...    exit 1 and name each site
"""

import re
import sys


def strip_comments(text):
    """Comments and string literals blanked, newlines kept (line numbers)."""
    blank = lambda m: re.sub(r"[^\n]", " ", m.group(0))
    text = re.sub(r"/\*.*?\*/", blank, text, flags=re.S)
    text = re.sub(r"//[^\n]*", blank, text)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', blank, text)


def sites(text):
    """(line, operand) for every `|| !operand`."""
    code = strip_comments(text)
    for m in re.finditer(r"\|\|\s*!(?!=)", code):
        i, depth = m.end(), 0
        while i < len(code):
            c = code[i]
            if c in "([":
                depth += 1
            elif c in ")]":
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and (code.startswith("||", i) or code.startswith("&&", i) or c in ";?:,"):
                break
            i += 1
        yield code.count("\n", 0, m.start()) + 1, code[m.end():i].strip()


def main(paths):
    bad = 0
    for path in paths:
        with open(path) as f:
            text = f.read()
        for line, operand in sites(text):
            print(f"{path}:{line}: `|| !{operand}`: 816-tcc drops the earlier operands' "
                  f"true (tools/tcclint.py)", file=sys.stderr)
            bad += 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
