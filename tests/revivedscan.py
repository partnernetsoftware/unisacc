#!/usr/bin/env python3
"""revivedscan: run every probe a knownfail list excuses and report the ones that now agree (0.0.25 P2).

0.0.24 lost a queue round to this: a front-end fix made a_externarr and the unary-plus probe agree,
both were still listed in ccrun.knownwrong, and the release queue reported them as REVIVED (red) at
161/568.  This checks only the excused probes, so it is cheap enough to run on every front-end edit:
  tests/revivedscan.py            scan; exit 1 when an entry is revived (drop it from the list)
  tests/revivedscan.py --changed  scan only when HEAD~1..worktree touches unisa/front or src/front_*
Python-route lists run `python3 -m unisa run`; product lists run ./unisacc.com -run.  Each probe is
bounded; the reference is the system cc.
"""
import os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
BOUND = [str(ROOT / 'tests/bound')]
LISTS = [('tests/pyfront.knownfail', 'python'), ('tests/ccrun.knownwrong', 'python'),
         ('tests/difftest.knownfail', 'python'), ('tests/difftest.com.knownfail', 'product'),
         ('tests/product-refusals.knownfail', 'product')]

def entries(path):
    for line in (ROOT / path).read_text().splitlines():
        if line.strip() and not line.lstrip().startswith('#'):
            yield line.split()[0].removesuffix('.c')

def run(cmd, cwd, t=15):
    try:
        r = subprocess.run(BOUND + [str(t)] + cmd, cwd=cwd, capture_output=True, timeout=t + 5)
        return r.returncode, r.stdout
    except subprocess.TimeoutExpired:
        return 142, b''

def main():
    if '--changed' in sys.argv:
        diff = subprocess.run(['git', 'diff', '--name-only', 'HEAD~1'], cwd=ROOT, capture_output=True, text=True).stdout.split()
        if not any(p.startswith(('unisa/front/', 'src/front_')) for p in diff):
            print('revivedscan: no front-end change since HEAD~1; nothing to scan'); return 0
    revived = []; checked = 0
    with tempfile.TemporaryDirectory(prefix='revivedscan-') as td:
        for path, route in LISTS:
            for name in entries(path):
                src = ROOT / 'tests/c' / (name + '.c')
                try: src.stat()
                except FileNotFoundError: continue
                exe = pathlib.Path(td) / name
                if subprocess.run(['cc', '-w', '-I', str(src.parent), '-o', str(exe), str(src)], capture_output=True).returncode:
                    continue
                want = run([str(exe)], td)
                got = run(['python3', '-m', 'unisa', 'run', str(src), '--drive', 'built'] if route == 'python'
                          else [str(ROOT / 'unisacc.com'), '-run', str(src)], td)
                checked += 1
                if got == want:
                    revived.append((path, name)); print('  REVIVED %-40s now agrees -- drop it from %s' % (name, path))
    print('revivedscan  excused probes run %d   revived %d' % (checked, len(revived)))
    return 1 if revived else 0

if __name__ == '__main__':
    os.environ.setdefault('PYTHONPATH', str(ROOT))
    sys.exit(main())
