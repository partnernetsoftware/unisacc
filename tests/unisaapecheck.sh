#!/bin/sh
# K6-16: unisa.ape build() C reference (seed/unisa_ape.c) vs unisa/ape.py on point samples: plain,
# with a P 1 payload, with a P 2 payload; compile_target is a FIXTURE in both languages (fixed bytes for
# the four Unix targets; for win/x86_64 the DOS-stub split of pe.write with a fixed PE body).  Output
# compared byte-for-byte (hex).  Independent layout check on the C output: the shell script's case table
# (o=, n=, k=) locates each gzip member, gzip.decompress returns the fixture image, the key is the SHA-256
# prefix of the compressed member, offsets follow the 16-byte alignment, the PE signature sits at
# e_lfanew, and the payload trailer (UNIPKG1 + length) is present when a payload is given.
# Negatives (both sides refuse, stdout empty): product name without version; a payload that is not a model
# package.  C double-run.  Scope: the bytes of build() with a fixture compile_target; not a product compile
# target, not a real PE image, not version injection, not loadable/executable on any OS; no product closure.
# Builds with -lz.  First red stops.  usage: tests/unisaapecheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisaape.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisaape: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisaape  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/ape" seed/unisa_ape.c -lz || fail "compile red: seed/unisa_ape.c"

# python side: ape.build with the same fixture compile_target; modes mirror the C side
cat > "$T/pydump.py" <<'PY'
import struct, sys, os
sys.path.insert(0, os.environ['REPO'])
from unisa import ape
mode = sys.argv[1]
def ct(target, stub=b""):
    if target == 'win/x86_64':
        sn = len(stub)
        dos = bytearray(64); dos[0:2] = b'MZ'
        first = stub[:62]; dos[2:2 + len(first)] = first
        struct.pack_into('<I', dos, 0x3C, sn + 2)
        return bytes(dos) + stub[62:] + b'PE\0\0' + bytes(16)
    n, salt = {'lnx/x86_64': (300, 1), 'lnx/arm64': (517, 2), 'osx/x86_64': (1000, 3), 'osx/arm64': (1500, 4)}[target]
    return bytes(((i * 31 + salt * 7) & 255) for i in range(n))
out = os.environ['OUTF']
try:
    kw = {}
    if mode == 'plain':
        payload = b''
    elif mode == 'payload-p1':
        payload = b'P 1 demo\n'
    elif mode == 'payload-p2':
        payload = b'P 2 xyz\n'
    elif mode == 'neg-name-only':
        payload = b''
        kw = {'product_name': 'x'}
    elif mode == 'neg-bad-payload':
        payload = b'hello'
    else:
        raise ValueError('mode')
    blob = ape.build(ct, out, payload=payload, **kw)
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(blob.hex() + '\n')
PY

# independent layout and content check of a hex output
cat > "$T/layout.py" <<'PY'
import gzip, hashlib, re, struct, sys
blob = bytes.fromhex(open(sys.argv[1]).read().strip())
mode = sys.argv[2]
assert blob[:8] == b"MZqFpD='", 'DOS head and stub opening'
peoff = struct.unpack_from('<I', blob, 0x3C)[0]
assert blob[peoff:peoff + 4] == b'PE\0\0' and peoff % 8 == 0, 'e_lfanew and PE signature'
stub_len = peoff - 2
head_len = peoff + 4 + 16
base = (head_len + 15) // 16 * 16
text = blob[:base].split(b'\n')
script = blob[:base]
s = script.decode('latin-1')
rows = re.findall(r'^  (\S+)\) o=(\d+) n=(\d+) k=([0-9a-f]{16});;$', s, re.M)
assert len(rows) == 4, 'case rows'
want = {'Linuxx86_64': ('lnx/x86_64', 300, 1), 'Linuxaarch64|Linuxarm64': ('lnx/arm64', 517, 2),
        'Darwinx86_64': ('osx/x86_64', 1000, 3), 'Darwinarm64': ('osx/arm64', 1500, 4)}
expect_off = base
seen = []
for pat, o, n, k in rows:
    if pat in ('Linuxaarch64|Linuxarm64',):
        tgt, cnt, salt = want[pat]
    else:
        tgt, cnt, salt = want[pat]
    assert int(o) == expect_off + 1, 'slice offset %s != %d' % (o, expect_off + 1)
    member = blob[int(o) - 1: int(o) - 1 + int(n)]
    assert hashlib.sha256(member).hexdigest()[:16] == k, 'key of %s' % pat
    img = gzip.decompress(member)
    assert img == bytes(((i * 31 + salt * 7) & 255) for i in range(cnt)), 'image of %s' % pat
    assert member[:4] == b'\x1f\x8b\x08\x00' and member[4:8] == b'\0\0\0\0' and member[8:10] == b'\x02\xff', 'gzip header of %s' % pat
    expect_off += (int(n) + 15) // 16 * 16
    seen.append(tgt)
