#!/bin/bash
# gatesummary.sh RESULT_DIR T0 -- the gate summary (0.0.40, 机房主任 20:54): counts only the leading
# "suite rc=N" field of each result file ('%-14s rc=%-3s %3ss :: %s').  Every result file must hold exactly one
# such line and every suite name may appear once; an empty, unreadable, multi-line or malformed file, a duplicate
# suite or no files at all is a failure.  Prints the summary; exits 1 if anything failed, 4 if any unverified
# (77) and none failed, else 0.
set -u
O=${1:?result dir}; T0=${2:-$(date +%s)}
python3 - "$O" "$T0" <<'PY'
import os, re, sys, time
d, t0 = sys.argv[1], int(sys.argv[2])
head = re.compile(r'^(\S+) +rc=(\d+) ')
files = sorted(os.listdir(d)) if os.path.isdir(d) else []
bad = unverified = 0; seen = set(); why = []
for f in files:
    try: lines = open(os.path.join(d, f), encoding='utf-8', errors='replace').read().splitlines()
    except OSError as e: bad += 1; why.append('%s unreadable (%s)' % (f, type(e).__name__)); continue
    if len(lines) != 1: bad += 1; why.append('%s holds %d lines, not 1' % (f, len(lines))); continue
    m = head.match(lines[0])
    if not m: bad += 1; why.append('%s has no leading suite rc field' % f); continue
    name, rc = m.group(1), int(m.group(2))
    if name in seen: bad += 1; why.append('suite %s reported twice' % name); continue
    seen.add(name)
    if rc == 77: unverified += 1
    elif rc != 0: bad += 1
if not files: bad += 1; why.append('no result files (nothing was checked)')
for w in why: print('gate: ' + w)
print('gate  suites %d   failed %d   unverified %d   %ds wall' % (len(files), bad, unverified, int(time.time()) - t0))
sys.exit(1 if bad else 4 if unverified else 0)
PY
