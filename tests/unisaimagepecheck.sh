#!/bin/sh
# K6-15: unisa.image.pe C reference (seed/unisa_image_pe.c) vs unisa/image/pe.py write() on point samples,
# stub empty and dynamic=False: x86_64 and arm64; non-empty text and data; full above and below len(data)
# (python clamps with max(1, full)); bss; a text crossing a 4 KB page; empty text; a negative entry whose
# address stays valid; relocations on two pages.  Output compared byte-for-byte (hex).  Layout property on
# the C output, independent of python: DOS/PE/COFF/optional header fields, the four section headers,
# data directories, the import descriptor and its name table (KERNEL32.dll and the 22 IMPORTS), the
# load-config size and stack-cookie pointer, and the decoded .reloc DIR64 entries.
# Negatives (both sides refuse, stdout empty): unknown arch; entry address below zero; entry address above
# 32 bits.  Scope: the bytes this writer produces; not a claim that the image loads on Windows or is an APE;
# not a product closure; keeps pe.py and bits.py.  C double-run.  First red stops.  usage: tests/unisaimagepecheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaimagepe.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisaimagepe: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaimagepe  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/pe" seed/unisa_image_pe.c || fail "compile red: seed/unisa_image_pe.c"

# python side: pe.write(arch, text, data, entry, relocs=..., full=..., bss=...) with stub empty, dynamic False
cat > "$T/pydump.py" <<'PY'
import sys
from unisa.image import pe
try:
    vals = {'relocs': [], 'bss': 0}
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
        elif f[0] == 'bss' and len(f) == 2:
            vals['bss'] = int(f[1])
        elif f[0] == 'reloc' and len(f) == 2:
            r = int(f[1])
            if r < 0:
                raise ValueError('negative relocation RVA')
            vals['relocs'].append(r)
        else:
            raise ValueError('malformed spec line')
    out = pe.write(vals['arch'], vals['text'], vals['data'], vals['entry'],
                   relocs=tuple(vals['relocs']), bss=vals['bss'], full=vals.get('full'))
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out.hex() + '\n')
PY

# layout and content property on a hex output and its spec (independent of pe.py's code path)
cat > "$T/layout.py" <<'PY'
import struct, sys
hexout, spec = sys.argv[1], sys.argv[2]
out = bytes.fromhex(open(hexout).read().strip())
vals = {'relocs': [], 'bss': 0}
for line in open(spec, encoding='ascii').read().split('\n'):
    f = line.split()
    if not f:
        continue
    if f[0] == 'arch': vals['arch'] = f[1]
    elif f[0] in ('text', 'data'):
        vals[f[0]] = bytes([int(f[2], 16)]) * int(f[3]) if f[1] == 'f' else (b'' if f[2] == '-' else bytes.fromhex(f[2]))
    elif f[0] == 'entry': vals['entry'] = int(f[1])
    elif f[0] == 'full': vals['full'] = int(f[1])
    elif f[0] == 'bss': vals['bss'] = int(f[1])
    elif f[0] == 'reloc': vals['relocs'].append(int(f[1]))
text, dat = vals['text'], vals['data']
full = vals.get('full', len(dat))
IMAGEBASE, SA, FA, TEXT_RVA, HDR = 0x140000000, 0x1000, 0x200, 0x1000, 0x400
IMPORTS = ["GetStdHandle", "WriteFile", "ReadFile", "CloseHandle", "CreateFileA", "ExitProcess", "GetCommandLineA",
           "VirtualAlloc", "VirtualProtect", "VirtualFree", "FlushInstructionCache", "SetFilePointer", "DeleteFileA",
           "MoveFileExA", "CreateProcessW", "WaitForSingleObject", "GetExitCodeProcess", "TerminateProcess",
           "SetStdHandle", "GetCurrentProcess", "GetCurrentProcessId", "GetModuleFileNameW"]
