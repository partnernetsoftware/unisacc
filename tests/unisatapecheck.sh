#!/bin/sh
# K6-10: unisa.tape C reference (seed/unisa_tape.c) vs unisa/tape.py on point samples: build (label /
# emit / string / B-zero globals) then to_text and to_canonical_text; parse of text files (link and
# unit records, strings with escapes and comments, bracket operands) then to_text / to_canonical_text.
# Byte-equal stdout; C double-run; six negatives refused by both sides with empty stdout.
# Point-sample equivalence only; not a product closure member; keeps tape.py, fp.py, padded.py, bits.py.
# First red stops.  usage: tests/unisatapecheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisatape.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisatape: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisatape  tree $H"

"$B" 30 cc -std=c99 -O2 -w -ffp-contract=off -o "$T/tape" seed/unisa_tape.c || fail "compile red: seed/unisa_tape.c"

# python side: the same Tape API, driven by the same script/text inputs (refuses on any exception)
cat > "$T/pydump.py" <<'PY'
import sys
from unisa import tape
mode, src, form = sys.argv[1], sys.argv[2], sys.argv[3]
try:
    text = open(src, encoding='ascii').read()
    if mode == 'build':
        t = tape.Tape()
        for line in text.split('\n'):
            f = line.split()
            if not f:
                continue
            k = f[0]
            if k == 'L' and len(f) == 2:
                t.label(f[1])
            elif k == 'E' and len(f) >= 2:
                shape = tape.SHAPE[f[1]]
                if len(f) - 2 != len(shape):
                    raise ValueError('operand count')
                t.emit(f[1], *[int(x, 0) if kd == 'i' else x for kd, x in zip(shape, f[2:])])
            elif k == 'S' and len(f) == 4:
                if int(f[3]) < 0:
                    raise ValueError('align')
                t.string(f[1], bytes.fromhex(f[2]), align=int(f[3]))
            elif k == 'B' and len(f) == 3:
                t.string(f[1], int(f[2]), align=8)
            else:
                raise ValueError('script form')
    else:
        t = tape.parse(text)
    out = t.to_text() if form == 'text' else t.to_canonical_text()
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out)
PY

# ---- build samples (script form) ----
cat > "$T/b1.txt" <<'EOF'
L main
E imm r1 5
E load64 r2 r7 8
E .ld r0 r1 16 4
E ret
S msg 6869 1
B arena 40
EOF
cat > "$T/b2.txt" <<'EOF'
L top
E imm r0 -7
E imm r1 0x1F
E add64 r1 r2 r3
E mov r4 r5
E store64 r7 -8 r1
E .st r7 0 r1 8
E .zero r3 0 16
E callm r4 24
E jumpz r1 top
E jump top
E call top
E callr r2
E .sys write r0 r1 r2
E .sys6 mmap r0 r1 r2 r3 r4 r5
E .lea r0 msg
E .frame 16
E .arg 0 r1
E .print r0
E .write r0 r1
E .argv r0 r1
E .libraryaddr r0 sym 1 2 3 4
E .hostaddr r1 2
E nop
E ret
EOF
cat > "$T/b3.txt" <<'EOF'
EOF
cat > "$T/b4.txt" <<'EOF'
L only
S z 00 1
S long 0000000000000000000000000000000000000000 1
S mixed 41424344454647484950515253545556575859606162636465666768697071 8
B z2 0
B big 17
E ret
EOF
cat > "$T/b5.txt" <<'EOF'
L a
E nop
L b
L a
E ret
L end
EOF

# ---- parse samples (text form) ----
cat > "$T/f1.tape" <<'EOF'
.unit 2
.global foo
.extern bar
.gdef baz
.str s "a\tb\x41\"q\\"   ; a comment after a string
.bss z 32
main:
  imm     r1, 0x10
  load64  r2, [r7+8]
  store64 [r7-8], r1
  .ld     r0, [r1+16], 4
  .st     [r1+0], r2, 8
  .zero   [r1-4], 8
  jumpz   r1, main
  .lea    r0, s
  ret
EOF
cat > "$T/f2.tape" <<'EOF'
; header comment
.str msg "x;y;z"
.str e ""
.bss pad 3
top:
  imm r0, 0
  jump top
  nop
EOF
cat > "$T/f3.tape" <<'EOF'
.str a "\x00\xff\n\r\0"
.bss big 40
L9:
  callm [r2+4]
EOF
# ---- negatives (both sides must refuse, stdout empty) ----
printf 'frob r0\n' > "$T/n1.tape"
printf 'imm r9, 1\n' > "$T/n2.tape"
printf 'imm r1, 010\n' > "$T/n3.tape"
printf 'L x\nE add64 r1 r2\n' > "$T/n4.txt"
printf '.str s "abc\n' > "$T/n5.tape"
printf '.gdef g\n.unit 2\n' > "$T/n6.tape"

same=0; total=0
pair() {   # pair MODE SRC FORM -- C and python stdout byte-equal; C double-run
    m=$1; src=$2; form=$3; total=$((total + 1))
    "$B" 10 "$T/tape" "$m" "$src" "$form" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/tape" "$m" "$src" "$form" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" "$src" "$form" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $m $(basename "$src") $form rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $m $(basename "$src") $form"
    [ "$p" -eq 0 ] || fail "python refused $m $(basename "$src") $form: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; cat "$T/c.out"; echo "--- py"; cat "$T/p.out"; fail "DIFF $m $(basename "$src") $form"; }
    same=$((same + 1))
}
for f in b1 b2 b3 b4 b5; do pair build "$T/$f.txt" text; pair build "$T/$f.txt" canon; done
for f in f1 f2 f3; do
    pair parse "$T/$f.tape" text
    pair parse "$T/$f.tape" canon
    "$B" 10 "$T/tape" parse "$T/$f.tape" canon >"$T/$f.canon" || fail "canon of $f"
    pair parse "$T/$f.canon" text
    pair parse "$T/$f.canon" canon
done
echo "SAME $same of $total outputs (build: 5 scripts, parse: 3 files + canonical round trips; text and canon)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME MODE SRC -- both refuse, empty stdout
    nm=$1; m=$2; src=$3
    "$B" 10 "$T/tape" "$m" "$src" canon >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" "$src" canon >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg unknown-op parse "$T/n1.tape"
neg bad-register parse "$T/n2.tape"
neg leading-zero-int parse "$T/n3.tape"
neg emit-arity build "$T/n4.txt"
neg unterminated-str parse "$T/n5.tape"
neg gdef-before-unit parse "$T/n6.tape"

echo "---- identity ----"
echo "HEAD $H"
echo "tape.py $(sha256sum unisa/tape.py | cut -d' ' -f1)"
echo "fp.py $(sha256sum unisa/fp.py | cut -d' ' -f1)"
echo "padded.py $(sha256sum unisa/padded.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_tape.c $(sha256sum seed/unisa_tape.c | cut -d' ' -f1)"
# emit-side negative (0.0.43): the Python emit API refuses an underscore integer with the C emitter's diagnostic
if PYTHONPATH="$R" python3 -c "from unisa import tape; tape.Tape().emit('imm', 'r1', '1_000')" 2>"$T/emit.err"; then fail "python emit accepted an underscore integer"; fi
grep -q 'not an integer (python: ValueError) 1_000' "$T/emit.err" || fail "python emit diagnostic differs from the C emitter: $(cat "$T/emit.err")"
echo "unisatapecheck.sh $(sha256sum tests/unisatapecheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisatape  same $same  diff 0  neg 6/6"
exit 0
