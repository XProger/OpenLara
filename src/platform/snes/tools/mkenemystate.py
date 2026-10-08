#!/usr/bin/env python3
"""Extract a species' original enums/updateState verbatim for a host oracle."""
import re
import sys
from pathlib import Path


def braced(source, start):
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth and end < len(source):
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
        end += 1
    if depth:
        raise ValueError('unterminated original class/method')
    return source[start:end]


def extract(source, species):
    matches = list(re.finditer(rf'^struct {re.escape(species)} : Enemy\s*\n\{{', source, re.M))
    if len(matches) != 1:
        raise ValueError(f'{species}: expected one original class, found {len(matches)}')
    body = braced(source, matches[0].start())
    enums = [braced(body, m.start()) + ';' for m in re.finditer(r'^    enum\s*\{', body, re.M)]
    methods = list(re.finditer(r'^    virtual int32 updateState\(\)\s*\n    \{', body, re.M))
    if not enums or len(methods) != 1:
        raise ValueError(f'{species}: expected enums and exactly one updateState')
    return '// Generated from src/fixed/enemy.h; do not edit.\n' + '\n\n'.join(
        enums + [braced(body, methods[0].start())]) + '\n'


if __name__ == '__main__':
    Path(sys.argv[3]).write_text(extract(Path(sys.argv[1]).read_text(), sys.argv[2]))