r = lambda v, a: (v + a - 1) // a * a
assert out[:2] == b'MZ' and struct.unpack_from('<I', out, 0x3C)[0] == 0x40, 'DOS header'
assert out[0x40:0x44] == b'PE\0\0', 'PE signature'
mach, nsec, _, _, _, optsz, chars = struct.unpack_from('<HHIIIHH', out, 0x44)
assert mach == {'x86_64': 0x8664, 'arm64': 0xAA64}[vals['arch']] and nsec == 4 and optsz == 240 and chars == 0x22, 'COFF'
o = 0x58
magic, mj, mn, szc, szi, szu, entry_rva, base_code = struct.unpack_from('<HBBIIIII', out, o)
assert magic == 0x20B and base_code == TEXT_RVA and entry_rva == TEXT_RVA + vals['entry'], 'optional standard'
ib, sa, fa = struct.unpack_from('<QII', out, o + 24)
assert (ib, sa, fa) == (IMAGEBASE, SA, FA), 'image base and alignments'
sizeimg, sizehdr, chk, subsys, dllch = struct.unpack_from('<IIIHH', out, o + 56)
assert subsys == 3 and dllch == 0x8160 and sizehdr == HDR, 'windows fields'
dirs = [struct.unpack_from('<II', out, o + 112 + 8 * i) for i in range(16)]
secs = []
for i in range(nsec):
    name, vs, va, rs, ro, _, _, _, _, fl = struct.unpack_from('<8sIIIIIIHHI', out, 0x148 + 40 * i)
    secs.append((name.rstrip(b'\0').decode(), vs, va, rs, ro, fl))
txt, rdt, dat_s, rel = secs
assert txt[0] == '.text' and txt[2] == TEXT_RVA and txt[1] == len(text) and txt[4] == HDR, '.text header'
rd_rva = rdt[2]
assert rd_rva == TEXT_RVA + r(len(text), SA), '.rdata RVA'
dt_rva = dat_s[2]
idata_len = rdt[1]
assert dt_rva == rd_rva + r(idata_len, SA), '.data RVA'
assert dat_s[1] == r(max(1, full), 8) + 8 + vals['bss'], '.data virtual size'
rl_rva = rel[2]
assert rl_rva == dt_rva + r(dat_s[1], SA), '.reloc RVA'
assert dirs[1] == (rd_rva, 40) and dirs[5][0] == rl_rva, 'import and reloc directories'
rd = out[rdt[4]:rdt[4] + rdt[3]]
oft, _, _, namerva, ft = struct.unpack_from('<IIIII', rd, 0)
assert oft == rd_rva + 40 and ft == rd_rva + dirs[12][0] - rd_rva, 'import descriptor thunks'
iat_off = dirs[12][0] - rd_rva
assert dirs[12] == (rd_rva + iat_off, 23 * 8), 'IAT directory'
assert rd[namerva - rd_rva:namerva - rd_rva + 13] == b'KERNEL32.dll\0', 'dll name'
for i, n in enumerate(IMPORTS):
    ent = struct.unpack_from('<Q', rd, 40 + 8 * i)[0]
    hint = ent - rd_rva
    assert rd[hint + 2:hint + 2 + len(n) + 1] == n.encode() + b'\0', 'hint name %s' % n
