#!/usr/bin/env python3
"""Bounded runner for game/sim checks. Always alarm before work."""
from __future__ import annotations

import signal
import subprocess
import sys

# Hard wall: hang / unexpected long run must die.
WALL_SEC = 45


def main() -> int:
    signal.alarm(WALL_SEC)
    cmd = [
        "node",
        "/Users/wjc/repos/unisacc/ujs/web/game/_selftest.mjs",
    ]
    p = subprocess.run(cmd, cwd="/Users/wjc/repos/unisacc/ujs/web")
    return p.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:
        print("selftest wrapper failed:", e, file=sys.stderr)
        raise SystemExit(2)
