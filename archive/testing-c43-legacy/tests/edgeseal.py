#!/usr/bin/env python3
"""LEGACY shim: gate -> tests/edgeseal.cx; produce -> archive/tests/legacy/edgeseal.py."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    leaf, root = sys.argv[2], str(Path(sys.argv[3]).resolve())
    if sys.argv[1] == "--gate":
        cmd = [str(ROOT / "unisacc.com"), "-run", "tests/edgeseal.cx", "--", "--gate", leaf, root]
        return subprocess.run(cmd, cwd=ROOT).returncode
    if sys.argv[1] == "--produce":
        return subprocess.run(
            [sys.executable, str(ROOT / "archive/tests/legacy/edgeseal.py"), "--produce", leaf, root],
            cwd=ROOT,
        ).returncode
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
