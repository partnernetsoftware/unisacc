#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Both front ends optimise alike [H1] [H2]: the C front end's -O0 tape, run
# through unisa/opt.py, is the C front end's -O1 / -O2 tape byte for byte --
# on every probe and on the compiler itself.  A decision the optimiser
# takes (the peep table) is asked of the same net on both sides.
# Same net => this checks that the two implementations AGREE, not that the
# peep table is right C (no external referee here).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
SELF="$T/unisacc.flat.c"
if [ "${OPTPY_PART:-all}" != probes ]; then
    bound 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
fi
exec "$_BOUND" 55 python3 - "$UA" "$_BOUND" "$SELF" <<'PY'
import subprocess, sys, glob, os
sys.path.insert(0, '.')
from unisa.opt import optimise
sys.path.insert(0, 'tests')
from knownfail import read as knownfail_read
ua = sys.argv[1]
part = os.environ.get('OPTPY_PART', 'all')
if part not in ('all', 'probes', 'self'): raise SystemExit('invalid OPTPY_PART')
shard = os.environ.get('SHARD', '1/1')
import re
if not re.fullmatch(r'[1-9][0-9]{0,5}/[1-9][0-9]{0,5}', shard): raise SystemExit('invalid SHARD')
k, n = map(int, shard.split('/'))
if k > n: raise SystemExit('invalid SHARD')
refused = set(knownfail_read('tests/difftest.knownfail'))  # reference refuses on purpose
probes = sorted(f for f in glob.glob('examples/*.c') + glob.glob('tests/c/*.c') if os.path.basename(f)[:-2] not in refused)
files = (probes[k-1::n] if part != 'self' else []) + ([sys.argv[3]] if part != 'probes' else [])
if not files: raise SystemExit('empty optpy partition')
def tape(args):
    p = subprocess.run([sys.argv[2], '15', ua] + args,
                       capture_output=True, timeout=17)
    if p.returncode != 0 or not p.stdout:
        raise RuntimeError('tape failed/empty: %r rc=%d %s' % (args, p.returncode, p.stderr.decode('latin-1')))
    return p.stdout.decode('latin-1')
same = diff = 0
for f in files:
    base = tape([f, '-t', 'osx/arm64'])
    for lvl in (1, 2):
        want = tape(['-O%d' % lvl, f, '-t', 'osx/arm64'])
        if optimise(base, lvl) == want:
            same += 1
        else:
            diff += 1
            print('  DIFF %s -O%d' % (f, lvl))
print()
print('optpy  agree %d   differ %d' % (same, diff))
sys.exit(0 if diff == 0 and same > 0 else 1)
PY
