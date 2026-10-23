#!/bin/sh
# K6-9: unisa.fp C reference (seed/unisa_fp.c) vs unisa/fp.py on point samples: op3 (binary64 and
# binary32 arithmetic and compares) and op2 (conversions and sqrt), results as uint64 decimal bit
# patterns, byte-equal; the sample list is generated from python; C double-run; negatives refused by
# both sides with empty stdout (non-numeric argument).  dec_to_f32 is out of the C domain.
# Point-sample equivalence only; not a product closure member; keeps fp.py and bits.py.
# Built with -ffp-contract=off -lm.  First red stops.  usage: tests/unisafpcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisafp.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisafp: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisafp  tree $H"

"$B" 30 cc -std=c99 -O2 -w -ffp-contract=off -o "$T/fp" seed/unisa_fp.c -lm || fail "compile red: seed/unisa_fp.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa import fp
cmd = sys.argv[1]
if cmd == 'op3':
    op, x, y = sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
    print(fp.op3(op, x, y))
elif cmd == 'op2':
    op, x = sys.argv[2], int(sys.argv[3])
    print(fp.op2(op, x))
PY

# sample list: bit patterns from python (binary64 / binary32 operands, NaN, inf, zero, sign cases)
cat > "$T/mk.py" <<'PY'
from unisa import fp
import struct
def b64(f): return fp.bd(f)
def b32(f): return struct.unpack('<I', struct.pack('<f', f))[0]
NAN64 = 0x7FF8000000000001
pts = []
for op in ('fadd64', 'fsub64', 'fmul64', 'fdiv64', 'flt64', 'fle64', 'feq64'):
    for (a, b) in [(1.5, -2.25), (0.0, 0.0), (1.0, 0.0), (-3.5, 7.125), (1e308, 10.0)]:
        pts.append(('op3', op, b64(a), b64(b)))
for op in ('fadd32', 'fsub32', 'fmul32', 'fdiv32', 'flt32', 'fle32', 'feq32'):
    for (a, b) in [(1.5, -2.25), (0.0, -0.0), (1.0, 0.0), (3.0e38, 3.0e38), (0.1, 0.2)]:
        pts.append(('op3', op, b32(a), b32(b)))
pts.append(('op3', 'fadd64', NAN64, b64(1.0)))
for op in ('cvtid', 'cvtud', 'cvtis', 'cvtus', 'cvtdi', 'cvtdu', 'cvtsd', 'cvtds', 'fsqrt64', 'fsqrt32'):
    for x in [b64(-7.0), b64(3.75), b64(-1e30), b64(1e30), (1 << 60) + 12345, (1 << 63) + 1, 1 << 53, 5, (1 << 64) - 3, b32(-2.5), b32(1e-40)]:
        pts.append(('op2', op, x))
for p in pts:
    print(' '.join(str(v) for v in p))
PY
PYTHONPATH="$R" "$B" 20 python3 "$T/mk.py" > "$T/samples.txt" || fail "sample generation (python)"
[ -s "$T/samples.txt" ] || fail "empty sample list"

same=0; total=0
while read -r kind op a b; do
    total=$((total + 1))
    if [ "$kind" = op3 ]; then
        "$B" 10 "$T/fp" op3 "$op" "$a" "$b" >"$T/c.out" 2>"$T/c.err"; c=$?
        "$B" 10 "$T/fp" op3 "$op" "$a" "$b" >"$T/c2.out" 2>/dev/null; c2=$?
        PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" op3 "$op" "$a" "$b" >"$T/p.out" 2>"$T/p.err"; p=$?
    else
        "$B" 10 "$T/fp" op2 "$op" "$a" >"$T/c.out" 2>"$T/c.err"; c=$?
        "$B" 10 "$T/fp" op2 "$op" "$a" >"$T/c2.out" 2>/dev/null; c2=$?
        PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" op2 "$op" "$a" >"$T/p.out" 2>"$T/p.err"; p=$?
    fi
    [ "$c" -eq 0 ] || fail "C refused point $kind $op $a $b rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $kind $op $a $b"
    [ "$p" -eq 0 ] || fail "python refused point $kind $op $a $b: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "DIFF $kind $op $a $b C=$(cat "$T/c.out") py=$(cat "$T/p.out")"; fail "DIFF point"; }
    same=$((same + 1))
done < "$T/samples.txt"
echo "SAME $same of $total points (op3 + op2; binary64/binary32; NaN/inf/zero/sign/saturation)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME KIND OP A B -- both refuse, empty stdout
    nm=$1; kind=$2; op=$3; a=$4; b=${5-}
    if [ "$kind" = op3 ]; then
        "$B" 10 "$T/fp" op3 "$op" "$a" "$b" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
        PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" op3 "$op" "$a" "$b" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    else
        "$B" 10 "$T/fp" op2 "$op" "$a" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
        PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" op2 "$op" "$a" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    fi
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg non-numeric-operand op3 fadd64 abc 1
neg non-numeric-operand-op2 op2 cvtid xyz

echo "---- identity ----"
echo "HEAD $H"
echo "fp.py $(sha256sum unisa/fp.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_fp.c $(sha256sum seed/unisa_fp.c | cut -d' ' -f1)"
echo "unisafpcheck.sh $(sha256sum tests/unisafpcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisafp  same $same  diff 0  neg 2/2"
exit 0
