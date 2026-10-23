#!/bin/sh
# K6-13: unisa.image.elf C reference (seed/unisa_image_elf.c) vs unisa/image/elf.py write(dynamic=False)
# on point samples: x86_64 and arm64; non-empty text and data; a bss point (full > len(data)); a text
# crossing a page; a text ending exactly on a page; an empty text; hex and fill segments.
# Output compared byte-for-byte (hex).  Layout property on the C output, independent of python: the
# header, two PT_LOAD entries (r-x over headers+text, rw- over data), p_offset = round_up(tend, PAGE),
# p_vaddr = VADDR + p_offset, filesz/memsz as the spec, text and data bytes at their offsets.
# A negative entry is accepted while VADDR + HDRS + entry stays a valid address (python accepts entry -4).
# Negatives (both sides refuse, stdout empty): unknown arch; an entry whose address is negative
# (-(VADDR+176)-1: python struct.error); negative full (python struct.error).
# dynamic=True is not sampled (not claimed).  C double-run.  Point-sample equivalence only; not a
# product closure member; keeps image/elf.py and bits.py; does not cover macho/pe/ape.
# First red stops.  usage: tests/unisaimageelfcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaimageelf.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisaimageelf: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaimageelf  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/elf" seed/unisa_image_elf.c || fail "compile red: seed/unisa_image_elf.c"

# python side: elf.write(arch, text, data, entry, full) with dynamic=False; refuses on any exception
cat > "$T/pydump.py" <<'PY'
import sys
from unisa.image import elf
try:
    vals = {}
    for line in open(sys.argv[1], encoding='ascii').read().split('\n'):
        f = line.split()
        if not f:
            continue
        if f[0] == 'arch' and len(f) == 2:
            vals['arch'] = f[1]
        elif f[0] in ('text', 'data') and len(f) >= 3 and f[1] == 'f' and len(f) == 4 and len(f[2]) == 2:
            n = int(f[3])
            if n < 0:
                raise ValueError('negative fill count')
            vals[f[0]] = bytes([int(f[2], 16)]) * n
        elif f[0] in ('text', 'data') and len(f) == 3 and f[1] == 'x':
            vals[f[0]] = b'' if f[2] == '-' else bytes.fromhex(f[2])
        elif f[0] == 'entry' and len(f) == 2:
            vals['entry'] = int(f[1])
        elif f[0] == 'full' and len(f) == 2:
            vals['full'] = int(f[1])
        else:
            raise ValueError('malformed spec line')
    out = elf.write(vals['arch'], vals['text'], vals['data'], vals['entry'], full=vals.get('full'))
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out.hex() + '\n')
PY

# layout property on a hex output and its spec (independent of the writer's code path)
cat > "$T/layout.py" <<'PY'
import struct, sys
hexout, spec = sys.argv[1], sys.argv[2]
data_out = bytes.fromhex(open(hexout).read().strip())
vals = {}
for line in open(spec, encoding='ascii').read().split('\n'):
    f = line.split()
    if not f:
        continue
    if f[0] == 'arch': vals['arch'] = f[1]
    elif f[0] in ('text', 'data'):
        if f[1] == 'f': vals[f[0]] = bytes([int(f[2], 16)]) * int(f[3])
        else: vals[f[0]] = b'' if f[2] == '-' else bytes.fromhex(f[2])
    elif f[0] == 'entry': vals['entry'] = int(f[1])
    elif f[0] == 'full': vals['full'] = int(f[1])
text, dat = vals['text'], vals['data']
full = vals.get('full', len(dat))
PAGE, VADDR, HDRS = 0x1000, 0x400000, 64 + 56 * 2
assert data_out[:4] == b'\x7fELF' and data_out[4:6] == bytes([2, 1]), 'ident'
machine = struct.unpack_from('<H', data_out, 18)[0]
assert machine == {'x86_64': 0x3E, 'arm64': 0xB7}[vals['arch']], 'machine'
entry, phoff, shoff = struct.unpack_from('<QQQ', data_out, 24)
assert entry == VADDR + HDRS + vals['entry'], 'entry'
assert phoff == 64 and shoff == 0, 'phoff/shoff'
phnum = struct.unpack_from('<H', data_out, 56)[0]
assert phnum == 2, 'phnum'
tend = HDRS + len(text)
doff = (tend + PAGE - 1) // PAGE * PAGE
def ph(i):
    return struct.unpack_from('<IIQQQQQQ', data_out, 64 + 56 * i)
