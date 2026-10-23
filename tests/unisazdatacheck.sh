#!/bin/sh
# K6-4b: unisa.padded.ZData C reference (seed/unisa_zdata.c) vs unisa/padded.py on scripted
# operation sequences: segment dumps, reads, zero counts, copy_into, byte-equal observables;
# C double-run; negatives refused by both sides.  Point-sample equivalence over the listed scripts
# only; not a product closure member; keeps padded.py; does not touch unisa_padded.c / K6-4 check.
# First red stops.  usage: tests/unisazdatacheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisazdata.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisazdata: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisazdata  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/zdata" seed/unisa_zdata.c || fail "compile red: seed/unisa_zdata.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa.padded import ZData
z = ZData()
for line in open(sys.argv[1]):
    line = line.rstrip('\n')
    if not line or line.startswith('#'):
        continue
    t = line.split()
    op = t[0]
    if op == 'extend':
        z.extend(bytes.fromhex(t[1]) if len(t) > 1 else b'')
    elif op == 'zeros':
        z.extend(bytes(int(t[1])))
    elif op == 'extzero':
        z.extend_zero(int(t[1]))
    elif op == 'set':
        s = int(t[1]); raw = bytes.fromhex(t[2]) if len(t) > 2 else b''
        z[s:s + len(raw)] = raw
    elif op == 'setlen':
        s, e = int(t[1]), int(t[2]); raw = bytes.fromhex(t[3]) if len(t) > 3 else b''
        z[s:e] = raw
    elif op == 'read':
        s, e = int(t[1]), int(t[2]); print(bytes(z[s:e]).hex())
    elif op == 'count':
        s, e = int(t[1]), min(int(t[2]), len(z)); print(z.count(0, s, e))
    elif op == 'len':
        print(len(z))
    elif op == 'dump':
        for st, obj in z.segs:
            if isinstance(obj, int):
                print('SEG %d Z %d' % (st, obj))
            else:
                print('SEG %d D %d %s' % (st, len(obj), bytes(obj).hex()))
    elif op == 'copy':
        m, base = int(t[1]), int(t[2]); mem = bytearray(m); z.copy_into(mem, base); print(bytes(mem).hex())
    elif op == 'copy_data':
        from unisa.padded import copy_data   # the module-level entry point; ZData branch
        m, base = int(t[1]), int(t[2]); mem = bytearray(m); copy_data(mem, base, z); print(bytes(mem).hex())
    elif op == 'stepread':
        s, e, step = int(t[1]), int(t[2]), int(t[3]); z[s:e:step]   # real ZData.__getitem__ -> _range
    elif op == 'countx':
        s, e, x = int(t[1]), int(t[2]), int(t[3]); z.count(x, s, e)   # real ZData.count
    else:
        raise SystemExit('unknown op ' + op)
PY

mkdir -p "$T/scripts"
cat > "$T/scripts/A.ops" <<'OPS'
extend 0102
zeros 5000
extend 03
extzero 10
len
dump
read 0 8
count 0 5010
set 2 ff00
dump
read 0 12
count 0 5015
copy 5020 3
OPS
cat > "$T/scripts/B.ops" <<'OPS'
zeros 4096
zeros 4096
dump
set 100 00
dump
set 100 7f
set 101 7f
dump
count 0 8192
read 99 103
OPS
cat > "$T/scripts/C.ops" <<'OPS'
extend 1122
zeros 10
extzero 4096
set 5 01
dump
copy 4120 3
read 0 6
count 0 4120
OPS
cat > "$T/scripts/E.ops" <<'OPS'
extend 7f
zeros 4100
extend 0a0b
extzero 3
set 4101 00
dump
copy_data 4120 5
read 0 6
count 0 4106
OPS
cat > "$T/scripts/D.ops" <<'OPS'
extend ab
extzero 5000
extend cd
set 0 00
set 1 00
dump
read 0 5002
count 0 5002
copy 5010 0
OPS

same=0
pt() {   # pt NAME
    nm=$1; f=$T/scripts/$nm.ops
    "$B" 20 "$T/zdata" "$f" >"$T/c-$nm.txt" 2>"$T/c-$nm.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused script $nm rc=$c $(cat "$T/c-$nm.err")"
    "$B" 20 "$T/zdata" "$f" >"$T/c2-$nm.txt" 2>/dev/null || fail "C second run rc!=0 $nm"
    cmp -s "$T/c-$nm.txt" "$T/c2-$nm.txt" || fail "C not deterministic $nm"
    PYTHONPATH="$R" "$B" 20 python3 "$T/pydump.py" "$f" >"$T/p-$nm.txt" 2>"$T/p-$nm.err" || fail "python refused script $nm: $(tail -1 "$T/p-$nm.err")"
    cmp -s "$T/c-$nm.txt" "$T/p-$nm.txt" || { diff -u "$T/p-$nm.txt" "$T/c-$nm.txt" | head -30; fail "DIFF script $nm"; }
    same=$((same + 1)); echo "SAME $nm ($(wc -l <"$T/c-$nm.txt" | tr -d ' ') lines)"
}
pt A; pt B; pt C; pt D; pt E


refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME LINE -- both sides must refuse; nothing printed for the refused op
    nm=$1; line=$2; f=$T/scripts/neg-$nm.ops
    printf 'extend 0102\n%s\n' "$line" >"$f"
    "$B" 10 "$T/zdata" "$f" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$f" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed stdout (must be empty): first red"
    [ ! -s "$T/n-$nm-p.out" ] || fail "NEG-$nm python printed stdout (must be empty): first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
neg stepread "stepread 0 2 2"
neg countx "countx 0 2 1"
neg setlen "setlen 0 2 aabbcc"

echo "---- identity ----"
echo "HEAD $H"
echo "padded.py $(sha256sum unisa/padded.py | cut -d' ' -f1)"
echo "unisa_zdata.c $(sha256sum seed/unisa_zdata.c | cut -d' ' -f1)"
echo "unisazdatacheck.sh $(sha256sum tests/unisazdatacheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisazdata  same $same  diff 0  neg 3/3"
exit 0
