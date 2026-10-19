#!/usr/bin/env python3
"""r7g static draft of the stall cases (C entry).  Identity comes only from the ledger written by the helper while the
processes were alive: each line is (tag, pid, starttime).  After the run, for every ledger line:
  - an unreadable ledger, or a malformed line, rejects the case;
  - /proc/<pid>/stat missing, or starttime different (pid reused): that process is gone;
  - same pid and starttime: the process is this run's residue, whatever its state (Z is unreaped, not gone).
Every pid in the report must also be in the ledger (cross-check); a report pid missing from the ledger rejects.
The stderr report's start fields are not read.  Not run in this draft.
Known gap: a spawn that dies between fork and its ledger write leaves no line and is not detected here.
Usage: python3 -I tests/boundstall_r7g.py REPO_ROOT [--gate]
"""
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time

REPO = sys.argv[1]
GATE = '--gate' in sys.argv[2:]
if sys.platform != 'linux':
    print('SKIP %s: needs Linux /proc (subreaper, ledger identity, dmesg admission); host is %s'
          % (os.path.basename(__file__), sys.platform), file=sys.stderr)
    sys.exit(77)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from admission_r7p import admit
# Hard deadline = N + TOTAL_EXTRA_SECS; outer-wall bound = hard + CLEANUP_EPSILON_SECS.  Both read from tests/bound.c.
_BOUND_SRC = open(os.path.join(REPO, 'tests', 'bound.c')).read()
TOTAL_EXTRA = float(re.search(r'#define TOTAL_EXTRA_SECS ([0-9.]+)', _BOUND_SRC).group(1))
EPS = float(re.search(r'#define CLEANUP_EPSILON_SECS ([0-9.]+)', _BOUND_SRC).group(1))
def hard(n):
    return n + TOTAL_EXTRA
def bound_max(n):
    return hard(n) + EPS
BOUND = [os.path.join(REPO, 'tests', 'bound')]
HELPER = os.path.join(REPO, 'tests', 'boundstall_ledger.sh')
PS_STALL = os.path.join(REPO, 'tests', 'boundstall_ps_stall_r7g.sh')
PS_FLOOD = os.path.join(REPO, 'tests', 'boundstall_ps_flood_r7g.sh')
WORK = tempfile.mkdtemp(prefix='bound-stall-r7g-')
os.environ['BOUND_CACHE'] = os.path.join(WORK, 'cache')
PID = os.getpid()
N = 2
results = []
def unreaped_pattern(fmt):
    """Regex derived from BOUND_UNREAPED_FMT (tests/bound.c): %s -> reason, %ld -> pid, every other character literal."""
    out = []
    for piece in re.split(r'(%ld|%s)', fmt):
        if piece == '%s': out.append('(?P<reason>.*?)')
        elif piece == '%ld': out.append('(?P<pid>-?\\d+)')
        else: out.append(re.escape(piece))
    return ''.join(out)
_UNREAPED_FMT = re.search(r'#define BOUND_UNREAPED_FMT "([^"]*)"', open(os.path.join(os.path.join(REPO, 'tests'), 'bound.c'), encoding='utf-8').read()).group(1)
REPORT = re.compile(unreaped_pattern(_UNREAPED_FMT) + r' ?(?:unreaped ps pid (?P<ps>-?\d+);)? known tree pids:(?P<tree>(?: -?\d+)+)')
_DEF = dict(re.findall(r'#define BOUND_REASON_(\w+) "([^"]*)"', open(os.path.join(REPO, 'tests', 'bound.c')).read()))
DEADLINE = _DEF['DEADLINE']
ENUM = _DEF['ENUM']


def read_ledger(path):
    """Strict read.  'intent TAG' is written by the parent before a one-shot spawn; 'TAG pid starttime' is written by
    the helper before exec.  Any other line, or an unreadable file, raises: the case rejects instead of passing."""
    rows, intents = [], []
    with open(path, 'r') as f:
        for raw in f:
            parts = raw.rstrip('\n').split(' ')
            if len(parts) == 2 and parts[0] == 'intent' and parts[1]:
                intents.append(parts[1])
                continue
            if len(parts) != 3 or not parts[0] or not parts[1].isdigit() or not parts[2].isdigit():
                raise ValueError('malformed ledger line: %r' % raw)
            rows.append((parts[0], int(parts[1]), int(parts[2])))
    return rows, intents


