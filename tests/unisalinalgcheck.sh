#!/bin/sh
# K6-5: unisa.linalg C reference (seed/unisa_linalg.c) vs unisa/linalg.py on scripted point samples:
# gemv (ascending accumulation), relu, argmax (ties to lowest index), margin.  Output floats are
# compared as %.17g text (byte-equal); C double-run; negatives refused by both sides with empty
# stdout.  Point-sample equivalence only; not a product closure member; keeps linalg.py.
# Built with -ffp-contract=off.  First red stops.  usage: tests/unisalinalgcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisalinalg.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisalinalg: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisalinalg  tree $H"

"$B" 30 cc -std=c99 -O2 -w -ffp-contract=off -o "$T/linalg" seed/unisa_linalg.c || fail "compile red: seed/unisa_linalg.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa.linalg import gemv, relu, argmax, margin
for line in open(sys.argv[1]):
    t = line.split()
    if not t or t[0].startswith('#'):
        continue
    op = t[0]
    if op == 'gemv':
        nin, nout = int(t[1]), int(t[2]); toks = t[3:]
        x = [float(v) for v in toks[0:nin]]
        W = [float(v) for v in toks[nin:nin + nin * nout]]
        b = [float(v) for v in toks[nin + nin * nout:nin + nin * nout + nout]]
        y = gemv(x, W, b, nin, nout)
        for v in y:
            print('%.17g' % v)
    elif op == 'relu':
        n = int(t[1]); v = [float(q) for q in t[2:2 + n]]
        for w in relu(v):
            print('%.17g' % w)
    elif op == 'argmax':
        n = int(t[1]); v = [float(q) for q in t[2:2 + n]]
        print(argmax(v))
    elif op == 'margin':
        n = int(t[1]); v = [float(q) for q in t[2:2 + n]]
        print('%.17g' % margin(v))
    else:
        raise SystemExit('unknown op ' + op)
PY

mkdir -p "$T/scripts"
cat > "$T/scripts/A.ops" <<'OPS'
gemv 3 2 1.5 -2 0.25 0.1 0.2 0.3 -0.4 0.5 0.6 0.01 -0.02
relu 5 -0.0 0 1e-300 -1e300 2.5
argmax 5 1 4 4 0 -1
margin 4 0.3 0.1 0.3 0.2
OPS
cat > "$T/scripts/B.ops" <<'OPS'
gemv 4 3 0.1 0.7 -0.3 1.9 0.11 0.22 0.33 0.44 0.55 0.66 0.77 0.88 -0.99 1.01 1.2 -1.3 0.123456789 -0.987654321 0.5
argmax 3 2 2 2
margin 3 -1 -2 -3
OPS
cat > "$T/scripts/C.ops" <<'OPS'
gemv 0 2 0.5 -0.25
gemv 1 1 3 0.125 0.0625
relu 0
argmax 4 -5 -4 -4.5 -4
margin 2 1 1
OPS

same=0
pt() {
    nm=$1; f=$T/scripts/$nm.ops
    "$B" 20 "$T/linalg" "$f" >"$T/c-$nm.txt" 2>"$T/c-$nm.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused script $nm rc=$c $(cat "$T/c-$nm.err")"
    "$B" 20 "$T/linalg" "$f" >"$T/c2-$nm.txt" 2>/dev/null || fail "C second run rc!=0 $nm"
    cmp -s "$T/c-$nm.txt" "$T/c2-$nm.txt" || fail "C not deterministic $nm"
    PYTHONPATH="$R" "$B" 20 python3 "$T/pydump.py" "$f" >"$T/p-$nm.txt" 2>"$T/p-$nm.err" || fail "python refused script $nm: $(tail -1 "$T/p-$nm.err")"
    cmp -s "$T/c-$nm.txt" "$T/p-$nm.txt" || { diff -u "$T/p-$nm.txt" "$T/c-$nm.txt" | head -30; fail "DIFF script $nm"; }
    same=$((same + 1)); echo "SAME $nm ($(wc -l <"$T/c-$nm.txt" | tr -d ' ') lines)"
}
pt A; pt B; pt C

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME LINE -- both sides refuse, stdout empty on both
    nm=$1; line=$2; f=$T/scripts/neg-$nm.ops
    printf '%s\n' "$line" >"$f"
    "$B" 10 "$T/linalg" "$f" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$f" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-$nm-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
neg argmax-empty "argmax 0"
neg gemv-short-x "gemv 3 2 1 2"

echo "---- identity ----"
echo "HEAD $H"
echo "linalg.py $(sha256sum unisa/linalg.py | cut -d' ' -f1)"
echo "unisa_linalg.c $(sha256sum seed/unisa_linalg.c | cut -d' ' -f1)"
echo "unisalinalgcheck.sh $(sha256sum tests/unisalinalgcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisalinalg  same $same  diff 0  neg 2/2"
exit 0
