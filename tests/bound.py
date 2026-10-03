#!/usr/bin/env python3
"""Bound one command and its descendants. Test infrastructure only."""
import os
import signal
import subprocess
import sys


def stop(process, code):
    owned = {process.pid}
    def send(pid, sig):
        try:
            os.kill(pid, sig)
        except ProcessLookupError:
            pass
    send(process.pid, signal.SIGSTOP)
    try:
        while True:
            rows = subprocess.check_output(
                ['/bin/ps', '-axo', 'pid=,ppid='], text=True,
                stderr=subprocess.DEVNULL,
            ).splitlines()
            added = False
            for row in rows:
                pid, parent = map(int, row.split())
                if parent in owned and pid not in owned:
                    owned.add(pid)
                    send(pid, signal.SIGSTOP)
                    added = True
            if not added:
                break
    except (OSError, subprocess.CalledProcessError, ValueError):
        # A restricted host may deny ps. The child still owns a new process
        # group, so kill that group even when descendant enumeration fails.
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    for pid in owned:
        send(pid, signal.SIGKILL)
    process.wait()
    return code


def main():
    if len(sys.argv) < 3 or not sys.argv[1].isdigit() or not 1 <= int(sys.argv[1]) <= 60:
        print('bound: timeout must be 1..60 seconds, followed by a command', file=sys.stderr)
        return 2
    process = subprocess.Popen(sys.argv[2:], start_new_session=True)
    def interrupted(sig, frame):
        raise SystemExit(stop(process, 128 + sig))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    try:
        rc = process.wait(timeout=int(sys.argv[1]))
    except subprocess.TimeoutExpired:
        return stop(process, 142)
    return rc if rc >= 0 else 128 - rc


if __name__ == '__main__':
    sys.exit(main())
