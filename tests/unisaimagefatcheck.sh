#!/bin/sh
# K6-12: unisa.image.fat C reference (seed/unisa_image_fat.c) vs unisa/image/fat.py write() on point
# samples: single x86_64 slice, single arm64 slice, two slices in both orders, an empty slice list,
# an empty image, and page-multiple images; output compared byte-for-byte (hex).  Alignment property:
# each descriptor's offset is a multiple of 1 << align and at or after the previous slice's end.
# Negatives (both sides refuse, stdout empty): unknown arch; malformed fill count; malformed hex.
# C double-run.  Point-sample equivalence only; not a product closure member; keeps image/fat.py,
# bits.py; does not cover elf/macho/pe/ape.  First red stops.  usage: tests/unisaimagefatcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaimagefat.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisaimagefat: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaimagefat  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/fat" seed/unisa_image_fat.c || fail "compile red: seed/unisa_image_fat.c"

# python side: fat.write() on the same spec lines; refuses on any exception (unknown arch, bad fields)
cat > "$T/pydump.py" <<'PY'
import sys
from unisa.image import fat
try:
    slices = []
    for line in open(sys.argv[1], encoding='ascii').read().split('\n'):
        f = line.split()
        if not f:
            continue
        if len(f) == 3 and f[1] == 'x':
            img = b'' if f[2] == '-' else bytes.fromhex(f[2]) if len(f[2]) % 2 == 0 else None
            if img is None:
                raise ValueError('odd-length hex')
        elif len(f) == 4 and f[1] == 'f':
            if len(f[2]) != 2:
                raise ValueError('fill byte needs two hex digits')
            n = int(f[3])
            if n < 0 or not f[3].isdigit():
                raise ValueError('fill count must be a non-negative decimal')
            img = bytes([int(f[2], 16)]) * n
        else:
            raise ValueError('malformed slice line')
        slices.append((f[0], img))
    out = fat.write(slices)
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out.hex() + '\n')
PY

# alignment property on a hex output and its spec: descriptors honour their alignment and do not overlap
cat > "$T/align.py" <<'PY'
import struct, sys
hexout, spec = sys.argv[1], sys.argv[2]
data = bytes.fromhex(open(hexout).read().strip())
magic, n = struct.unpack('>II', data[:8])
assert magic == 0xCAFEBABE, 'magic'
lines = [l.split() for l in open(spec, encoding='ascii').read().split('\n') if l.split()]
want = []
for f in lines:
    if f[1] == 'x':
        want.append(0 if f[2] == '-' else len(bytes.fromhex(f[2])))
    else:
        want.append(int(f[3]))
assert n == len(lines), 'slice count'
prev_end = 8 + 20 * n
for i in range(n):
    cpu, sub, at, size, al = struct.unpack('>IIIII', data[8 + 20 * i: 28 + 20 * i])
    assert at % (1 << al) == 0, 'offset not aligned to 1<<align'
    assert at >= prev_end, 'slice overlaps the previous one'
    assert size == want[i], 'size differs from the spec'
    assert data[at:at + size] == (bytes.fromhex(lines[i][2]) if lines[i][1] == 'x' and lines[i][2] != '-' else (b'' if lines[i][1] == 'x' else bytes([int(lines[i][2], 16)]) * size)), 'payload differs'
    prev_end = at + size
print('ALIGN OK %d slices' % n)
PY

same=0; total=0
spec() {   # spec NAME: run both sides on $T/NAME.spec, compare hex, check alignment on the C output
    nm=$1; total=$((total + 1))
    "$B" 10 "$T/fat" write "$T/$nm.spec" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/fat" write "$T/$nm.spec" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$T/$nm.spec" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $nm rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $nm"
    [ "$p" -eq 0 ] || fail "python refused $nm: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -c 160 "$T/c.out"; echo; echo "--- py"; head -c 160 "$T/p.out"; echo; fail "DIFF $nm"; }
    a=$(PYTHONPATH="$R" "$B" 10 python3 "$T/align.py" "$T/c.out" "$T/$nm.spec" 2>&1) || fail "alignment $nm: $a"
    echo "$nm: $a; C output $(( ($(wc -c < "$T/c.out") - 1) / 2 )) bytes, equal to python"
    same=$((same + 1))
}
printf 'x86_64 f ab 10\n' > "$T/single-x86.spec"
printf 'arm64 f cd 100\n' > "$T/single-arm.spec"
printf 'x86_64 f 11 5000\narm64 f 22 3\n' > "$T/pair-xa.spec"
printf 'arm64 f 22 3\nx86_64 f 11 5000\n' > "$T/pair-ax.spec"
printf 'x86_64 f 33 4096\nx86_64 x 0102030405060708\n' > "$T/pagemult.spec"
printf 'arm64 x -\nx86_64 f 00 0\n' > "$T/empty-images.spec"
: > "$T/none.spec"
for nm in single-x86 single-arm pair-xa pair-ax pagemult empty-images none; do spec "$nm"; done
echo "SAME $same of $total specs (x86_64 / arm64 / two slices both orders / page multiple / empty images / empty list)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME SPEC -- both refuse, empty stdout
    nm=$1; sp=$2
    "$B" 10 "$T/fat" write "$sp" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$sp" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
printf 'ppc f 00 4\n' > "$T/n1.spec";  neg unknown-arch "$T/n1.spec"
printf 'x86_64 f 00 -4\n' > "$T/n2.spec"; neg negative-fill-count "$T/n2.spec"
printf 'arm64 x abc\n' > "$T/n3.spec"; neg odd-length-hex "$T/n3.spec"

echo "---- identity ----"
echo "HEAD $H"
echo "fat.py $(sha256sum unisa/image/fat.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_image_fat.c $(sha256sum seed/unisa_image_fat.c | cut -d' ' -f1)"
echo "unisaimagefatcheck.sh $(sha256sum tests/unisaimagefatcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaimagefat  same $same  diff 0  neg 3/3"
exit 0
