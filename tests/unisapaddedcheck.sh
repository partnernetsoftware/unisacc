#!/bin/sh
# K6-4: unisa.padded.Padded C reference (seed/unisa_padded.c) vs unisa/padded.py on sampled
# (stored prefix, logical length) points: observable values byte-equal; C double-run; negatives
# refused by both sides.  Point-sample equivalence; not a product closure member; keeps padded.py.
# ZData (the segment class in padded.py) is out of this leaf (K6-4b).  First red stops.
# usage: tests/unisapaddedcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisapadded.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisapadded: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisapadded  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/padded" seed/unisa_padded.c || fail "compile red: seed/unisa_padded.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa.padded import Padded
hexs, fulls = sys.argv[1], sys.argv[2]
prefix = bytes.fromhex(hexs)          # refuses odd length / non-hex by name
full = int(fulls)                     # refuses non-decimal by name
p = Padded(prefix, full)
print('LEN %d' % len(p))
print('STORED %d' % p.stored())
print('PREFIX ' + bytes(p).hex())
print('RSTRIP %d' % len(p.rstrip(b'\x00')))
print('SLICE %d' % len(p[:]))
print('VIEW %d' % len(memoryview(p)))
print('ZERO_TAIL %d' % (len(p) - p.stored()))
PY

same=0
pt() {   # pt HEX FULL
    hx=$1; fu=$2; tag=$(printf '%s_%s' "${hx:-empty}" "$fu" | tr -c 'A-Za-z0-9_' '_')
    "$B" 10 "$T/padded" "$hx" "$fu" >"$T/c-$tag.txt" 2>"$T/c-$tag.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused a sampled point hex='$hx' full=$fu rc=$c $(cat "$T/c-$tag.err")"
    "$B" 10 "$T/padded" "$hx" "$fu" >"$T/c2-$tag.txt" 2>/dev/null || fail "C second run rc!=0 hex='$hx' full=$fu"
    cmp -s "$T/c-$tag.txt" "$T/c2-$tag.txt" || fail "C not deterministic hex='$hx' full=$fu"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$hx" "$fu" >"$T/p-$tag.txt" 2>"$T/p-$tag.err" || fail "python refused a sampled point hex='$hx' full=$fu: $(tail -1 "$T/p-$tag.err")"
    cmp -s "$T/c-$tag.txt" "$T/p-$tag.txt" || { diff -u "$T/p-$tag.txt" "$T/c-$tag.txt" | head -20; fail "DIFF hex='$hx' full=$fu"; }
    same=$((same + 1)); echo "SAME hex='$hx' full=$fu ($(wc -l <"$T/c-$tag.txt" | tr -d ' ') lines)"
}
pt "" 0
pt 41 1
pt 4100 10
pt 00000000 4
pt deadbeef 3
pt ff00ff -5
pt 00 1099511627776

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME HEX FULL -- both sides must refuse and print nothing
    nm=$1; hx=$2; fu=$3
    "$B" 10 "$T/padded" "$hx" "$fu" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$hx" "$fu" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed output: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
neg nonint-full 41 x
neg odd-hex abc 4
neg nonhex-prefix zz 2

echo "---- identity ----"
echo "HEAD $H"
echo "padded.py $(sha256sum unisa/padded.py | cut -d' ' -f1)"
echo "unisa_padded.c $(sha256sum seed/unisa_padded.c | cut -d' ' -f1)"
echo "unisapaddedcheck.sh $(sha256sum tests/unisapaddedcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisapadded  same $same  diff 0  neg 3/3"
exit 0
