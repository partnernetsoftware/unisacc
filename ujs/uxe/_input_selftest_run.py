#!/usr/bin/env python3
"""Wall-clock wrapper for input encode/decode selftest."""
from __future__ import annotations
import signal
import subprocess
import sys

WALL_SEC = 15


def main() -> int:
    signal.alarm(WALL_SEC)
    return subprocess.run(
        ["node", "/Users/wjc/repos/unisacc/ujs/uxe/_input_selftest.mjs"],
        cwd="/Users/wjc/repos/unisacc/ujs",
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
