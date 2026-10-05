#!/usr/bin/env python3
"""Bound one command and its descendants. Test infrastructure only."""
import os
import signal
import subprocess
import sys


def stop(process, code):
    # Cleanup must not be interrupted after descendants have been stopped.
    for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        signal.signal(sig, signal.SIG_IGN)
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
    # The 60 s ceiling is this machine's rule (owner 2026-09-25); GitHub's hosted runners are two to
    # four times slower, so under GITHUB_ACTIONS the ceiling is 300 s (0.0.27 C1).
    cap = 300 if os.environ.get('GITHUB_ACTIONS') == 'true' else 60
    if len(sys.argv) < 3 or not sys.argv[1].isdigit() or not 1 <= int(sys.argv[1]) <= cap:
        print('bound: timeout must be 1..%d seconds, followed by a command' % cap, file=sys.stderr)
        return 2
    process = subprocess.Popen(sys.argv[2:], start_new_session=True)
    def interrupted(sig, frame):
        raise SystemExit(stop(process, 128 + sig))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    signal.signal(signal.SIGHUP, interrupted)
    try:
        rc = process.wait(timeout=int(sys.argv[1]))
    except subprocess.TimeoutExpired:
        return stop(process, 142)
    # The direct child may exit while background children remain in its session.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    return rc if rc >= 0 else 128 - rc


if __name__ == '__main__':
    sys.exit(main())