assert seen == ['lnx/x86_64', 'lnx/arm64', 'osx/x86_64', 'osx/arm64'], 'slice order'
tail = blob[expect_off:]
if mode == 'plain':
    assert tail == b'', 'no payload trailer'
    assert b'UNIPKG1' not in blob and b'UNISA_CONTAINER' not in blob, 'no container line without payload'
else:
    payload = b'P 1 demo\n' if mode == 'payload-p1' else b'P 2 xyz\n'
    assert tail == payload + b'UNIPKG1\n' + struct.pack('<Q', len(payload)), 'payload trailer'
    assert b'UNISA_CONTAINER="$0"; export UNISA_CONTAINER' in blob, 'container line present with payload'
assert blob.rstrip(b'\n').endswith(b'exec "$t" "$@"') or b'exec "$t" "$@"' in blob, 'exec line'
print('LAYOUT OK base=%#x slices=%d tail=%d' % (base, len(seen), len(tail)))
PY

same=0; total=0
mode_pair() {   # mode_pair MODE -- C and python hex equal; C double-run; layout checked on the C output
    m=$1; total=$((total + 1))
    "$B" 10 "$T/ape" build "$m" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/ape" build "$m" >"$T/c2.out" 2>/dev/null; c2=$?
    REPO="$R" OUTF="$T/py.bin" PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $m rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $m"
    [ "$p" -eq 0 ] || fail "python refused $m: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -c 160 "$T/c.out"; echo; echo "--- py"; head -c 160 "$T/p.out"; echo; fail "DIFF $m"; }
    a=$(REPO="$R" PYTHONPATH="$R" "$B" 10 python3 "$T/layout.py" "$T/c.out" "$m" 2>&1) || fail "layout $m: $a"
    echo "$m: $a; $(( ($(wc -c < "$T/c.out") - 1) / 2 )) bytes equal to python"
    same=$((same + 1))
}
mode_pair plain
mode_pair payload-p1
mode_pair payload-p2
cat > "$T/stubx.py" <<'PYX'
import struct
from unisa.image import pe
stub = b"qFpD='\n" + b"." * 51 + b"....'\n" + b"# script\nexec x\n"
real = pe.write('x86_64', b'\x90' * 16, b'', 0, stub=stub)
peoff = struct.unpack_from('<I', real, 0x3C)[0]
fx = bytearray(64); fx[0:2] = b'MZ'; first = stub[:62]; fx[2:2 + len(first)] = first
struct.pack_into('<I', fx, 0x3C, len(stub) + 2)
fixture = bytes(fx) + stub[62:] + b'PE\0\0'
assert peoff == len(stub) + 2, 'e_lfanew'
assert real[:peoff + 4] == fixture, 'prefix'
print('STUB PREFIX OK e_lfanew=%d' % peoff)
PYX
# the fixture compile_target's DOS-stub prefix equals pe.write(stub=...) up to the PE signature (not a full PE check)
PYTHONPATH="$R" "$B" 10 python3 "$T/stubx.py" >"$T/stubx.out" 2>"$T/stubx.err" || fail "stub prefix cross-check: $(tail -1 "$T/stubx.err")"
cat "$T/stubx.out"
echo "SAME $same of $total builds (plain; P 1 payload; P 2 payload; fixture compile_target; Unix gzip members; PE stub split)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg MODE -- both refuse, empty stdout
    m=$1
    "$B" 10 "$T/ape" build "$m" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    REPO="$R" OUTF="$T/py-n.bin" PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$m C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$m python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$m C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$m python printed stdout: first red"
    echo "NEG-$m both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg neg-name-only
neg neg-bad-payload

echo "---- identity ----"
echo "HEAD $H"
echo "ape.py $(sha256sum unisa/ape.py | cut -d' ' -f1)"
echo "image/pe.py $(sha256sum unisa/image/pe.py | cut -d' ' -f1)"
echo "peversion.py $(sha256sum unisa/peversion.py | cut -d' ' -f1)"
echo "bits.py $(sha256sum unisa/bits.py | cut -d' ' -f1)"
echo "unisa_ape.c $(sha256sum seed/unisa_ape.c | cut -d' ' -f1)"
echo "unisaapecheck.sh $(sha256sum tests/unisaapecheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisaape  same $same  diff 0  neg 2/2"
exit 0
