#!/usr/bin/env python3
"""Wall-clock wrapper for packet encode/decode selftest."""
from __future__ import annotations
import signal
import subprocess
import sys

WALL_SEC = 20


def main() -> int:
    signal.alarm(WALL_SEC)
    return subprocess.run(
        ["node", "/Users/wjc/repos/unisacc/ujs/web/engine/_packet_selftest.mjs"],
        cwd="/Users/wjc/repos/unisacc/ujs/web",
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
