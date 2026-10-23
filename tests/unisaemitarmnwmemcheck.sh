#!/bin/sh
# K6-19: unisa.emit_arm N / w / movimm / mem C reference (seed/unisa_emit_arm_nwmem.c) vs unisa/emit_arm.py on
# the legal domain (N: plain x-registers, x31, sp, a w-token; w: zero, one, minus one, a value above 32 bits;
# mem: scaled imm12, unscaled imm9 with negative offsets, the sp base, and the IP0 far path on both sides of
# the 4096-word limit and the 256-byte limit) and on three refusal points (empty register token; width 0;
# width 3).  The python side imports unisa.emit_arm directly (its top level has no build step) and calls
# only N, w and mem; movimm is reached through mem's far path.  Outputs compared byte-for-byte; C double-run.
# Not the full encode(), not relocations, WinAPI, fp, _callimp, adr/adrp_add, or emit_x86.  Point-sample
# equivalence only; keeps emit_arm.py, bits.py, catalog.py.  First red stops.  usage: tests/unisaemitarmnwmemcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaemitarm.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisaemitarm: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaemitarm  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/enw" seed/unisa_emit_arm_nwmem.c || fail "compile red: seed/unisa_emit_arm_nwmem.c"

# python side: emit_arm.N / w / mem on the same arguments; refusals (exceptions) exit 1 with nothing on stdout
cat > "$T/pydump.py" <<'PY'
import sys
from unisa import emit_arm as e
cmd = sys.argv[1]
try:
    if cmd == 'N':
        out = '%d\n' % e.N(sys.argv[2])
    elif cmd == 'w':
        out = e.w(int(sys.argv[2])).hex() + '\n'
    elif cmd == 'mem':
        store, rt, rn, off, wd = int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
        out = e.mem(store, rt, rn, off, wd).hex() + '\n'
    else:
        raise ValueError('command')
except Exception as ex:
    sys.stderr.write('refused: %s\n' % ex)
    sys.exit(1)
sys.stdout.write(out)
PY

same=0; total=0
pair() {   # pair ARGS... -- C and python stdout byte-equal; C double-run
    total=$((total + 1))
    "$B" 10 "$T/enw" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/enw" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; cat "$T/c.out"; echo "--- py"; cat "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
# N: x-registers, x31, sp, and a w-token (python returns 31 for any non-x token)
for t in x0 x7 x30 x31 sp w5; do pair N "$t"; done
# w: zero, one, minus one, a value above 32 bits (masked)
pair w 0; pair w 1; pair w -1; pair w 78187493520
# mem: scaled imm12 (ldr64 / str32 / ldr8 / str16), unscaled imm9 (negative and positive), sp base, far path
pair mem 0 0 1 8 8
pair mem 1 2 31 16 4
pair mem 0 0 1 4 1
pair mem 1 2 1 -2 2
pair mem 0 0 1 -1 1
pair mem 1 2 31 -300 4
pair mem 0 0 1 32768 8
echo "SAME $same of $total points (N: 6 tokens; w: 4 values; mem: 7 legal forms incl. scaled, unscaled, sp base, far path)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/enw" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg empty-register-token N ""
neg width-zero mem 0 0 1 8 0
neg width-three mem 0 0 1 0 3

echo "---- identity ----"
echo "HEAD $H"
echo "emit_arm.py $(sha256sum unisa/emit_arm.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_emit_arm_nwmem.c $(sha256sum seed/unisa_emit_arm_nwmem.c | cut -d' ' -f1)"
echo "unisaemitarmnwmemcheck.sh $(sha256sum tests/unisaemitarmnwmemcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaemitarm  same $same  diff 0  neg 3/3"
exit 0
