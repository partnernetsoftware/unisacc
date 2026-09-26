#!/bin/sh
# E0 artefact ledger: executor machine-code bytes (text), file bytes, delta
# table bytes, speed.  Run check.sh first (it builds build/exec_cc, exec_ua).
# Speed = in-process runs/sec from `exec -r N` minus `exec -r 1`, median of 5.
set -u
cd "$(dirname "$0")"
B=build
bound() { perl -e 'alarm shift; exec @ARGV' "$@"; }
for x in cc ua; do
  [ -x $B/exec_$x ] || { echo "run check.sh first"; exit 1; }
  echo "== exec_$x"
  bound 10 size -m $B/exec_$x | grep -E 'Section __text|Segment __TEXT|Segment __DATA|total' | head -8
  echo "file bytes: $(wc -c < $B/exec_$x)"
done
echo "== delta table: toy/toy.tbl $(wc -c < toy/toy.tbl) bytes text; rows $(sed -n 7p toy/toy.tbl)"
printf '2*3*4+5*(6+7)' > $B/bench1
cp $B/cases/c24 $B/bench2          # 200-deep parentheses around 1
bound 50 python3 - "$B" <<'EOF'
import subprocess, sys, time, statistics
B = sys.argv[1]
def t(exe, inp, n):
    a = time.perf_counter()
    subprocess.run(['perl', '-e', 'alarm 20; exec @ARGV', exe, '-r', str(n), 'toy/toy.tbl', inp],
                   stdout=subprocess.DEVNULL, check=True)
    return time.perf_counter() - a
for x in ('cc', 'ua'):
    for inp, n in (('bench1', 20000), ('bench2', 2000)):
        exe = '%s/exec_%s' % (B, x); f = '%s/%s' % (B, inp)
        rs = []
        for _ in range(5):
            rs.append((n - 1) / (t(exe, f, n) - t(exe, f, 1)))
        proc = statistics.median([t(exe, f, 1) for _ in range(5)])
        print('exec_%s %s (%d B): %.0f inputs/s in-process (median of 5); '
              'one process incl. load %.1f ms' % (x, inp, len(open(f, 'rb').read()),
                                                  statistics.median(rs), proc * 1000))
EOF
