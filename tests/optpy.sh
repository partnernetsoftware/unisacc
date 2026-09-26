#!/bin/bash
# Both front ends optimise alike [H1] [H2]: the C front end's -O0 tape, run
# through unisa/opt.py, is the C front end's -O1 / -O2 tape byte for byte --
# on every probe and on the compiler itself.  A decision the optimiser
# takes (the peep table) is asked of the same net on both sides.
# Same net => this checks that the two implementations AGREE, not that the
# peep table is right C (no external referee here).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
exec perl -e 'alarm 55; exec @ARGV' python3 - "$UA" <<'PY'
import subprocess, sys, glob
sys.path.insert(0, '.')
from unisa.opt import optimise
ua = sys.argv[1]
files = sorted(glob.glob('examples/*.c') + glob.glob('tests/c/*.c')) + ['unisacc.c']
same = diff = 0
for f in files:
    base = subprocess.run([ua, f, '-t', 'osx/arm64'], capture_output=True).stdout.decode('latin-1')
    for lvl in (1, 2):
        want = subprocess.run([ua, '-O%d' % lvl, f, '-t', 'osx/arm64'], capture_output=True).stdout.decode('latin-1')
        if optimise(base, lvl) == want:
            same += 1
        else:
            diff += 1
            print('  DIFF %s -O%d' % (f, lvl))
print()
print('optpy  agree %d   differ %d' % (same, diff))
sys.exit(0 if diff == 0 and same > 0 else 1)
PY