def identity_state(pid):
    """(starttime, state) of pid now, or None when /proc/<pid> is gone.  Any other read failure raises."""
    try:
        with open('/proc/%d/stat' % pid, 'rb') as f:
            fields = f.read().rsplit(b')', 1)[1].split()
    except (FileNotFoundError, ProcessLookupError):
        return None
    return int(fields[19]), fields[0].decode()


def case(label, ps, command, rc_want, lo, hi):
    root_file = os.path.join(WORK, label + '.root')
    ledger = os.path.join(WORK, label + '.ledger')
    open(ledger, 'w').close()
    env = dict(os.environ, UNISACC_BOUND_PS=ps, BOUNDSTALL_PID=str(PID), BOUNDSTALL_ROOT=root_file,
               BOUNDSTALL_LEDGER=ledger)
    # stdout and stderr go to files, not pipes: an escaped descendant cannot hold a pipe open and stall this harness.
    # The direct child is awaited with a finite timeout; a timeout is a red, never a hang.
    out_path = os.path.join(WORK, label + '.out')
    err_path = os.path.join(WORK, label + '.err')
    if not GATE:
        admit(label)
    t0 = time.monotonic()
    with open(out_path, 'wb') as fo, open(err_path, 'wb') as fe:
        proc = subprocess.Popen(BOUND + [str(N), 'sh', '-c', command], env=env, stdout=fo, stderr=fe)
        try:
            rc_direct = proc.wait(timeout=60)
        except subprocess.TimeoutExpired:
            proc.kill()
            try:
                proc.wait(timeout=10)                 # finite: a child that will not die is itself a red
            except subprocess.TimeoutExpired:
                pass
            rc_direct = None
    dt = time.monotonic() - t0
    stderr_text = open(err_path, 'r', errors='replace').read()
    class _Done:
        pass
    p = _Done()
    p.returncode = rc_direct if rc_direct is not None else -9
    p.stderr = stderr_text
    if rc_direct is None:
        p.stderr += '\n[harness] direct child did not exit within 60 s'
    time.sleep(0.3)
    why = []
    if p.returncode != rc_want:
        why.append('rc=%d want %d' % (p.returncode, rc_want))
    if not (lo <= dt <= hi):
        why.append('took %.2fs, want in [%.2f, %.2f] (an upper-bound check, not a strict N+2)' % (dt, lo, hi))
    if DEADLINE not in p.stderr:
        why.append('report does not name the deadline')
    if ENUM in p.stderr:
        why.append('report names an enumeration failure for a deadline case')
    rows, intents = [], []
    try:
        rows, intents = read_ledger(ledger)
    except (OSError, ValueError) as e:
        rows = []
        why.append('ledger unreadable or malformed: %s' % e)
    if not rows:
        why.append('ledger is empty: identity cannot be bound')
    recorded = {tag for tag, _, _ in rows}
    for tag in intents:
        if tag not in recorded:
            why.append('spawn %s has an intent but no record: lost before its ledger line, identity unbound' % tag)
    m = REPORT.search(p.stderr)
    reported = set()
    if not m:
        why.append('no named report on one line')
    else:
        reported = {int(x) for x in m.group('tree').split()}
        if m.group('pid'):
            reported.add(int(m.group('pid')))
        if m.group('ps'):
            reported.add(int(m.group('ps')))
    ledger_pids = {pid for _, pid, _ in rows}
    missing = sorted(reported - ledger_pids)
    if missing:
        why.append('reported pids not in ledger: %s' % missing)
    # The command shell's pid comes from the ledger itself: its strict 'cmd-root' record, written by the helper before
    # exec.  No separate root file is read, so a case that never wrote one cannot fail for that reason alone.
    roots = [pid for tag, pid, _ in rows if tag == 'cmd-root']
    if len(roots) != 1:
        why.append('command shell (tag cmd-root) recorded %d times, want exactly 1: identity unbound' % len(roots))
    for tag, pid, start in rows:
        try:
            now = identity_state(pid)
        except (OSError, IndexError, ValueError) as e:
            why.append('cannot read /proc/%d/stat (%s): identity cannot be decided' % (pid, e))
            continue
        if now is None:
            continue                                  # gone
        cur_start, state = now
        if cur_start != start:
            continue                                  # pid reused: a different process, not residue
        why.append('ledger process %s pid %d (starttime %d, state %s) still exists' % (tag, pid, start, state))
    ok = not why
    results.append((label, ok))
    print(('PASS ' if ok else 'FAIL ') + '%s rc=%d %.2fs ledger=%d' % (label, p.returncode, dt, len(rows)) +
          (' | ' + '; '.join(why) if why else ''))
    return p


