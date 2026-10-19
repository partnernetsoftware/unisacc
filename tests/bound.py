#!/usr/bin/env python3
"""Bound one command and its descendants. Test infrastructure only."""
import math
import os
import re
import secrets
import signal
import subprocess
import sys
import time
CHAIN = "UNISACC_BOUND_CHAIN"
ROOT = os.path.dirname(os.path.abspath(__file__))
def c_constant(name):
    """A string #define from tests/bound.c: the single spelling shared with the C binary and the parsers."""
    text = open(os.path.join(ROOT, 'bound.c'), encoding='utf-8').read()
    m = re.search(r'#define %s "([^"]*)"' % re.escape(name), text)
    if not m: raise SystemExit('bound: %s missing from bound.c (not green)' % name)
    return m.group(1)
BOUND_UNREAPED_FMT = c_constant('BOUND_UNREAPED_FMT')
BOUND_REASON_DEADLINE = c_constant('BOUND_REASON_DEADLINE')
# Defensive (research/c43-bound-stop-889s-readonly.md): stop() has a total budget, so a child that cannot be
# reaped cannot hold the wall clock.  The root cause of the 889 s case is NOT located; this only makes the
# cleanup finite and visible.  Outcome is unchanged: a timeout is always 142, and an unreaped child is never green.
STOP_SECS = float(os.environ.get('UNISACC_BOUND_STOP_SECS', '10'))
# One absolute deadline covers command wait plus cleanup, not N then another ten seconds.
# With scale=1, the scheduled process budget is N+2s (external startup allowance: 1s).
TOTAL_EXTRA_SECS = 2.0
# Shared hard deadline across nested bounds: an absolute time.monotonic() value (same machine), passed down.
PARENT_DEADLINE = 'UNISACC_BOUND_DEADLINE_MONO'
PS_CALL_SECS = 0.5
_TOTAL_DEADLINE = None

class CleanupTimeout(Exception):
    pass

def cleanup_deadline():
    local = time.monotonic() + STOP_SECS
    return local if _TOTAL_DEADLINE is None else min(local, _TOTAL_DEADLINE)

def ps_output(argv, deadline):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise CleanupTimeout('cleanup budget exhausted before ps')
    try:
        return subprocess.check_output(argv, text=True, stderr=subprocess.DEVNULL,
                                       timeout=min(PS_CALL_SECS, remaining))
    except subprocess.TimeoutExpired:
        raise CleanupTimeout('cleanup ps timed out') from None

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
        out = ps_output(['ps', '-axwwE', '-o', 'pid=,command='],
                        cleanup_deadline() if deadline is None else deadline)
        for line in out.splitlines():
            if CHAIN + '=' in line and token in line:
                found.add(int(line.split(None, 1)[0]))
    found.discard(os.getpid())
    return found

def stop(process, code, token=None):
    # Cleanup must not be interrupted after descendants have been stopped.
    for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        signal.signal(sig, signal.SIG_IGN)
    deadline = cleanup_deadline()
    effective_stop_budget = max(0.0, deadline - time.monotonic())
    failure = None
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
            rows = ps_output(['/bin/ps', '-axo', 'pid=,ppid='], deadline).splitlines()
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
    except CleanupTimeout as error:
        failure = str(error)
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
        try:
            fresh = scoped_pids(token, deadline) - owned
        except CleanupTimeout as error:
            failure = failure or str(error)
            break
        if not fresh: break
        owned |= fresh
        for pid in fresh: send(pid, signal.SIGSTOP)
        mark('scope loop %d fresh=%d' % (n + 1, len(fresh)))
    mark('scope loop end')
    if not TEST_NO_KILL:
        try:
            final = scoped_pids(token, deadline)
        except CleanupTimeout as error:
            failure = failure or str(error)
            final = set()
        for pid in final | owned:
            send(pid, signal.SIGKILL)
    mark('final SIGKILL sweep')
    # Finite reap: poll (WNOHANG through Popen.poll) until the deadline, never an unbounded wait.
    while process.poll() is None and time.monotonic() < deadline:
        time.sleep(0.05)
    mark('reap finished')
    if process.poll() is None:
        state, wchan = describe(process.pid) if time.monotonic() < deadline else ('?', '?')
        fmt = BOUND_UNREAPED_FMT.replace('%ld', '%d')
        sys.stderr.write((fmt % (BOUND_REASON_DEADLINE, process.pid)) + ' state %s, wchan %s\n' % (state, wchan))
        sys.stderr.flush()
    if failure:
        sys.stderr.write('bound: '+failure+' (not green)\n')
        sys.stderr.flush()
        return 142
    return code


def scale():
    """0.0.27 C1: GitHub's hosted runners are 2-4x slower than this machine.  Under GITHUB_ACTIONS,
    UNISACC_TIME_SCALE (1..5) multiplies every bound, so the scripts' own 20/46/55 s limits stretch
    with the runner instead of timing out.  Locally the scale is always 1 (the 60 s rule holds)."""
    if os.environ.get('GITHUB_ACTIONS') != 'true': return 1
    v = os.environ.get('UNISACC_TIME_SCALE', '1')
    return int(v) if v.isdigit() and 1 <= int(v) <= 5 else 1


