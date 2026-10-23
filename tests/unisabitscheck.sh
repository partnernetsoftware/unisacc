#!/bin/sh
# K6-1: unisa.bits C reference (seed/unisa_bits.c) vs unisa/bits.py — same-input pair,
# negative refuse-by-name both sides, C deterministic double-run.  Does not delete bits.py,
# does not change __main__ / gold / construct / emit-kernel / product acceptance / version.h.
# usage: tests/unisabitscheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisabits.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisabits: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisabits  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/bits" seed/unisa_bits.c || fail "unisa_bits.c does not build"

# fixed case table (non-negative; results fit uint64) — identity input closure
cat > "$T/cases.txt" <<'CASES'
0 1
1 1
1 8
7 8
8 8
9 16
0 4096
100 64
15 16
16 16
17 16
1 2
2 2
3 4
4095 4096
4096 4096
4097 4096
18446744073709551615 1
18446744073709551614 2
0 18446744073709551615
CASES

same=0; bad=0
ok() { echo "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; bad=$((bad + 1)); fail "first red: $1"; }   # 1841: first DIFF/NEG failure stops here
refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }

# MASK64
"$B" 10 "$T/bits" mask64 >"$T/c-mask.txt" 2>"$T/c-mask.err"; c=$?
[ "$c" -eq 0 ] || fail "C mask64 rc=$c $(cat "$T/c-mask.err")"
"$B" 10 python3 -c 'from unisa.bits import MASK64; print(MASK64)' >"$T/py-mask.txt" 2>"$T/py-mask.err"; p=$?
[ "$p" -eq 0 ] || fail "Python MASK64 rc=$p $(cat "$T/py-mask.err")"
if cmp -s "$T/c-mask.txt" "$T/py-mask.txt"; then ok "mask64"; else no "mask64"; fi

# round_up batch
"$B" 10 "$T/bits" round-up-batch <"$T/cases.txt" >"$T/c-ru.txt" 2>"$T/c-ru.err"; c=$?
[ "$c" -eq 0 ] || fail "C round-up-batch rc=$c $(cat "$T/c-ru.err")"
"$B" 10 python3 - "$T/cases.txt" <<'PY' >"$T/py-ru.txt" 2>"$T/py-ru.err"
import sys
from unisa.bits import round_up
for line in open(sys.argv[1]):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    v, a = line.split()
    print(round_up(int(v), int(a)))
PY
p=$?
[ "$p" -eq 0 ] || fail "Python round_up rc=$p $(cat "$T/py-ru.err")"
if cmp -s "$T/c-ru.txt" "$T/py-ru.txt"; then ok "round_up-batch ($(wc -l <"$T/cases.txt" | tr -d ' ') cases)"; else no "round_up-batch"; diff -u "$T/py-ru.txt" "$T/c-ru.txt" | head -40; fi

# per-case single round-up (spot identity of CLI form)
i=0
while read -r v a; do
    [ -n "$v" ] || continue
    i=$((i + 1))
    "$B" 10 "$T/bits" round-up "$v" "$a" >"$T/one-c.txt" 2>"$T/one-c.err"; c=$?
    "$B" 10 python3 -c 'from unisa.bits import round_up; import sys; print(round_up(int(sys.argv[1]), int(sys.argv[2])))' "$v" "$a" >"$T/one-py.txt" 2>"$T/one-py.err"; p=$?
    if [ "$c" -eq 0 ] && [ "$p" -eq 0 ] && cmp -s "$T/one-c.txt" "$T/one-py.txt"; then
        :
    else
        no "round-up $v $a (c=$c p=$p)"
    fi
done <"$T/cases.txt"
ok "round-up-singles ($i)"

# deterministic double-run (C batch)
"$B" 10 "$T/bits" round-up-batch <"$T/cases.txt" >"$T/c-ru2.txt" 2>"$T/c-ru2.err"; c=$?
if [ "$c" -eq 0 ] && cmp -s "$T/c-ru.txt" "$T/c-ru2.txt"; then ok "round_up-det"; else no "round_up nondeterministic (rc=$c)"; fi
"$B" 10 "$T/bits" mask64 >"$T/c-mask2.txt" 2>/dev/null
if cmp -s "$T/c-mask.txt" "$T/c-mask2.txt"; then ok "mask64-det"; else no "mask64 nondeterministic"; fi

# negatives: bad width (a=0) — both refuse; C by name "alignment zero"; Python ZeroDivisionError
"$B" 10 "$T/bits" round-up 8 0 >"$T/neg-c.out" 2>"$T/neg-c.err"; c_rc=$?
"$B" 10 python3 -c 'from unisa.bits import round_up; round_up(8, 0)' >"$T/neg-py.out" 2>"$T/neg-py.err"; py_rc=$?
neg0=$bad
refused "$c_rc" || no "NEG-a0 C did not refuse (rc=$c_rc)"
refused "$py_rc" || no "NEG-a0 Python did not refuse (rc=$py_rc)"
grep -q 'alignment zero' "$T/neg-c.err" || no "NEG-a0 C refuse not named ($(tail -1 "$T/neg-c.err"))"
grep -qi 'ZeroDivision\|division by zero' "$T/neg-py.err" || no "NEG-a0 Python refuse not named ($(tail -1 "$T/neg-py.err"))"
[ ! -s "$T/neg-c.out" ] || no "NEG-a0 C wrote stdout"
[ "$bad" -eq "$neg0" ] && ok "NEG-a0 both refuse by name (py_rc=$py_rc c_rc=$c_rc)"

# negatives: bad integer token
"$B" 10 "$T/bits" round-up 8 x >"$T/neg2-c.out" 2>"$T/neg2-c.err"; c_rc=$?
"$B" 10 python3 -c 'from unisa.bits import round_up; round_up(8, int("x"))' >"$T/neg2-py.out" 2>"$T/neg2-py.err"; py_rc=$?
neg0=$bad
refused "$c_rc" || no "NEG-badint C did not refuse (rc=$c_rc)"
refused "$py_rc" || no "NEG-badint Python did not refuse (rc=$py_rc)"
grep -q 'bad integer' "$T/neg2-c.err" || no "NEG-badint C refuse not named"
[ "$bad" -eq "$neg0" ] && ok "NEG-badint both refuse (py_rc=$py_rc c_rc=$c_rc)"

# identity closure (print; not a gate hash lock)
{
    echo "HEAD $H"
    python3 -c 'import hashlib,sys;print("bits.py",hashlib.sha256(open("unisa/bits.py","rb").read()).hexdigest())'
    python3 -c 'import hashlib,sys;print("unisa_bits.c",hashlib.sha256(open("seed/unisa_bits.c","rb").read()).hexdigest())'
    python3 -c 'import hashlib,sys;print("cases",hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$T/cases.txt"
    python3 -c 'import hashlib,sys;print("bits.bin",hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$T/bits"
    cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
} >"$T/identity.txt"
echo "---- identity ----"
cat "$T/identity.txt"

[ "$bad" -eq 0 ] || fail "$bad DIFF (same=$same)"
echo "unisabits  same $same  diff 0"
exit 0
