#!/bin/sh
# K6-17: unisa.vm run() C reference (seed/unisa_vm.c) vs unisa/vm.py VM(tape, max_steps).run() on point
# programs: integer arithmetic; signed and unsigned division and remainder, shifts, a div-by-zero halt (136);
# a loop with a call and a return, a store and a load, and a print; the step-limit halt (124); a memory
# fault halt (139); narrow loads and stores with sign extension.  Each program is built through the
# Tape API (label / emit) on the python side and as a text script on the C side; the result is CODE,
# STEPS and OUT (hex), compared byte-for-byte.  C double-run.  Negatives (both sides refuse, stdout
# empty): an unknown tape op (python: assert in emit); a register name that is not a register (python:
# KeyError when the instruction runs); a jump to an unknown label (python: KeyError when it runs).
# Halts are normal results, not refusals.  Point-sample equivalence over the listed programs only; not
# a full opcode sweep; not a real OS file/syscall closure; keeps vm.py and the closed deps.  First red stops.
# usage: tests/unisavmcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisavm.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
fail() { echo "unisavm: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisavm  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/vmc" seed/unisa_vm.c || fail "compile red: seed/unisa_vm.c"

# python side: the Tape API and vm.VM(...).run(); exceptions refuse, halts print CODE/STEPS/OUT
cat > "$T/pydump.py" <<'PY'
import sys
from unisa import tape, vm
src, maxs = sys.argv[1], int(sys.argv[2])
try:
    t = tape.Tape()
    for line in open(src, encoding='ascii').read().split('\n'):
        f = line.split()
        if not f:
            continue
        if f[0] == 'L' and len(f) == 2:
            t.label(f[1])
        elif f[0] == 'I' and len(f) >= 2:
            shape = tape.SHAPE[f[1]]
            if len(f) - 2 != len(shape):
                raise ValueError('operand count')
            t.emit(f[1], *[int(x, 0) if kd == 'i' else x for kd, x in zip(shape, f[2:])])
        else:
            raise ValueError('script form')
    out, code, steps = vm.VM(t, max_steps=maxs).run()
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write('CODE %d\nSTEPS %d\nOUT %s\n' % (code, steps, out.hex()))
PY

# Subclass hooks must retain reference call order, including inherited overrides.
PYTHONPATH="$R" "$B" 10 python3 - <<'PYHOOK' || fail "subclass hook equivalence"
from unisa.tape import Tape
from unisa.vm import VM

t = Tape()
t.emit('imm', 'r6', 8192)
t.emit('imm', 'r0', 42)
t.emit('store64', 'r6', 0, 'r0')
t.emit('load64', 'r1', 'r6', 0)
t.emit('.st', 'r6', 8, 'r1', 1)
t.emit('.ld', 'r2', 'r6', 8, 1)
t.emit('call', 'sub')
t.emit('.exit', 'r2')
t.label('sub')
t.emit('ret')
for name in ('ld', 'st', '_addr'):
    def hook(self, *args, _name=name, **kw):
        self.calls.append((args, kw))
        return getattr(VM, _name)(self, *args, **kw)
    parent = type('Hook_' + name, (VM,), {name: hook})
    child = type('Inherited_' + name, (parent,), {})
    for cls in (parent, child):
        a, b = cls(t), cls(t)
        a.calls, b.calls = [], []
        expected, actual = a.run_reference(), b.run()
        assert expected == actual and a.calls == b.calls and a.calls, (name, cls, a.calls, b.calls)
        print('subclass hook exact', cls.__name__, len(a.calls))
PYHOOK

same=0; total=0
pair() {   # pair NAME MAXSTEPS -- C and python outputs byte-equal; C double-run
    nm=$1; ms=$2; total=$((total + 1))
    "$B" 10 "$T/vmc" run "$ms" "$T/$nm.txt" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/vmc" run "$ms" "$T/$nm.txt" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$T/$nm.txt" "$ms" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $nm rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $nm"
    [ "$p" -eq 0 ] || fail "python refused $nm: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; cat "$T/c.out"; echo "--- py"; cat "$T/p.out"; fail "DIFF $nm"; }
    echo "$nm: $(tr '\n' ' ' < "$T/c.out")"
    same=$((same + 1))
}
printf 'I imm r0 7\nI imm r1 5\nI add64 r2 r0 r1\nI mul64 r3 r2 r1\nI sub64 r4 r3 r0\nI .exit r4\n' > "$T/arith.txt"
printf 'I imm r0 -7\nI imm r1 2\nI .div r2 r0 r1\nI .mod r3 r0 r1\nI shr64 r4 r0 r1\nI lshr64 r5 r0 r1\nI .print r2\nI .print r3\nI .print r4\nI .print r5\nI .exit r2\n' > "$T/divmod.txt"
printf 'I imm r0 0\nI imm r1 100\nL top\nI jumpz r1 done\nI add64 r0 r0 r1\nI imm r2 1\nI sub64 r1 r1 r2\nI jump top\nL done\nI imm r6 8192\nI store64 r6 0 r0\nI load64 r3 r6 0\nI .print r3\nI call sub1\nI .exit r0\nL sub1\nI imm r4 9\nI add64 r0 r0 r4\nI ret\n' > "$T/loop.txt"
printf 'I imm r0 100\nI imm r1 7\nI .udiv r2 r0 r1\nI .umod r3 r0 r1\nI .print r2\nI .print r3\nI .exit r3\n' > "$T/udiv.txt"
printf 'I imm r0 -1\nI imm r1 3\nI .udiv r2 r0 r1\nI .umod r3 r0 r1\nI .exit r3\n' > "$T/udivbig.txt"
printf 'L top\nI jump top\n' > "$T/spin.txt"
printf 'I imm r0 16777216\nI load64 r1 r0 0\n' > "$T/fault.txt"
printf 'I imm r0 5\nI imm r1 0\nI .udiv r2 r0 r1\n' > "$T/divzero.txt"
printf 'I imm r0 255\nI imm r6 4096\nI .st r6 0 r0 1\nI .ld r1 r6 0 1\nI .print r1\nI .exit r1\n' > "$T/narrow.txt"
pair arith 1000
pair divmod 1000
pair loop 1000
pair spin 50
pair fault 1000
pair divzero 1000
pair narrow 1000
pair udiv 1000
pair udivbig 1000
echo "SAME $same of $total programs (arithmetic; signed div and mod; unsigned successful .udiv/.umod incl. 2^64-1; shifts; loop/call/ret; store/load; print; step-limit halt 124; memory fault 139; div-zero halt 136; narrow load/store)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME PROGRAM -- both refuse, empty stdout
    nm=$1; src=$2
    "$B" 10 "$T/vmc" run 1000 "$src" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$src" 1000 >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
printf 'I frob r0\n' > "$T/n1.txt"; neg unknown-op "$T/n1.txt"
printf 'I imm r9 1\nI .exit r0\n' > "$T/n2.txt"; neg bad-register "$T/n2.txt"
printf 'I jump nowhere\n' > "$T/n3.txt"; neg unknown-label "$T/n3.txt"
# .libraryfn: in the shape table, not executed by either reference; both refuse by name (c43, cc ruling).
# C 与 Python 参考的拒绝措辞不同，均点名操作码：each side is asserted on its own text.
negnamed() {   # negnamed NAME PROGRAM C_TEXT PY_TEXT -- both refuse, empty stdout, diagnostics name the op
    nm=$1; src=$2
    "$B" 10 "$T/vmc" run 1000 "$src" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$src" 1000 >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    grep -q "$3" "$T/n-c.err" || fail "NEG-$nm C diagnostic lacks '$3': $(tail -1 "$T/n-c.err")"
    grep -q "$4" "$T/n-p.err" || fail "NEG-$nm python diagnostic lacks '$4': $(tail -1 "$T/n-p.err")"
    echo "NEG-$nm both refuse by name (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err") py_err=$(tail -1 "$T/n-p.err")"
}
printf 'I .libraryfn r1 256\n' > "$T/n4.txt"; negnamed libraryfn-not-covered "$T/n4.txt" 'operation not covered by this reference' ".libraryfn"

echo "---- identity ----"
echo "HEAD $H"
echo "vm.py $(sha256sum unisa/vm.py | cut -d' ' -f1)"
echo "tape.py $(sha256sum unisa/tape.py | cut -d' ' -f1)"
echo "fp.py $(sha256sum unisa/fp.py | cut -d' ' -f1)"
echo "padded.py $(sha256sum unisa/padded.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_vm.c $(sha256sum seed/unisa_vm.c | cut -d' ' -f1)"
echo "unisavmcheck.sh $(sha256sum tests/unisavmcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisavm  same $same  diff 0  neg 4/4"
exit 0
