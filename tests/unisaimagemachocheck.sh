#!/bin/sh
# K6-14: unisa.image.macho C reference (seed/unisa_image_macho.c) vs unisa/image/macho.py write() on
# point samples: x86_64 and arm64; non-empty text and data; bss (full > len(data)); text crossing a
# 16 KB page; empty text; a negative signed entry whose address stays valid.  Output compared
# byte-for-byte (hex).  Layout property on the C output, independent of python: the header, the twelve
# load commands in order with their segment/section fields, LC_MAIN entryoff, the LC_CODE_SIGNATURE
# offset and size, and the ad-hoc SuperBlob whose CodeDirectory SHA-256 slots are recomputed here with
# hashlib over each 4 KB page of the image up to the signature offset.
# Negatives (both sides refuse, stdout empty): unknown arch; bss below zero (full < len(data));
# an entry whose address is negative.  Scope: the bytes this writer produces; not a claim that the image
# loads, runs, or is accepted by a real kernel or codesign; not a product closure; keeps macho.py/bits.py.
# C double-run.  First red stops.  usage: tests/unisaimagemachocheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaimagemacho.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisaimagemacho: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaimagemacho  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/macho" seed/unisa_image_macho.c || fail "compile red: seed/unisa_image_macho.c"

# python side: macho.write(arch, text, data, entry, full); refuses on any exception
cat > "$T/pydump.py" <<'PY'
import sys
from unisa.image import macho
try:
    vals = {}
    for line in open(sys.argv[1], encoding='ascii').read().split('\n'):
        f = line.split()
        if not f:
            continue
        if f[0] == 'arch' and len(f) == 2:
            vals['arch'] = f[1]
        elif f[0] in ('text', 'data') and len(f) == 4 and f[1] == 'f' and len(f[2]) == 2:
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
    out = macho.write(vals['arch'], vals['text'], vals['data'], vals['entry'], full=vals.get('full'))
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out.hex() + '\n')
PY

# layout and ad-hoc signature property on a hex output and its spec (independent of the writer)
cat > "$T/layout.py" <<'PY'
import hashlib, struct, sys
hexout, spec = sys.argv[1], sys.argv[2]
out = bytes.fromhex(open(hexout).read().strip())
vals = {}
for line in open(spec, encoding='ascii').read().split('\n'):
    f = line.split()
    if not f:
        continue
    if f[0] == 'arch': vals['arch'] = f[1]
    elif f[0] in ('text', 'data'):
        vals[f[0]] = bytes([int(f[2], 16)]) * int(f[3]) if f[1] == 'f' else (b'' if f[2] == '-' else bytes.fromhex(f[2]))
    elif f[0] == 'entry': vals['entry'] = int(f[1])
    elif f[0] == 'full': vals['full'] = int(f[1])
text, dat = vals['text'], vals['data']
full = vals.get('full', len(dat))
PAGE, PAGE4, VM, DLP = 0x4000, 0x1000, 0x100000000, 32
cpu = {'x86_64': 0x01000007, 'arm64': 0x0100000C}[vals['arch']]
magic, c, s, ft, ncmds, sizeofcmds, flags, _ = struct.unpack_from('<IiiIIIII', out, 0)
assert magic == 0xFEEDFACF and c == cpu and ft == 2 and ncmds == 12, 'header'
hdrs = 32 + sizeofcmds + 156
assert out[32 + sizeofcmds:hdrs] == bytes(156), 'slack'
off, kinds, lcs = 32, [], {}
for _ in range(ncmds):
    cmd, cs = struct.unpack_from('<II', out, off)
    kinds.append(cmd); lcs.setdefault(cmd, []).append(off)
    off += cs
assert off == 32 + sizeofcmds, 'load command sizes'
assert kinds == [0x19, 0x19, 0x19, 0x19, 0xE, 0xC, 0x80000028, 0x32, 0x80000022, 0x2, 0xB, 0x1D], 'load command order'
textsz = (hdrs + len(text) + PAGE - 1) // PAGE * PAGE
datavm = (full + DLP + PAGE - 1) // PAGE * PAGE
datasz = (len(dat) + DLP + PAGE - 1) // PAGE * PAGE
link = textsz + datasz
def seg(i):
    o = lcs[0x19][i]
    return struct.unpack_from('<II16sQQQQiiII', out, o)
pz = seg(0); tx = seg(1); dt = seg(2); le_ = seg(3)
assert pz[2].rstrip(b'\0') == b'__PAGEZERO' and pz[3] == 0 and pz[4] == VM, 'PAGEZERO'
assert tx[2].rstrip(b'\0') == b'__TEXT' and tx[3] == VM and tx[4] == textsz and tx[5] == 0 and tx[6] == textsz, 'TEXT'
assert dt[2].rstrip(b'\0') == b'__DATA' and dt[3] == VM + textsz and dt[4] == datavm and dt[5] == textsz and dt[6] == datasz, 'DATA'
assert le_[2].rstrip(b'\0') == b'__LINKEDIT' and le_[3] == VM + textsz + datavm and le_[5] == link, 'LINKEDIT'
text_sect = struct.unpack_from('<16s16sQQIIIIIIII', out, lcs[0x19][1] + 72)
assert text_sect[2] == VM + hdrs and text_sect[3] == len(text) and text_sect[4] == hdrs, '__text'
assert out[hdrs:hdrs + len(text)] == text, 'text bytes'
entryoff = struct.unpack_from('<Q', out, lcs[0x80000028][0] + 8)[0]
assert entryoff == hdrs + vals['entry'], 'LC_MAIN entryoff'
bl = 0
for n in (b'dlopen', b'dlsym', b'dlclose', b'dlerror'):
    bl += 5 + 1 + len(n) + 1 + 1