t0, f0, o0, v0, p0, fs0, ms0, al0 = ph(0)
t1, f1, o1, v1, p1, fs1, ms1, al1 = ph(1)
assert (t0, f0, o0, v0, p0, fs0, ms0, al0) == (1, 5, 0, VADDR, VADDR, tend, tend, PAGE), 'PT_LOAD r-x'
assert (t1, f1, o1, v1, p1, fs1, ms1, al1) == (1, 6, doff, VADDR + doff, VADDR + doff, len(dat), full, PAGE), 'PT_LOAD rw-'
assert o1 % PAGE == 0 and v1 % PAGE == o1 % PAGE, 'data alignment'
assert len(data_out) == doff + len(dat), 'file length'
assert data_out[HDRS:tend] == text, 'text bytes'
assert data_out[tend:doff] == bytes(doff - tend), 'padding'
assert data_out[doff:] == dat, 'data bytes'
print('LAYOUT OK text_end=%#x data_off=%#x memsz=%d' % (tend, doff, full))
PY

same=0; total=0
spec() {   # spec NAME: both sides on $T/NAME.spec, compare hex, check layout on the C output
    nm=$1; total=$((total + 1))
    "$B" 10 "$T/elf" write "$T/$nm.spec" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/elf" write "$T/$nm.spec" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$T/$nm.spec" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $nm rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $nm"
    [ "$p" -eq 0 ] || fail "python refused $nm: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -c 200 "$T/c.out"; echo; echo "--- py"; head -c 200 "$T/p.out"; echo; fail "DIFF $nm"; }
    a=$(PYTHONPATH="$R" "$B" 10 python3 "$T/layout.py" "$T/c.out" "$T/$nm.spec" 2>&1) || fail "layout $nm: $a"
    echo "$nm: $a; equal to python"
    same=$((same + 1))
}
printf 'arch x86_64\ntext f 90 16\ndata f 00 8\nentry 4\nfull 64\n' > "$T/x-bss.spec"
printf 'arch arm64\ntext x 1f2003d51f2003d5\ndata x 0102030405060708\nentry 0\n' > "$T/a-full.spec"
printf 'arch x86_64\ntext f 90 5000\ndata f ab 4096\nentry 16\n' > "$T/x-cross.spec"
printf 'arch arm64\ntext x -\ndata f 11 1\nentry 0\n' > "$T/a-empty-text.spec"
printf 'arch x86_64\ntext f cc 3920\ndata x 4142434445\nentry 0\n' > "$T/x-page-edge.spec"
printf 'arch arm64\ntext f 90 64\ndata f 22 16\nentry 8\nfull 4096\n' > "$T/a-bss.spec"
printf 'arch arm64\ntext x 90\ndata x 00\nentry -4\n' > "$T/a-negentry.spec"
for nm in x-bss a-full x-cross a-empty-text x-page-edge a-bss a-negentry; do spec "$nm"; done
echo "SAME $same of $total specs (x86_64 and arm64; bss/full; page-crossing text; text ending on a page; empty text; signed entry)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME SPEC -- both refuse, empty stdout
    nm=$1; sp=$2
    "$B" 10 "$T/elf" write "$sp" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$sp" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
printf 'arch riscv\ntext x 90\ndata x 00\nentry 0\n' > "$T/n1.spec";  neg unknown-arch "$T/n1.spec"
printf 'arch x86_64\ntext x 90\ndata x 00\nentry -4194481\n' > "$T/n2.spec"; neg entry-address-negative "$T/n2.spec"
printf 'arch arm64\ntext x 90\ndata x 00\nentry 0\nfull -1\n' > "$T/n3.spec"; neg negative-full "$T/n3.spec"

echo "---- identity ----"
echo "HEAD $H"
echo "elf.py $(sha256sum unisa/image/elf.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_image_elf.c $(sha256sum seed/unisa_image_elf.c | cut -d' ' -f1)"
echo "unisaimageelfcheck.sh $(sha256sum tests/unisaimageelfcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaimageelf  same $same  diff 0  neg 3/3"
exit 0
