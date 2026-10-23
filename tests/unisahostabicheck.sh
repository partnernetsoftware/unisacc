#!/bin/sh
# K6-2: unisa.hostabi C reference (seed/unisa_hostabi.c) vs unisa/hostabi.py on the same TSV.
# Point-sample equivalence of the exported constants (ARM_FN ARM_ARGV ARM_BODY X86_BODY WIN_*);
# not a product closure member; does not delete hostabi.py; no src/version.h, release/, exec/ paths.
# First red stops (compile, DIFF, a NEG accepted by one side, out-of-scope).
# usage: tests/unisahostabicheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisahostabi.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisahostabi: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisahostabi  tree $H"
TSV=exec/enc/hostbridge.tsv
[ -f "$TSV" ] || fail "missing $TSV"

"$B" 30 cc -std=c99 -O2 -w -o "$T/hostabi" seed/unisa_hostabi.c || fail "compile red: seed/unisa_hostabi.c"

# Python dump: the module's exported constants in the same line format as the C dump.
cat > "$T/pydump.py" <<'PY'
from unisa import hostabi as h
def words(name, ws):
    print(f"{name} {len(ws)} " + ''.join('%08x' % w for w in ws))
def bts(name, b):
    print(f"{name} {len(b)} " + b.hex())
print(f"ARM_FN 1 {h.ARM_FN:08x}")
print(f"ARM_ARGV 1 {h.ARM_ARGV:08x}")
words('ARM_BODY', h.ARM_BODY)
bts('X86_BODY', h.X86_BODY)
words('WIN_ARM_BODY', h.WIN_ARM_BODY)
bts('WIN_X86_BODY', h.WIN_X86_BODY)
PY

"$B" 10 "$T/hostabi" dump "$TSV" >"$T/c1.txt" 2>"$T/c1.err"; c=$?
[ "$c" -eq 0 ] || fail "C dump rc=$c $(cat "$T/c1.err")"
"$B" 10 "$T/hostabi" dump "$TSV" >"$T/c2.txt" 2>"$T/c2.err"; c2=$?
[ "$c2" -eq 0 ] && cmp -s "$T/c1.txt" "$T/c2.txt" || fail "C not deterministic (rc=$c2)"
echo "SAME C double-run"

PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" >"$T/p.txt" 2>"$T/p.err" || fail "python dump rc=$? $(cat "$T/p.err")"
cmp -s "$T/c1.txt" "$T/p.txt" || { diff -u "$T/p.txt" "$T/c1.txt" | head -20; fail "DIFF C vs python dump"; }
echo "SAME C vs python: $(wc -l <"$T/c1.txt" | tr -d ' ') constants"

# negatives: one side must refuse, both sides must refuse; first accepted/one-sided = red
refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
mkneg() {   # mkneg NAME TSV-filter-sed
    d=$T/neg-$1; mkdir -p "$d/unisa" "$d/exec/enc" || fail "scratch $1"
    cp unisa/hostabi.py unisa/__init__.py "$d/unisa/" || fail "scratch copy $1"
    sed "$2" "$TSV" >"$d/exec/enc/hostbridge.tsv" || fail "scratch tsv $1"
}
neg() {   # neg NAME SED-EXPR -- must be refused by both sides
    mkneg "$1" "$2"
    d=$T/neg-$1
    "$B" 10 "$T/hostabi" dump "$d/exec/enc/hostbridge.tsv" >"$d/c.out" 2>"$d/c.err"; c=$?
    (cd "$d" && "$B" 10 python3 -c 'from unisa import hostabi' >"$d/p.out" 2>"$d/p.err"); p=$?
    refused "$c" || fail "NEG-$1 C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$1 python accepted (rc=$p): first red"
    [ ! -s "$d/c.out" ] || fail "NEG-$1 C wrote stdout: first red"
    echo "NEG-$1 both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$d/c.err")"
}
neg missing-arm-fn '/^arm64	fn	/d'
neg bad-hex-fn 's/^arm64	fn	aa0003f0$/arm64\tfn\taa0003zz/'
neg odd-x86-body 's/^x86_64	body	\(.*\)$/x86_64\tbody\t\1a/'

echo "---- identity ----"
echo "HEAD $H"
echo "hostabi.py $(sha256sum unisa/hostabi.py | cut -d' ' -f1)"
echo "hostbridge.tsv $(sha256sum "$TSV" | cut -d' ' -f1)"
echo "unisa_hostabi.c $(sha256sum seed/unisa_hostabi.c | cut -d' ' -f1)"
echo "dump-out $(sha256sum "$T/c1.txt" | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisahostabi  same 6 constants-lines diff 0 neg 3/3"
exit 0
