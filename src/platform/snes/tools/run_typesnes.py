#!/usr/bin/env python3
"""Run cartridge fixtures without modifying the TypeSNES checkout.

Reuse its installed dependencies, but keep source, Vite caches and temporary
test files in a disposable directory. No package installation is performed.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--typesnes", type=Path, required=True)
    parser.add_argument("--runner", type=Path, action="append", required=True)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("args", nargs=argparse.REMAINDER)
    options = parser.parse_args()
    checkout = options.typesnes.resolve()
    runners = [runner.resolve() for runner in options.runner]
    log = options.log.resolve()
    modules = checkout / "node_modules"
    if not (checkout / "src/emulator.ts").is_file() or not (modules / ".bin/vitest").is_file():
        parser.error(f"TypeSNES source and installed vitest required at {checkout}")
    args = options.args[1:] if options.args[:1] == ["--"] else options.args
    log.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="openlara-typesnes-") as directory:
        root = Path(directory)
        shutil.copytree(checkout / "src", root / "src",
                        ignore=shutil.ignore_patterns("__openlara*", "*.test.ts", "*.test.tsx"))
        shutil.copyfile(checkout / "package.json", root / "package.json")
        # Keep node_modules itself writable locally: Vite may create .vite-temp
        # there even when cacheDir is set. Individual dependencies stay read-only.
        (root / "node_modules").mkdir()
        for entry in modules.iterdir():
            if not entry.name.startswith(".vite"):
                (root / "node_modules" / entry.name).symlink_to(entry, target_is_directory=entry.is_dir())
        tests = []
        for index, runner in enumerate(runners):
            target = f"src/__openlara_{index}.test.ts"
            shutil.copyfile(runner, root / target)
            tests.append(target)
        (root / "vitest.config.mjs").write_text(
            "export default { cacheDir: './.vite', test: { environment: 'node' } };\n")
        with log.open("w") as output:
            result = subprocess.run(
                [os.fspath(root / "node_modules/.bin/vitest"), "run",
                 "--config", os.fspath(root / "vitest.config.mjs"),
                 "--reporter=verbose", "--testTimeout=600000", *args,
                 *tests], cwd=root, stdout=output, stderr=subprocess.STDOUT)
    for line in log.read_text().splitlines():
        if any(marker in line for marker in ("[openlara]", "FAIL", "Error", "Test Files", "Tests")):
            print(line)
    if result.returncode:
        print(f"TypeSNES failed; full output: {log}")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