def windows_main(command_budget):
    # Windows owns descendants through a kill-on-close Job Object; no POSIX ps/signals.
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    class BASIC(ctypes.Structure):
        _fields_=[('ProcessTime',ctypes.c_longlong),('JobTime',ctypes.c_longlong),
                  ('LimitFlags',wintypes.DWORD),('MinWorking',ctypes.c_size_t),
                  ('MaxWorking',ctypes.c_size_t),('ActiveLimit',wintypes.DWORD),
                  ('Affinity',ctypes.c_size_t),('Priority',wintypes.DWORD),('Scheduling',wintypes.DWORD)]
    class IO(ctypes.Structure):
        _fields_=[(n,ctypes.c_ulonglong) for n in ('ReadOps','WriteOps','OtherOps','ReadBytes','WriteBytes','OtherBytes')]
    class EXTENDED(ctypes.Structure):
        _fields_=[('Basic',BASIC),('IO',IO),('ProcessMemory',ctypes.c_size_t),
                  ('JobMemory',ctypes.c_size_t),('PeakProcess',ctypes.c_size_t),('PeakJob',ctypes.c_size_t)]
    kernel.CreateJobObjectW.argtypes=[ctypes.c_void_p,wintypes.LPCWSTR];kernel.CreateJobObjectW.restype=wintypes.HANDLE
    kernel.SetInformationJobObject.argtypes=[wintypes.HANDLE,ctypes.c_int,ctypes.c_void_p,wintypes.DWORD]
    kernel.AssignProcessToJobObject.argtypes=[wintypes.HANDLE,wintypes.HANDLE]
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    job=kernel.CreateJobObjectW(None,None)
    limits=EXTENDED();limits.Basic.LimitFlags=0x2000 # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
    if not job or not kernel.SetInformationJobObject(job,9,ctypes.byref(limits),ctypes.sizeof(limits)):
        if job:kernel.CloseHandle(job)
        print('bound: Windows Job Object setup failed',file=sys.stderr);return 2
    process=None
    def kill_tree():
        kernel.CloseHandle(job) # kills every assigned descendant, including reparented children
        if process is not None and process.poll() is None:
            try:subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=1)
            except subprocess.TimeoutExpired:print('bound: Windows taskkill timed out (not green)',file=sys.stderr)
    try:
        process=subprocess.Popen(sys.argv[2:],env=dict(os.environ),creationflags=subprocess.CREATE_NEW_PROCESS_GROUP | 0x4) # CREATE_SUSPENDED: assign job before any child can spawn
        if not kernel.AssignProcessToJobObject(job,wintypes.HANDLE(process._handle)):
            print('bound: Windows Job Object assignment failed (not green)',file=sys.stderr)
            kill_tree();job=None;return 2
        # Popen closes the primary thread handle; enumerate its suspended thread through Win32.
        class THREADENTRY(ctypes.Structure):
            _fields_=[('size',wintypes.DWORD),('usage',wintypes.DWORD),('tid',wintypes.DWORD),
                      ('pid',wintypes.DWORD),('priority',wintypes.LONG),('delta',wintypes.LONG),('flags',wintypes.DWORD)]
        kernel.CreateToolhelp32Snapshot.argtypes=[wintypes.DWORD,wintypes.DWORD];kernel.CreateToolhelp32Snapshot.restype=wintypes.HANDLE
        kernel.Thread32First.argtypes=[wintypes.HANDLE,ctypes.POINTER(THREADENTRY)]
        kernel.Thread32Next.argtypes=[wintypes.HANDLE,ctypes.POINTER(THREADENTRY)]
        kernel.OpenThread.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD];kernel.OpenThread.restype=wintypes.HANDLE
        kernel.ResumeThread.argtypes=[wintypes.HANDLE];kernel.ResumeThread.restype=wintypes.DWORD
        snapshot=kernel.CreateToolhelp32Snapshot(4,0);resumed=False
        if snapshot and snapshot!=ctypes.c_void_p(-1).value:
            entry=THREADENTRY();entry.size=ctypes.sizeof(entry)
            available=kernel.Thread32First(snapshot,ctypes.byref(entry))
            while available and time.monotonic()<_TOTAL_DEADLINE:
                if entry.pid==process.pid:
                    thread=kernel.OpenThread(2,False,entry.tid) # THREAD_SUSPEND_RESUME
                    if thread:
                        resumed=kernel.ResumeThread(thread)!=0xffffffff;kernel.CloseHandle(thread)
                    break
                available=kernel.Thread32Next(snapshot,ctypes.byref(entry))
            kernel.CloseHandle(snapshot)
        if not resumed:
            print('bound: Windows suspended process resume failed (not green)',file=sys.stderr)
            kill_tree();job=None;return 2
        def interrupted(sig,frame):
            raise SystemExit(128+sig)
        signal.signal(signal.SIGTERM,interrupted);signal.signal(signal.SIGINT,interrupted)
        remaining=max(0.0,_TOTAL_DEADLINE-TOTAL_EXTRA_SECS-time.monotonic())
        try:rc=process.wait(timeout=remaining)
        except subprocess.TimeoutExpired:
            print('bound: Windows process tree timeout',file=sys.stderr);rc=142
        kill_tree();job=None
        try:process.wait(timeout=max(0.01,min(1,_TOTAL_DEADLINE-time.monotonic())))
        except subprocess.TimeoutExpired:
            print('bound: Windows process tree unreaped (not green)',file=sys.stderr);return 142
        return rc if rc>=0 else 128-rc
    finally:
        if job:kill_tree()


