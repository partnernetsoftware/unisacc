#!/usr/bin/env python3
"""Bound one command and its descendants. Test infrastructure only."""
import os
import re
import secrets
import signal
import subprocess
import sys
CHAIN = "UNISACC_BOUND_CHAIN"



def scoped_pids(token):
    """Every live process whose environment carries this bound's token.
    Linux reads /proc/<pid>/environ.  macOS asks ps for the environment (-E): THE macOS BRANCH IS NOT VERIFIED
    on Darwin yet (c43 watchdog, m4pro check pending); until it is, macOS must not be recorded green.
    Unreadable entries are skipped; a failing ps yields nothing rather than an error."""
    found = set()
    needle = ('%s=' % CHAIN).encode()
    if os.path.isdir('/proc/self'):
        for name in os.listdir('/proc'):
            if not name.isdigit(): continue
            try:
                with open('/proc/%s/environ' % name, 'rb') as f: env = f.read()
            except OSError:
                continue
            for item in env.split(b'\0'):
                if item.startswith(needle) and token.encode() in item:
                    found.add(int(name)); break
    else:
        out = subprocess.run(['ps', '-axwwE', '-o', 'pid=,command='], capture_output=True, text=True).stdout
        for line in out.splitlines():
            if CHAIN + '=' in line and token in line:
                found.add(int(line.split(None, 1)[0]))
    found.discard(os.getpid())
    return found

def stop(process, code, token=None):
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
    # c43: anything in this scope (orphaned, setsid'd or nested) carries the token; freeze, then kill all of them
    for _ in range(50):
        fresh = scoped_pids(token) - owned
        if not fresh: break
        owned |= fresh
        for pid in fresh: send(pid, signal.SIGSTOP)
    for pid in scoped_pids(token) | owned:
        send(pid, signal.SIGKILL)
    process.wait()
    return code


def scale():
    """0.0.27 C1: GitHub's hosted runners are 2-4x slower than this machine.  Under GITHUB_ACTIONS,
    UNISACC_TIME_SCALE (1..5) multiplies every bound, so the scripts' own 20/46/55 s limits stretch
    with the runner instead of timing out.  Locally the scale is always 1 (the 60 s rule holds)."""
    if os.environ.get('GITHUB_ACTIONS') != 'true': return 1
    v = os.environ.get('UNISACC_TIME_SCALE', '1')
    return int(v) if v.isdigit() and 1 <= int(v) <= 5 else 1


def main():
    if len(sys.argv) < 3 or not sys.argv[1].isdigit() or not 1 <= int(sys.argv[1]) <= 60:
        print('bound: timeout must be 1..60 seconds, followed by a command', file=sys.stderr)
        return 2
    token = secrets.token_hex(8)
    env = dict(os.environ)
    env[CHAIN] = (env.get(CHAIN, "") + " " + token).strip()
    process = subprocess.Popen(sys.argv[2:], start_new_session=True, env=env)
    def interrupted(sig, frame):
        raise SystemExit(stop(process, 128 + sig, token))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    signal.signal(signal.SIGHUP, interrupted)
    try:
        rc = process.wait(timeout=int(sys.argv[1]) * scale())
    except subprocess.TimeoutExpired:
        return stop(process, 142, token)
    # The direct child may exit while background children remain in its session.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    # c43: a command that exits normally may still leave an escaped descendant (reparented, own session).
    # Kill everything carrying this scope's token; no freeze needed once the command has ended.
    for pid in scoped_pids(token):
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    return rc if rc >= 0 else 128 - rc


if __name__ == '__main__':
    sys.exit(main())
