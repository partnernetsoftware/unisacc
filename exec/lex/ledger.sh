#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# E1 on the C executor: sizes and speed, using stamped exec/lex/run.sh
# preparation and a private independent source export. Every step is bounded.
#   executor __text: exec.c built with cc -Os and the stamped reference
#   table bytes: exec/lex/tbl.py's text table
#   speed: the lexer input buffer of unisacc.c (UA_LEXIN); median of 5 of
#     (wall(N runs) - wall(1 run)) / (N-1), same process, same buffer:
#     executor = run() only (step loop + building the -dump-tokens bytes in
#     memory; table load and output write are in the subtracted run);
#     reference = lex() only (UA_LEXREP; tokens into arrays, no printing).
set -u
cd "$(dirname "$0")/../.."
bound() { "$_BOUND" "$@"; }
. exec/stamp.sh
bound 58 ./exec/lex/run.sh gen || exit 1
B=$(mktemp -d); trap 'rm -rf "$B"' EXIT HUP INT TERM
PRE="$X/ua_pre"; REF="$X/ua_ref"; SELF="$B/unisacc.c"
bound 10 python3 tests/sourceflat.py "$SELF" || exit 1
bound 50 python3 exec/lex/tbl.py "$X/e1delta.json" "$B/e1.tbl" || exit 1
bound 30 cc -std=c99 -Os -w -o $B/exec_cc exec/exec.c || exit 1
bound 30 "$REF" exec/exec.c -o "$B/exec_ua" || exit 1
for x in cc ua; do
  printf 'exec_%s __text ' $x; bound 10 size -m $B/exec_$x | awk '/Section __text/ {print $3}'
done
echo "table bytes $(wc -c < $B/e1.tbl)"
env UA_LEXIN="$B/self.i" "$_BOUND" 10 "$PRE" -dump-tokens "$SELF" || exit 1
echo "input bytes $(wc -c < $B/self.i)"
bound 55 python3 - "$B" "$PRE" "$SELF" <<'EOF'
import os, subprocess, sys, time, statistics
B, PRE, SELF = sys.argv[1:]
def wall(argv, env=None):
    e = dict(os.environ); e.update(env or {})
    a = time.perf_counter()
    subprocess.run([sys.executable, 'tests/bound.py', '15'] + argv, stdout=subprocess.DEVNULL, env=e, check=True)
    return time.perf_counter() - a
n = os.path.getsize(B + '/self.i')
def med(f, N):
    return statistics.median((f(N) - f(1)) / (N - 1) for _ in range(5))
res = {}
for x in ('cc', 'ua'):
    ex = '%s/exec_%s' % (B, x)
    res['exec_' + x] = med(lambda N: wall([ex, '-r', str(N), B + '/e1.tbl', B + '/self.i']), 11)
res['ref_lex'] = med(lambda N: wall([PRE, '-dump-tokens', SELF], {'UA_LEXREP': str(N)}), 21)
for k, s in res.items():
    print('%-9s %8.3f ms/run  %6.1f MB/s' % (k, s * 1e3, n / s / 1e6))
EOF
