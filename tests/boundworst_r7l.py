#!/usr/bin/env python3
"""Worst-path cases for the cleanup, split into a positive and a negative control (r7l).

  clean  (positive control): the cleanup finishes before the hard deadline.  No deadline report is expected.
         Pass = rc 142 (the command's own timeout), no 'hard deadline reached' on stderr, wall <= bound_max(N),
         no residue, and the trace ends in 'scan_end clean'.
  forced (negative control): many escaped setsid descendants press the cleanup against the hard deadline.
         Pass = a REAL deadline report on stderr ('hard deadline reached'), rc 142 (not green), wall <= bound_max(N),
         no residue.  If the report is absent the control did not trigger and the case is red.

The upper bound is bound_max(N) = hard deadline + CLEANUP_EPSILON_SECS, both read from tests/bound.c.  The four
CLEANUP_TRACE marks (kill, scan_start, scan_end, exit) are written to a file for every case.
Usage: python3 -I tests/boundworst_r7l.py REPO_ROOT {clean|forced}
"""
import os
import re
import subprocess
import sys
import tempfile
import time

REPO = sys.argv[1]
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from admission_r7p import admit
MODE = sys.argv[2]
BOUND = [os.path.join(REPO, 'tests', 'bound')]
N = 2
PID = os.getpid()
WORK = tempfile.mkdtemp(prefix='bound-worst-r7l-')
os.environ['BOUND_CACHE'] = os.path.join(WORK, 'cache')
_BOUND_SRC = open(os.path.join(REPO, 'tests', 'bound.c')).read()
TOTAL_EXTRA = float(re.search(r'#define TOTAL_EXTRA_SECS ([0-9.]+)', _BOUND_SRC).group(1))
EPS = float(re.search(r'#define CLEANUP_EPSILON_SECS ([0-9.]+)', _BOUND_SRC).group(1))
def hard(n):
    return n + TOTAL_EXTRA
def bound_max(n):
    return hard(n) + EPS

COUNT = {'clean': 3000, 'forced': 12000}[MODE]
script = ('i=0; while [ $i -lt %d ]; do setsid sleep 60.%d & i=$((i+1)); done; sleep 30.%d'
          % (COUNT, PID, PID))
trace_path = os.path.join(WORK, 'trace.txt')
out_path = os.path.join(WORK, 'out')
err_path = os.path.join(WORK, 'err')
env = dict(os.environ, UNISACC_BOUND_CLEANUP_TRACE=trace_path)
admit('worst-' + MODE)
t0 = time.monotonic()
with open(out_path, 'wb') as fo, open(err_path, 'wb') as fe:
    proc = subprocess.Popen(BOUND + [str(N), 'sh', '-c', script], env=env, stdout=fo, stderr=fe)
    try:
        rc = proc.wait(timeout=60)
    except subprocess.TimeoutExpired:
        proc.kill()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass
        rc = None
wall = time.monotonic() - t0
err = open(err_path, 'r', errors='replace').read()
marks = []
if os.path.exists(trace_path):
    for line in open(trace_path):
        parts = line.split(None, 1)
        if len(parts) == 2:
            marks.append((float(parts[0]), re.sub(r'^pid=\d+ ', '', parts[1].strip())))   # r4: every line carries pid=
left = []
for d in os.listdir('/proc'):
    if not d.isdigit():
        continue
    try:
        cmd = open('/proc/%s/cmdline' % d, 'rb').read().replace(b'\0', b' ')
    except OSError:
        continue
    if cmd.startswith(b'sleep 60.%d' % PID) or cmd.startswith(b'sleep 30.%d' % PID):
        left.append(int(d))
why = []
if rc != 142:
    why.append('rc=%s want 142 (not green)' % rc)
if wall > bound_max(N):
    why.append('wall %.3fs exceeds bound_max(N)=%.3fs (hard %.3f + epsilon %.2f)' % (wall, bound_max(N), hard(N), EPS))
has_deadline = 'hard deadline reached' in err
if MODE == 'clean' and has_deadline:
    why.append('clean control produced a deadline report: the cleanup did not finish before the deadline')
if MODE == 'forced' and not has_deadline:
    why.append('forced control did not trigger: no real deadline report')
if left:
    why.append('residue pids of this run still alive: %s' % left)
if MODE == 'clean' and marks and marks[-1][1] != 'exit normal' and 'scan_end clean' not in ' '.join(m[1] for m in marks):
    why.append('trace does not show scan_end clean')
print(('PASS ' if not why else 'FAIL ') + 'worst-%s rc=%s wall=%.3fs (bound_max %.3fs) escaped=%d deadline_report=%s marks=%s'
      % (MODE, rc, wall, bound_max(N), COUNT, has_deadline, [m[1] for m in marks]) +
      (' | ' + '; '.join(why) if why else ''))
sys.exit(0 if not why else 1)