def delegate_target(environ=None):
    """Verified ready C binary when this call may run in C: Linux, and none of the Python-only
    switches is set (test injections, the legacy BOUND_TRACE).  Otherwise None: the Python path runs unchanged."""
    env = os.environ if environ is None else environ
    if sys.platform != 'linux': return None
    if 'UNISACC_BOUND_TEST_NO_KILL' in env or 'UNISACC_BOUND_STOP_SECS' in env or 'BOUND_TRACE' in env: return None
    # Readiness is read-only: never call the wrapper, take a lock, or build here.
    import hashlib, pathlib, shutil, stat
    sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))  # same-directory helpers also under -I
    from checklib import verify_cached_file
    compiler = shutil.which((env.get('CC') or 'cc'), path=env.get('PATH'))
    if compiler is None: return None
    try:
        source = pathlib.Path(ROOT, 'bound.c').read_bytes()
        key = hashlib.sha256((compiler + '\n' + os.uname().machine + '\n').encode() + source).hexdigest()
        cache = (env.get('BOUND_CACHE') or os.path.join(env.get('TMPDIR') or '/tmp', 'unisacc-bound-%d' % os.getuid()))
        binary = pathlib.Path(cache, key)
        certificate = pathlib.Path(str(binary) + '.sha256')
        if not stat.S_ISREG(certificate.lstat().st_mode): return None
        fields = certificate.read_text(encoding='ascii').split()
        if len(fields) != 2 or fields[0] != key: return None
        verify_cached_file(binary, fields[1], executable=True)
        return str(binary)  # exec the verified ready binary, never the building wrapper
    except (OSError, ValueError, UnicodeError):
        return None  # absent, unreadable, malformed, or damaged => original Python path


def parent_deadline():
    """The inherited absolute hard deadline, or None.  Non-finite or non-positive values refuse to start."""
    raw = os.environ.get(PARENT_DEADLINE)
    if raw is None:
        return None
    try:
        value = float(raw)
    except ValueError:
        value = float('nan')
    if not math.isfinite(value) or value <= 0:
        sys.stderr.write('bound: bad %s=%r, refusing to start (not green)\n' % (PARENT_DEADLINE, raw))
        raise SystemExit(2)
    return value


def main():
    if len(sys.argv) < 3 or not sys.argv[1].isdigit() or not 1 <= int(sys.argv[1]) <= 60:
        print('bound: timeout must be 1..60 seconds, followed by a command', file=sys.stderr)
        return 2
    target = delegate_target()
    if target is not None:
        os.execv(target, [target] + sys.argv[1:])   # same PID; N and UNISACC_BOUND_DEADLINE_MONO unchanged
    global _TOTAL_DEADLINE
    command_budget = int(sys.argv[1]) * scale()
    _TOTAL_DEADLINE = time.monotonic() + command_budget + TOTAL_EXTRA_SECS
    if os.name == 'nt':return windows_main(command_budget)
    token = secrets.token_hex(8)
    env = dict(os.environ)
    env[CHAIN] = (env.get(CHAIN, "") + " " + token).strip()
    parent = parent_deadline()
    if parent is not None:
        if parent <= time.monotonic():
            sys.stderr.write('bound: inherited deadline already passed, refusing to start (not green)\n')
            return 142
        _TOTAL_DEADLINE = min(_TOTAL_DEADLINE, parent)
    env[PARENT_DEADLINE] = repr(_TOTAL_DEADLINE)
    process = subprocess.Popen(sys.argv[2:], start_new_session=True, env=env)
    def interrupted(sig, frame):
        raise SystemExit(stop(process, 128 + sig, token))
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    signal.signal(signal.SIGHUP, interrupted)
    try:
        remaining = max(0.0, _TOTAL_DEADLINE - TOTAL_EXTRA_SECS - time.monotonic())
        rc = process.wait(timeout=remaining)
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
    deadline = cleanup_deadline()
    mark('normal exit: cleanup begin')
    try:
        for pid in scoped_pids(token, deadline):
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
    except CleanupTimeout as error:
        sys.stderr.write('bound: '+str(error)+' (not green)\n')
        return 142
    mark('normal exit: cleanup scan end')
    if time.monotonic() >= deadline:
        sys.stderr.write('bound: cleanup after normal exit unfinished after %.1fs budget (not green)\n' % STOP_SECS)
        sys.stderr.flush()
        return 142
    return rc if rc >= 0 else 128 - rc


if __name__ == '__main__':
    sys.exit(main())
