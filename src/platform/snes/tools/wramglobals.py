#!/usr/bin/env python3
"""Collect linker-owned C WRAM symbols and FX3's fixed direct-page aliases.

Only explicit $7E .bss declarations and the fixed BANK 0/SLOT 1 host pointer
sections qualify. ROM constants, compiler-private labels and unknown layouts
are excluded. tccfix uses the extents to avoid changing bank-crossing accesses.
"""
import argparse
import json
from pathlib import Path
import re


def collect(sources, direct_page=None):
    symbols = {}
    for source in sources:
        active = False
        for line in source.splitlines():
            if line.startswith('.RAMSECTION'):
                active = bool(re.match(r'^\.RAMSECTION "\.bss" BANK \$7[eE] SLOT 2\s*$', line))
            elif line.startswith('.ENDS'):
                active = False
            elif active:
                m = re.fullmatch(r'(\w+) dsb (\d+)\s*', line)
                if m and not m[1].startswith('tccs_'):
                    if m[1] in symbols:
                        raise ValueError(f'duplicate WRAM global {m[1]}')
                    symbols[m[1]] = {'size': int(m[2]), 'mode': 'w'}
    if direct_page is not None:
        address, pending = None, []
        for line in direct_page.splitlines():
            if line.startswith('.RAMSECTION'):
                m = re.fullmatch(r'\.RAMSECTION "host_ptrs_\w+" BANK 0 SLOT 1 ORGA \$([0-9A-Fa-f]+) FORCE PRIORITY 1000', line)
                address = int(m[1], 16) if m else None
                pending = []
            elif line.startswith('.ENDS'):
                if pending:
                    raise ValueError('unbounded direct-page alias')
                address = None
            elif address is not None:
                m = re.fullmatch(r'(\w+) dsb (\d+)', line)
                if not m:
                    raise ValueError(f'unknown direct-page declaration {line}')
                name, size = m[1], int(m[2])
                pending.append(name)
                if size:
                    if address + size > 256:
                        raise ValueError('host pointer outside direct page')
                    for alias in pending:
                        if alias in symbols:
                            raise ValueError(f'duplicate direct-page global {alias}')
                        symbols[alias] = {'size': size, 'mode': 'b', 'address': address}
                    address += size
                    pending = []
    return symbols


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--direct-page', type=Path)
    parser.add_argument('sources', type=Path, nargs='+')
    args = parser.parse_args()
    symbols = collect([p.read_text() for p in args.sources],
                      args.direct_page.read_text() if args.direct_page else None)
    args.output.write_text(json.dumps(symbols, sort_keys=True, indent=2) + '\n')


if __name__ == '__main__':
    main()