def intent(tag):
    return 'echo "intent %s" >> "$BOUNDSTALL_LEDGER"' % tag


def helper_call(tag, *cmd):
    return 'sh %s %s %s' % (HELPER, tag, shlex.join(cmd))


def spawn_stmt(tag, *cmd):
    """One-shot spawn as two statements in the spawning shell: the parent records the intent, then the helper records
    its own pid and starttime and execs the command.  No argument is ever spliced into another quote level."""
    return '%s; %s' % (intent(tag), helper_call(tag, *cmd))


def root_command(*cmd):
    """The bound child itself becomes the helper: 'exec' keeps the pid, so the 'cmd-root' record is the bound child."""
    return 'exec sh %s cmd-root %s' % (HELPER, shlex.join(cmd))


def root_wrapping(inner):
    """Root running an inner script: exec the helper, which records 'cmd-root' and execs 'sh -c INNER' (same pid).
    The inner script is a single quoted argument, so its own quotes and '$$' are not touched by this level."""
    return 'exec sh %s cmd-root sh -c %s' % (HELPER, shlex.quote(inner))


# Fixture input checklist (each case must produce what its stand-in reads; sh -n only proves the syntax):
#   S1  ps = ps_stall: reads BOUNDSTALL_PID (set by the harness) and BOUNDSTALL_LEDGER (helper).  Needs no root file.
#   S2  ps = ps_flood: reads BOUNDSTALL_ROOT, produced by the inner 'echo $$ > "$BOUNDSTALL_ROOT"'.
#   S3  ps = ps_flood: as S2, the same root producer.
#   setsid  ps = ps_flood: needs BOUNDSTALL_ROOT, produced by the inner echo (added here); without it the flood
#           exits 1 and the stand-in reports an enumeration failure, which this deadline case must not produce.
# S1: ps never answers.  The bound child is the helper itself.
case('S1-ps-stall', PS_STALL, root_command('sleep', '30.%d' % PID), 142, N + 1.5, bound_max(N))

# S2: ps listing never ends; the command shell writes its pid and spawns a foreground sleep through the helper.
case('S2-ps-flood', PS_FLOOD,
     root_wrapping('echo $$ > "$BOUNDSTALL_ROOT"; ' + spawn_stmt('fg', 'sleep', '30.%d' % PID)),
     142, N + 1.5, bound_max(N))

# S3: as S2, with TERM ignored.
case('S3-ps-flood-ignore-TERM', PS_FLOOD,
     root_wrapping('trap "" TERM; echo $$ > "$BOUNDSTALL_ROOT"; ' + spawn_stmt('fg', 'sleep', '30.%d' % PID)),
     142, N + 1.5, bound_max(N))

# setsid grandchild: a background helper spawn escapes the process group; the parent records its intent before '&'.
case('setsid-grandchild', PS_FLOOD,
     root_wrapping('echo $$ > "$BOUNDSTALL_ROOT"; ' + spawn_stmt('bg', 'setsid', 'sleep', '60.%d' % PID)
                   + ' & ' + spawn_stmt('fg', 'sleep', '30.%d' % PID)),
     142, N + 1.5, bound_max(N))

failed = [label for label, ok in results if not ok]
print('SUMMARY stall r7g: %d/%d pass (draft; not run)' % (len(results) - len(failed), len(results)))
sys.exit(1 if failed else 0)
