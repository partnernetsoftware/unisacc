#!/bin/sh
# K6-7: unisa.control.uns1 C reference (seed/unisa_uns1.c) vs unisa/control/uns1.py on point samples:
# dumps (i8 and f32) byte-equal to python dumps; loads (header, tensors, weights as %.17g) equal to
# python loads on the same blob; _pad4 lengths.  C double-run; negatives (bad MAGIC, truncated header,
# truncated tensor payload) refused by both sides with empty stdout.  Point-sample equivalence only;
# not a product closure member; keeps uns1.py.  Built with -ffp-contract=off -lm.  First red stops.
# usage: tests/unisauns1check.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisauns1.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisauns1: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisauns1  tree $H"

"$B" 30 cc -std=c99 -O2 -w -ffp-contract=off -o "$T/uns1" seed/unisa_uns1.c -lm || fail "compile red: seed/unisa_uns1.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa.control import uns1 as U
cmd = sys.argv[1]
if cmd == 'dumps':
    dtype, flags, acc, spec = int(sys.argv[2]), int(sys.argv[3]), float(sys.argv[4]), sys.argv[5]
    ts = []
    for line in open(spec):
        t = line.split()
        if not t or t[0].startswith('#'):
            continue
        name, rows, cols = t[0], int(t[1]), int(t[2])
        w = [float(v) for v in t[3:]]
        ts.append((name, rows, cols, w))
    blob = U.dumps(ts, dtype=dtype, acc=acc, flags=flags)
    print(blob.hex())
elif cmd == 'loads':
    blob = bytes.fromhex(sys.argv[2])
    d = U.loads(blob)
    print('HEADER %d %d %d %u %.17g' % (d['dtype'], d['flags'], len(d['tensors']),
          sum(len(w) for _, _, _, w in d['tensors']), d['acc']))
    for name, rows, cols, w in d['tensors']:
        print('T %s %d %d %.17g' % (name, rows, cols, 0.0))
        for v in w:
            print('W %.17g' % v)
elif cmd == 'pad4':
    n = int(sys.argv[2])
    print('PAD4 %d %d' % (n, len(U._pad4(bytes(n)))))
PY

cat > "$T/spec1.txt" <<'SPEC'
enc.w 2 3 0.5 -1.25 2.0 0.0 1e-3 -7.5
enc.b 1 3 0.25 0.5 -0.125
SPEC
cat > "$T/spec2.txt" <<'SPEC'
head 1 5 0.3 -0.6 0.9 0.0 -0.3
SPEC

same=0
pt_dump() {   # pt_dump LABEL DTYPE SPEC ACC
    lab=$1; dt=$2; sp=$3; acc=$4
    "$B" 10 "$T/uns1" dumps "$dt" 0 "$acc" "$sp" >"$T/c-$lab.hex" 2>"$T/c-$lab.err"; c=$?
    [ "$c" -eq 0 ] || fail "C dumps refused $lab rc=$c $(cat "$T/c-$lab.err")"
    "$B" 10 "$T/uns1" dumps "$dt" 0 "$acc" "$sp" >"$T/c2-$lab.hex" 2>/dev/null || fail "C second run rc!=0 $lab"
    cmp -s "$T/c-$lab.hex" "$T/c2-$lab.hex" || fail "C not deterministic dumps $lab"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" dumps "$dt" 0 "$acc" "$sp" >"$T/p-$lab.hex" 2>"$T/p-$lab.err" || fail "python dumps refused $lab: $(tail -1 "$T/p-$lab.err")"
    cmp -s "$T/c-$lab.hex" "$T/p-$lab.hex" || fail "DIFF dumps $lab"
    same=$((same + 1)); echo "SAME dumps $lab ($(wc -c < "$T/c-$lab.hex" | tr -d ' ') hex chars)"
}
pt_dump f32-spec1 2 "$T/spec1.txt" 0.75
pt_dump i8-spec1 0 "$T/spec1.txt" 0.75
pt_dump i8-spec2 0 "$T/spec2.txt" 0.0
pt_dump f32-spec2 2 "$T/spec2.txt" -1.5

pt_load() {   # pt_load LABEL HEX
    lab=$1; hx=$2
    "$B" 10 "$T/uns1" loads "$hx" >"$T/cl-$lab.txt" 2>"$T/cl-$lab.err"; c=$?
    [ "$c" -eq 0 ] || fail "C loads refused $lab rc=$c $(cat "$T/cl-$lab.err")"
    "$B" 10 "$T/uns1" loads "$hx" >"$T/cl2-$lab.txt" 2>/dev/null || fail "C second loads run rc!=0 $lab"
    cmp -s "$T/cl-$lab.txt" "$T/cl2-$lab.txt" || fail "C not deterministic loads $lab"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" loads "$hx" >"$T/pl-$lab.txt" 2>"$T/pl-$lab.err" || fail "python loads refused $lab: $(tail -1 "$T/pl-$lab.err")"
    # python loads() does not return the per-tensor scale; compare every line with the T scale field dropped
    awk '/^T /{NF=NF-1} {print}' "$T/cl-$lab.txt" > "$T/cl-$lab.nos"
    awk '/^T /{NF=NF-1} {print}' "$T/pl-$lab.txt" > "$T/pl-$lab.nos"
    cmp -s "$T/cl-$lab.nos" "$T/pl-$lab.nos" || { diff -u "$T/pl-$lab.nos" "$T/cl-$lab.nos" | head -30; fail "DIFF loads $lab"; }
    same=$((same + 1)); echo "SAME loads $lab ($(wc -l <"$T/cl-$lab.txt" | tr -d ' ') lines)"
}
pt_load f32-spec1 "$(cat "$T/c-f32-spec1.hex")"
pt_load i8-spec1 "$(cat "$T/c-i8-spec1.hex")"
pt_load i8-spec2 "$(cat "$T/c-i8-spec2.hex")"

pt_pad() {
    n=$1
    "$B" 10 "$T/uns1" pad4 "$n" >"$T/cp-$n.txt" || fail "C pad4 rc!=0 $n"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" pad4 "$n" >"$T/pp-$n.txt" || fail "python pad4 refused $n"
    cmp -s "$T/cp-$n.txt" "$T/pp-$n.txt" || fail "DIFF pad4 $n"
    same=$((same + 1))
}
for n in 0 1 2 3 4 5 17; do pt_pad $n; done
echo "SAME pad4 0..17 (7 points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME HEX
    nm=$1; hx=$2
    "$B" 10 "$T/uns1" loads "$hx" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" loads "$hx" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-$nm-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
GOOD=$(cat "$T/c-f32-spec1.hex")
neg bad-magic "5858585800$(echo "$GOOD" | cut -c11-)"
neg truncated-header "$(echo "$GOOD" | cut -c1-30)"
neg truncated-payload "$(echo "$GOOD" | cut -c1-120)"

echo "---- identity ----"
echo "HEAD $H"
echo "uns1.py $(sha256sum unisa/control/uns1.py | cut -d' ' -f1)"
echo "unisa_uns1.c $(sha256sum seed/unisa_uns1.c | cut -d' ' -f1)"
echo "unisauns1check.sh $(sha256sum tests/unisauns1check.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisauns1  same $same  diff 0  neg 3/3"
exit 0
