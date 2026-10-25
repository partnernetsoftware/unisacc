#!/usr/bin/env python3
"""LEGACY shim: seal-modules audit lives in tests/sealparadigm.cx (unisacc.com -run).

Full historical implementation: archive/tests/legacy/sealmodulescheck.py
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    cmd = [str(ROOT / "unisacc.com"), "-run", "tests/sealparadigm.cx", "--", "--audit", str(ROOT)]
    return subprocess.run(cmd, cwd=ROOT).returncode


if __name__ == "__main__":
    sys.exit(main())
