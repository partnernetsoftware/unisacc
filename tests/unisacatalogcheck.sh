#!/bin/sh
# K6-8: unisa.catalog C reference (seed/unisa_catalog.c) vs unisa/catalog.py on point samples: for each
# (op, os, arch) point, one line of derived target facts (form, symbol, gate, sysno, nrreg, argument
# registers, ret, argshape, retconv, winimp) must be byte-equal to the python derivation; OS/ARCH/TARGETS
# constants; C double-run; negatives (unknown op on lnx and on osx) refused by both sides with empty stdout.
# Point-sample equivalence only; not a product closure member; keeps catalog.py; the full table is not
# compared exhaustively (K6-8b).  First red stops.  usage: tests/unisacatalogcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisacatalog.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisacatalog: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisacatalog  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/catalog" seed/unisa_catalog.c || fail "compile red: seed/unisa_catalog.c"

cat > "$T/pydump.py" <<'PY'
import sys
from unisa import catalog as C
cmd = sys.argv[1]
if cmd == 'consts':
    print('OS ' + ' '.join(C.OS))
    print('ARCH ' + ' '.join(C.ARCH))
    print('TARGETS ' + ' '.join(C.TARGETS))
    sys.exit(0)
op, os_, arch = sys.argv[1], sys.argv[2], sys.argv[3]
n = C.nine(op, os_, arch)
form = C.enc_form(op, os_, arch)
assert form == n['form']
fields = [op, os_, arch, form, n['symbol'], n['gate'], n['sysno'], n['nrreg'],
          n['arg0'], n['arg1'], n['arg2'], n['arg3'], n['arg4'], n['arg5'], n['ret'],
          n['argshape'], n['retconv'], n['winimp']]
print(' '.join(fields))
PY

same=0
pt() {   # pt OP OS ARCH
    "$B" 10 "$T/catalog" "$1" "$2" "$3" >"$T/c-$1-$2-$3.txt" 2>"$T/c-$1-$2-$3.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused $1 $2 $3 rc=$c $(cat "$T/c-$1-$2-$3.err")"
    "$B" 10 "$T/catalog" "$1" "$2" "$3" >"$T/c2-$1-$2-$3.txt" 2>/dev/null || fail "C second run rc!=0 $1 $2 $3"
    cmp -s "$T/c-$1-$2-$3.txt" "$T/c2-$1-$2-$3.txt" || fail "C not deterministic $1 $2 $3"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$1" "$2" "$3" >"$T/p-$1-$2-$3.txt" 2>"$T/p-$1-$2-$3.err" || fail "python refused $1 $2 $3: $(tail -1 "$T/p-$1-$2-$3.err")"
    cmp -s "$T/c-$1-$2-$3.txt" "$T/p-$1-$2-$3.txt" || { diff -u "$T/p-$1-$2-$3.txt" "$T/c-$1-$2-$3.txt" | head -20; fail "DIFF $1 $2 $3"; }
    same=$((same + 1)); echo "SAME $1 $2 $3 : $(cut -d' ' -f4-8 "$T/c-$1-$2-$3.txt")"
}
PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" consts > "$T/py-consts.txt" || fail "python consts"
"$B" 10 "$T/catalog" consts >"$T/c-consts.txt" 2>"$T/c-consts.err" || fail "C consts refused $(cat "$T/c-consts.err")"
"$B" 10 "$T/catalog" consts >"$T/c2-consts.txt" 2>/dev/null || fail "C consts second run rc!=0"
cmp -s "$T/c-consts.txt" "$T/c2-consts.txt" || fail "C consts not deterministic"
cmp -s "$T/c-consts.txt" "$T/py-consts.txt" || { diff -u "$T/py-consts.txt" "$T/c-consts.txt"; fail "DIFF consts (C vs python OS/ARCH/TARGETS)"; }
echo "SAME consts C vs python OS/ARCH/TARGETS (6 targets)"; same=$((same + 1))

# sysno: 3 ops x 2 targets; plus the osx-None case (clock_gettime on osx -> none)
pt exit lnx x86_64
pt exit osx arm64
pt mmap win arm64
pt open lnx arm64
pt clock_gettime osx x86_64
pt getdents64 osx arm64
# sysno >= 3 ops x >= 2 targets (narrow-fix 2137): exit, mmap, open, close each on two targets
pt mmap lnx x86_64
pt mmap osx arm64
pt open osx x86_64
pt close lnx x86_64
pt close osx arm64
# enc_form / symbol / gate: >= 2 points each (covered above and below)
pt add64 lnx x86_64
pt syscall_gate win arm64
pt fsqrt64 osx arm64
pt unlink win x86_64
pt rename lnx arm64
pt write win arm64
pt ppoll lnx arm64
pt lseek win arm64

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME OP OS ARCH
    nm=$1; op=$2; os_=$3; arch=$4
    "$B" 10 "$T/catalog" "$op" "$os_" "$arch" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$op" "$os_" "$arch" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-$nm-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
neg unknown-op-lnx nosuchop lnx x86_64
neg unknown-op-osx nosuchop osx arm64

echo "---- identity ----"
echo "HEAD $H"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_catalog.c $(sha256sum seed/unisa_catalog.c | cut -d' ' -f1)"
echo "unisacatalogcheck.sh $(sha256sum tests/unisacatalogcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisacatalog  same $same  diff 0  neg 2/2"
exit 0