cfg = dirs[10][0]
assert dirs[10][1] == 0x140, 'load config size'
cfg_off = cfg - rd_rva
assert struct.unpack_from('<I', rd, cfg_off)[0] == 0x140, 'load config self size'
cookie_rva = dt_rva + r(max(1, full), 8)
assert struct.unpack_from('<Q', rd, cfg_off + 0x58)[0] == IMAGEBASE + cookie_rva, 'stack cookie pointer'
exp = sorted(set([rd_rva + cfg_off + 0x58] + [dt_rva + x for x in vals['relocs']]))
got = []
q = out[rel[4]:rel[4] + rel[1]]
p = 0
while p < len(q):
    page, bsz = struct.unpack_from('<II', q, p)
    assert bsz >= 8, 'relocation block size'
    for k in range((bsz - 8) // 2):
        w = struct.unpack_from('<H', q, p + 8 + 2 * k)[0]
        if w >> 12 == 10:
            got.append(page + (w & 0xFFF))
        else:
            assert w >> 12 == 0, 'reloc type'
    p += bsz
assert sorted(got) == exp, 'relocation entries %s != %s' % (sorted(got), exp)
assert out[txt[4]:txt[4] + len(text)] == text, '.text bytes'
assert out[dat_s[4]:dat_s[4] + len(dat)] == dat, '.data bytes'
assert sizeimg == rl_rva + r(rel[1], SA), 'SizeOfImage'
print('LAYOUT OK rdata=%#x data=%#x reloc=%#x relocs=%d' % (rd_rva, dt_rva, rl_rva, len(exp)))
PY

same=0; total=0
spec() {   # spec NAME: both sides on $T/NAME.spec, compare hex, check layout and content on the C output
    nm=$1; total=$((total + 1))
    "$B" 10 "$T/pe" write "$T/$nm.spec" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/pe" write "$T/$nm.spec" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$T/$nm.spec" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $nm rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $nm"
    [ "$p" -eq 0 ] || fail "python refused $nm: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -c 120 "$T/c.out"; echo; echo "--- py"; head -c 120 "$T/p.out"; echo; fail "DIFF $nm"; }
    a=$(PYTHONPATH="$R" "$B" 10 python3 "$T/layout.py" "$T/c.out" "$T/$nm.spec" 2>&1) || fail "layout $nm: $a"
    echo "$nm: $a; $(( ($(wc -c < "$T/c.out") - 1) / 2 )) bytes equal to python"
    same=$((same + 1))
}
printf 'arch x86_64\ntext f 90 16\ndata f 00 8\nentry 4\n' > "$T/x-basic.spec"
printf 'arch arm64\ntext x 1f2003d51f2003d5\ndata x 0102030405060708\nentry 0\n' > "$T/a-hex.spec"
printf 'arch x86_64\ntext f 90 16\ndata f 00 8\nentry 0\nfull 64\n' > "$T/x-full.spec"
printf 'arch x86_64\ntext f 90 5000\ndata f ab 100\nentry 16\nreloc 8\nreloc 4096\n' > "$T/x-reloc-cross.spec"
printf 'arch arm64\ntext f 90 16\ndata f 22 8\nentry 0\nfull -1\n' > "$T/a-full-clamped.spec"
printf 'arch arm64\ntext x -\ndata f 11 1\nentry 0\nbss 16\n' > "$T/a-empty-bss.spec"
printf 'arch arm64\ntext f 90 16\ndata f 33 8\nentry -4\n' > "$T/a-negentry.spec"
for nm in x-basic a-hex x-full x-reloc-cross a-full-clamped a-empty-bss a-negentry; do spec "$nm"; done
echo "SAME $same of $total specs (x86_64/arm64; full above and below len(data); bss; text crossing a page; empty text; relocs on two pages; signed entry)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME SPEC -- both refuse, empty stdout
    nm=$1; sp=$2
    "$B" 10 "$T/pe" write "$sp" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$sp" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
printf 'arch riscv\ntext x 90\ndata x 00\nentry 0\n' > "$T/n1.spec"; neg unknown-arch "$T/n1.spec"
printf 'arch x86_64\ntext x 90\ndata x 00\nentry -4097\n' > "$T/n2.spec"; neg entry-address-negative "$T/n2.spec"
printf 'arch arm64\ntext x 90\ndata x 00\nentry 4294967295\n' > "$T/n3.spec"; neg entry-above-32-bits "$T/n3.spec"

echo "---- identity ----"
echo "HEAD $H"
echo "pe.py $(sha256sum unisa/image/pe.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_image_pe.c $(sha256sum seed/unisa_image_pe.c | cut -d' ' -f1)"
echo "unisaimagepecheck.sh $(sha256sum tests/unisaimagepecheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaimagepe  same $same  diff 0  neg 3/3"
exit 0
