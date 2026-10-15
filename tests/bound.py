#!/usr/bin/env python3
"""Bound one command and its descendants. Test infrastructure only."""
import os
import re
import secrets
import signal
import subprocess
import sys
import time
CHAIN = "UNISACC_BOUND_CHAIN"
# Defensive (research/c43-bound-stop-889s-readonly.md): stop() has a total budget, so a child that cannot be
# reaped cannot hold the wall clock.  The root cause of the 889 s case is NOT located; this only makes the
# cleanup finite and visible.  Outcome is unchanged: a timeout is always 142, and an unreaped child is never green.
STOP_SECS = float(os.environ.get('UNISACC_BOUND_STOP_SECS', '10'))
# Test-only injection (tests/boundcheck.py): skip the SIGKILLs so the direct child stays stopped.  This exercises the
# unreaped path on Linux; it is NOT a real uninterruptible (D state) child.
TEST_NO_KILL = os.environ.get('UNISACC_BOUND_TEST_NO_KILL') == '1'
TRACE = os.environ.get('BOUND_TRACE')   # phase timestamps, written only when set (probe / diagnosis)
_T0 = [None]


def mark(what):
    now = time.monotonic()
    if _T0[0] is None: _T0[0] = now
    if TRACE:
        with open(TRACE, 'a') as f: f.write('%8.3f %s\n' % (now - _T0[0], what))


def describe(pid):
    """state and wchan of a process for the unreaped diagnostic; '?' where /proc is not available."""
    state = wchan = '?'
    try:
        with open('/proc/%d/stat' % pid) as f: state = f.read().rsplit(')', 1)[1].split()[0]
    except (OSError, IndexError):
        pass
    try:
        with open('/proc/%d/wchan' % pid) as f: wchan = f.read().strip() or '-'
    except OSError:
        pass
    return state, wchan


def scoped_pids(token, deadline=None):
    """Every live process whose environment carries this bound's token.
    Linux reads /proc/<pid>/environ.  macOS asks ps for the environment (-E): THE macOS BRANCH IS NOT VERIFIED
    on Darwin yet (c43 watchdog, m4pro check pending); until it is, macOS must not be recorded green.
    Unreadable entries are skipped; a failing ps yields nothing rather than an error.
    With a deadline, a scan that runs past it stops early and returns what it has (defensive)."""
    found = set()
    needle = ('%s=' % CHAIN).encode()
    if os.path.isdir('/proc/self'):
        for name in os.listdir('/proc'):
            if deadline is not None and time.monotonic() > deadline: break
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
    deadline = time.monotonic() + STOP_SECS
    mark('stop begin')
    owned = {process.pid}
    def send(pid, sig):
        try:
            os.kill(pid, sig)
        except ProcessLookupError:
            pass
    send(process.pid, signal.SIGSTOP)
    try:
        rounds = 0
        while time.monotonic() < deadline:
            rounds += 1
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
            mark('tree round %d owned=%d' % (rounds, len(owned)))
            if not added:
                break
    except (OSError, subprocess.CalledProcessError, ValueError):
        # A restricted host may deny ps. The child still owns a new process
        # group, so kill that group even when descendant enumeration fails.
        pass
    mark('tree done')
    if not TEST_NO_KILL:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        for pid in owned:
            send(pid, signal.SIGKILL)
    mark('killpg + SIGKILL owned')
    # c43: anything in this scope (orphaned, setsid'd or nested) carries the token; freeze, then kill all of them
    for n in range(50):
        if time.monotonic() >= deadline: break
        fresh = scoped_pids(token, deadline) - owned
        if not fresh: break
        owned |= fresh
        for pid in fresh: send(pid, signal.SIGSTOP)
        mark('scope loop %d fresh=%d' % (n + 1, len(fresh)))
    mark('scope loop end')
    if not TEST_NO_KILL:
        for pid in scoped_pids(token, deadline) | owned:
            send(pid, signal.SIGKILL)
    mark('final SIGKILL sweep')
    # Finite reap: poll (WNOHANG through Popen.poll) until the deadline, never an unbounded wait.
    while process.poll() is None and time.monotonic() < deadline:
        time.sleep(0.05)
    mark('reap finished')
    if process.poll() is None:
        state, wchan = describe(process.pid)
        sys.stderr.write('bound: unreaped pid %d (state %s, wchan %s) after %.1fs stop budget\n'
                         % (process.pid, state, wchan, STOP_SECS))
        sys.stderr.flush()
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
    # Kill everything carrying this scope's token.  The cleanup has the same total budget as stop() (defensive,
    # 0b7e7920 covered only the timeout path; 447 s after a rc0 exit was seen, research note: normal-exit tail).
    # A cleanup that runs out of budget is reported and is not green: the command rc is replaced by 142.
    deadline = time.monotonic() + STOP_SECS
    mark('normal exit: cleanup begin')
    for pid in scoped_pids(token, deadline):
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    mark('normal exit: cleanup scan end')
    if time.monotonic() >= deadline:
        sys.stderr.write('bound: cleanup after normal exit unfinished after %.1fs budget (not green)\n' % STOP_SECS)
        sys.stderr.flush()
        return 142
    return rc if rc >= 0 else 128 - rc


if __name__ == '__main__':
    sys.exit(main())
