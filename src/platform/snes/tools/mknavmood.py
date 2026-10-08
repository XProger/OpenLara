#!/usr/bin/env python3
"""Extract the original Enemy mood methods unchanged for the host oracle.

This is generated test code, not a parallel reimplementation. Fail when the
expected methods change shape instead of quietly losing oracle coverage.
"""
import re
import sys
from pathlib import Path

source = Path(sys.argv[1]).read_text()
names = {"updateMood": 1, "moodSleep": 1, "moodStalk": 1, "moodAttack": 1,
         "moodEscape": 1, "setNextBox": 1, "getMoodWild": 1, "getMoodNormal": 1,
         "getRandomBox": 1, "checkZone": 2, "checkStalk": 1, "checkEscape": 1,
         "getZones": 1}
methods = []
for name, count in names.items():
    pattern = rf"^    (?:X_INLINE )?(?:void|uint32|int32|bool|const uint16\*) {name}\([^\n]*\)\n    \{{"
    found = list(re.finditer(pattern, source, re.MULTILINE))
    if len(found) != count:
        raise SystemExit(f"{name}: expected {count} original definitions, found {len(found)}")
    for m in found:
        depth = 1
        end = m.end()
        while depth:
            if source[end] == "{": depth += 1
            elif source[end] == "}": depth -= 1
            end += 1
        methods.append(source[m.start():end])
Path(sys.argv[2]).write_text("// Generated from src/fixed/enemy.h; do not edit.\n" + "\n\n".join(methods) + "\n")
