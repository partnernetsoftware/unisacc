#!/bin/sh
# K6-11: unisa.tapebin C reference (seed/unisa_tapebin.c) vs unisa/tapebin.py on point samples:
# encode (script-built Tapes: labels, emit, string, zero globals, unit and link records, SOURCE_SHA)
# to hex; decode of those blobs to ORIGIN / SHA / canonical text; target_id and check_origin.
# Byte-equal stdout; C double-run; six negatives refused by both sides with empty stdout.
# Point-sample equivalence only; not a product closure member; keeps tapebin.py, tape.py, fp.py,
# padded.py, bits.py.  First red stops.  usage: tests/unisatapebincheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisatapebin.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisatapebin: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisatapebin  tree $H"

"$B" 30 cc -std=c99 -O2 -w -ffp-contract=off -o "$T/tb" seed/unisa_tapebin.c || fail "compile red: seed/unisa_tapebin.c"

# python side: the same Tape API / record appends as the C script, then encode / decode / target / check_origin
cat > "$T/pydump.py" <<'PY'
import sys
from unisa import tape, tapebin
mode = sys.argv[1]
try:
    if mode == 'encode':
        src, origin, shaarg = sys.argv[2], int(sys.argv[3], 0), sys.argv[4]
        t = tape.Tape()
        for line in open(src, encoding='ascii').read().split('\n'):
            f = line.split()
            if not f:
                continue
            k = f[0]
            if k == 'U' and len(f) == 1:
                t.records.append(('unit', 2))
            elif k == 'G' and len(f) == 3 and f[1] in ('global', 'extern', 'gdef'):
                t.records.append((f[1], f[2]))
            elif k == 'L' and len(f) == 2:
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
                if int(f[2]) < 0:
                    raise ValueError('size')
                t.string(f[1], int(f[2]), align=8)
            else:
                raise ValueError('script form')
        sha = None if shaarg == '-' else bytes.fromhex(shaarg)
        blob = tapebin.encode(t, origin_target=origin, source_sha=sha)
        out = blob.hex() + '\n'
    elif mode == 'decode':
        blob = bytes.fromhex(open(sys.argv[2], encoding='ascii').read().strip())
        t = tapebin.decode(blob)
        out = 'ORIGIN %d\n' % t.origin_target
        if getattr(t, 'source_sha', None) is not None:
            out += 'SHA %s\n' % t.source_sha.hex()
        out += t.to_canonical_text()
    elif mode == 'target':
        out = '%d\n' % tapebin.target_id(sys.argv[2])
    elif mode == 'checkorigin':
        t = tape.Tape()
        t.origin_target = int(sys.argv[2], 0)
        tapebin.check_origin(t, sys.argv[3], force=sys.argv[4] == '1')
        out = 'OK\n'
    else:
        raise ValueError('mode')
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out)
PY

# ---- encode samples (script form) ----
cat > "$T/e1.txt" <<'EOF'
U
L main
E imm r1 5
E load64 r2 r7 8
E .sys write r0 r1 r2
E ret
S msg 6869 1
B arena 40
EOF
cat > "$T/e2.txt" <<'EOF'
L top
E imm r0 -7
E imm r1 0x1F
E add64 r1 r2 r3
E store64 r7 -8 r1
E .st r7 0 r1 8
E .zero r3 0 16
E callm r4 24
E jumpz r1 top
E jump top
E .libraryaddr r0 sym 1 2 3 4
E nop
EOF
cat > "$T/e3.txt" <<'EOF'
U
G global foo
G extern bar
G gdef baz
S s 61620a2200 1
B z 24
L after
E ret
EOF
cat > "$T/e4.txt" <<'EOF'
S zeros 00000000000000000000000000000000000000000000000000000000000000000000000000000000 8
S mixed 41424344454647484950515253545556575859606162636465666768697071 4
B small 3
L only
E ret
EOF
cat > "$T/e5.txt" <<'EOF'
EOF
printf 'deadbeef' > /dev/null
SHA32=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff

same=0; total=0
pair() {   # pair MODE ARGS... -- C and python stdout byte-equal; C double-run
    m=$1; shift; total=$((total + 1))
    "$B" 10 "$T/tb" "$m" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/tb" "$m" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $m $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $m $*"
    [ "$p" -eq 0 ] || fail "python refused $m $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; cat "$T/c.out"; echo "--- py"; cat "$T/p.out"; fail "DIFF $m $*"; }
    same=$((same + 1))
}
# encode: five scripts x (origin, sha) choices; the blob is the python result, kept for decode
pair encode "$T/e1.txt" 1 -
pair encode "$T/e2.txt" 0 -
pair encode "$T/e3.txt" 6 "$SHA32"
pair encode "$T/e4.txt" 3 -
pair encode "$T/e5.txt" 0 -
for f in e1 e2 e3 e4 e5; do
    case $f in e1) o=1; s=-;; e2) o=0; s=-;; e3) o=6; s="$SHA32";; e4) o=3; s=-;; e5) o=0; s=-;; esac
    "$B" 10 "$T/tb" encode "$T/$f.txt" "$o" "$s" > "$T/$f.hex" || fail "C encode for decode input $f"
    pair decode "$T/$f.hex"
done
# target and check_origin
for n in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do pair target "$n"; done
pair checkorigin 0 lnx/x86_64 0
pair checkorigin 1 lnx/x86_64 0
pair checkorigin 2 lnx/x86_64 1
echo "SAME $same of $total outputs (encode 5 scripts, decode of their blobs, target 6, checkorigin 3)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME MODE ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/tb" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
# bad magic, truncated blob, origin 7 in the header (hash unaffected), flipped body byte, unknown target, origin mismatch
sed 's/^../00/' "$T/e1.hex" > "$T/n1.hex";  neg bad-magic decode "$T/n1.hex"
head -c 200 "$T/e1.hex" > "$T/n2.hex";      neg truncated decode "$T/n2.hex"
python3 -c "import sys; s=open(sys.argv[1]).read().strip(); open(sys.argv[2],'w').write(s[:120]+'07'+s[122:]+'\n')" "$T/e1.hex" "$T/n3.hex"; neg origin-7 decode "$T/n3.hex"
python3 -c "import sys; s=open(sys.argv[1]).read().strip(); open(sys.argv[2],'w').write(s[:128]+('00' if s[128:130]!='00' else '01')+s[130:]+'\n')" "$T/e1.hex" "$T/n4.hex"; neg body-flip decode "$T/n4.hex"
neg unknown-target target bsd/x86
neg origin-mismatch checkorigin 2 lnx/x86_64 0

echo "---- identity ----"
echo "HEAD $H"
echo "tapebin.py $(sha256sum unisa/tapebin.py | cut -d' ' -f1)"
echo "tape.py $(sha256sum unisa/tape.py | cut -d' ' -f1)"
echo "fp.py $(sha256sum unisa/fp.py | cut -d' ' -f1)"
echo "padded.py $(sha256sum unisa/padded.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_tapebin.c $(sha256sum seed/unisa_tapebin.c | cut -d' ' -f1)"
echo "unisatapebincheck.sh $(sha256sum tests/unisatapebincheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisatapebin  same $same  diff 0  neg 6/6"
exit 0
