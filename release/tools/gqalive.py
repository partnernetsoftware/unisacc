#!/usr/bin/env python3
"""gqalive.py (0.0.40): print the PIDs of processes RUNNING tests/gatequeue.py; exit 0 if any, 1 if none, 2 if the
process table could not be read (the caller must not take that as "none").

A process runs gatequeue.py when argv[0] is that file, or argv[0] is a python interpreter and the first non-option
argument is that file (options such as -u, -B, -X dev are skipped).  A process whose arguments merely mention the
name -- a shell script text, `notgatequeue.py`, an editor -- does not count."""
import os, re, subprocess, sys
PY = re.compile(r'^python[0-9.]*$')
def runs_gatequeue(argv):
    if not argv: return False
    if os.path.basename(argv[0]) == 'gatequeue.py': return True
    if not PY.match(os.path.basename(argv[0])): return False
    i = 1
    while i < len(argv) and argv[i].startswith('-'):
        if argv[i] in ('-X', '-W'): i += 1   # options with a separate value
        if argv[i] in ('-c', '-m'): return False
        i += 1
    return i < len(argv) and os.path.basename(argv[i]) == 'gatequeue.py'
def table():
    if os.path.isdir('/proc') and os.environ.get('GQALIVE_TABLE') != 'ps':   # GQALIVE_TABLE=ps: the BSD/macOS path (tests)
        rows = []
        for p in os.listdir('/proc'):
            if not p.isdigit(): continue
            try: raw = open('/proc/%s/cmdline' % p, 'rb').read()
            except (FileNotFoundError, ProcessLookupError): continue   # exited meanwhile
            # any other read error (permission, I/O) is UNKNOWN, never "no gatequeue": it propagates to exit 2
            rows.append((int(p), [a.decode(errors='replace') for a in raw.split(b'\0') if a]))
        return rows
    out = subprocess.run(['ps', '-axo', 'pid=,args='], capture_output=True, text=True, timeout=10)
    if out.returncode: raise OSError('ps rc %d' % out.returncode)
    return [(int(l.split(None, 1)[0]), l.split(None, 1)[1].split()) for l in out.stdout.splitlines() if len(l.split(None, 1)) == 2]
try: rows = table()
except Exception as e: print('gqalive: cannot read the process table (%s)' % e, file=sys.stderr); sys.exit(2)
if not rows: print('gqalive: empty process table', file=sys.stderr); sys.exit(2)
me = os.getpid(); hits = [p for p, a in rows if p != me and runs_gatequeue(a)]
for p in hits: print(p)
sys.exit(0 if hits else 1)