bl += 1
bindoff = link + 8
sigoff = (bindoff + bl + 15) // 16 * 16
dy = lcs[0xB][0]
assert struct.unpack_from('<IIII', out, lcs[0x80000022][0] + 8)[0:4] == (0, 0, bindoff, bl), 'DYLD_INFO bind'
assert out[link + 8:link + 8 + 0] == b'' and out[bindoff:bindoff + bl][:5] == bytes((0x11, 0x51, 0x72, 0, 0x40)), 'bind bytes'
cs_off, cs_len = struct.unpack_from('<II', out, lcs[0x1D][0] + 8)
assert cs_off == sigoff, 'code signature offset'
assert cs_off + cs_len == len(out), 'signature runs to the end'
sb_magic, sb_len, sb_count = struct.unpack_from('>III', out, sigoff)
assert sb_magic == 0xFADE0CC0 and sb_len == cs_len and sb_count == 1, 'SuperBlob'
slot_type, slot_off = struct.unpack_from('>II', out, sigoff + 12)
cd = sigoff + slot_off
assert slot_type == 0 and slot_off == 20, 'CodeDirectory slot'
mg, cdlen, ver, flg, hoff, ioff, nspec, nslots, climit = struct.unpack_from('>IIIIIIIII', out, cd)
slots = (sigoff + PAGE4 - 1) // PAGE4
assert mg == 0xFADE0C02 and ver == 0x20400 and flg == 2 and climit == sigoff and nslots == slots, 'CodeDirectory fixed fields'
assert out[cd + ioff: cd + ioff + 6] == b'unisa\0', 'identifier'
hashes = out[cd + hoff: cd + hoff + 32 * slots]
for i in range(slots):
    page = out[i * PAGE4: min((i + 1) * PAGE4, sigoff)]
    assert hashes[32 * i: 32 * i + 32] == hashlib.sha256(page).digest(), 'page hash %d' % i
assert len(out) == sigoff + cs_len, 'file length'
print('LAYOUT OK sigoff=%#x slots=%d' % (sigoff, slots))
PY

same=0; total=0
spec() {   # spec NAME: both sides on $T/NAME.spec, compare hex, check layout and signature on the C output
    nm=$1; total=$((total + 1))
    "$B" 10 "$T/macho" write "$T/$nm.spec" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/macho" write "$T/$nm.spec" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$T/$nm.spec" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $nm rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $nm"
    [ "$p" -eq 0 ] || fail "python refused $nm: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -c 120 "$T/c.out"; echo; echo "--- py"; head -c 120 "$T/p.out"; echo; fail "DIFF $nm"; }
    a=$(PYTHONPATH="$R" "$B" 10 python3 "$T/layout.py" "$T/c.out" "$T/$nm.spec" 2>&1) || fail "layout $nm: $a"
    echo "$nm: $a; $(( ($(wc -c < "$T/c.out") - 1) / 2 )) bytes equal to python"
    same=$((same + 1))
}
printf 'arch x86_64\ntext f 90 16\ndata f 00 8\nentry 4\nfull 64\n' > "$T/x-bss.spec"
printf 'arch arm64\ntext x 1f2003d51f2003d5\ndata x 0102030405060708\nentry 0\n' > "$T/a-full.spec"
printf 'arch x86_64\ntext f 90 20000\ndata f ab 100\nentry 16\n' > "$T/x-cross.spec"
printf 'arch arm64\ntext x -\ndata f 11 1\nentry 0\n' > "$T/a-empty-text.spec"
printf 'arch arm64\ntext f 90 16\ndata f 22 8\nentry -4\n' > "$T/a-negentry.spec"
for nm in x-bss a-full x-cross a-empty-text a-negentry; do spec "$nm"; done
echo "SAME $same of $total specs (x86_64 and arm64; bss; text crossing a 16 KB page; empty text; signed entry; ad-hoc signature checked per 4 KB page)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME SPEC -- both refuse, empty stdout
    nm=$1; sp=$2
    "$B" 10 "$T/macho" write "$sp" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$sp" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
printf 'arch riscv\ntext x 90\ndata x 00\nentry 0\n' > "$T/n1.spec"; neg unknown-arch "$T/n1.spec"
printf 'arch x86_64\ntext x 90\ndata f 00 8\nentry 0\nfull 4\n' > "$T/n2.spec"; neg bss-below-zero "$T/n2.spec"
HD=$(PYTHONPATH="$R" python3 -c "from unisa.image import macho; print(macho.HDRS('x86_64'))")
printf 'arch x86_64\ntext x 90\ndata x 00\nentry -%d\n' "$((HD + 1))" > "$T/n3.spec"; neg entry-address-negative "$T/n3.spec"

echo "---- identity ----"
echo "HEAD $H"
echo "macho.py $(sha256sum unisa/image/macho.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_image_macho.c $(sha256sum seed/unisa_image_macho.c | cut -d' ' -f1)"
echo "unisaimagemachocheck.sh $(sha256sum tests/unisaimagemachocheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaimagemacho  same $same  diff 0  neg 3/3"
exit 0
