#!/bin/sh
# K6-3: unisa.peversion C reference (seed/unisa_peversion.c) vs unisa/peversion.py on the same unsigned
# PE32+ heads: byte-equal outputs for sampled (head, name, version) points, C double-run, and negatives
# refused by both sides.  Point-sample equivalence; not a product closure member; keeps peversion.py.
# First red stops (compile, DIFF, one-side accept, out-of-scope).  usage: tests/unisapeversioncheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisapeversion.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisapeversion: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisapeversion  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/peversion" seed/unisa_peversion.c || fail "compile red: seed/unisa_peversion.c"

# fixtures: generated unsigned PE32+ heads (python only builds the inputs)
cat > "$T/gen.py" <<'PY'
import struct,sys
def pe():
    b=bytearray(b'\0'*0x400)
    b[0:2]=b'MZ'; struct.pack_into('<I',b,60,0x80)
    b[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',b,0x84,0x8664,1,0,0,0,240,0x22)
    opt=0x98
    struct.pack_into('<H',b,opt,0x20B)
    struct.pack_into('<II',b,opt+32,0x1000,0x200)
    struct.pack_into('<I',b,opt+56,0x2000)
    struct.pack_into('<I',b,opt+60,0x200)
    struct.pack_into('<I',b,opt+8,0x200)
    struct.pack_into('<I',b,opt+108,16)
    struct.pack_into('<8sIIIIIIHHI',b,opt+240,b'.text\0\0\0',0x100,0x1000,0x200,0x200,0,0,0,0,0x60000020)
    for i in range(0x200): b[0x200+i]=(i*7+3)&255
    return bytes(b)
d=sys.argv[1]
open(d+'/base.pe','wb').write(pe())
open(d+'/ov.pe','wb').write(pe()+b'\x90')
p=bytearray(pe()); struct.pack_into('<H',p,0x98,0x10B); open(d+'/pe32.pe','wb').write(bytes(p))
p=bytearray(pe()); struct.pack_into('<II',p,0x98+112+16,0x1000,0x40); open(d+'/rsrc.pe','wb').write(bytes(p))
PY
python3 "$T/gen.py" "$T" || fail "fixture generation"

# python side: same contract as the C driver (refusal => non-zero exit, no output file)
cat > "$T/pyadd.py" <<'PY'
import sys
from unisa.peversion import add_version_info
inp,out,name,version=sys.argv[1:5]
data=open(inp,'rb').read()
res=add_version_info(data,name,version)
open(out,'wb').write(res)
PY

same=0
case_pt() {   # case_pt HEAD NAME VERSION
    head=$1; name=$2; version=$3; tag=$(echo "$head.$name.$version" | tr -c 'A-Za-z0-9' '_')
    sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T/c-$tag.pe" "$T/p-$tag.pe" || exit 3
    "$B" 10 "$T/peversion" "$T/$head.pe" "$T/c-$tag.pe" "$name" "$version" 2>"$T/c-$tag.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused a sampled point $head $name $version rc=$c $(cat "$T/c-$tag.err")"
    "$B" 10 "$T/peversion" "$T/$head.pe" "$T/c2-$tag.pe" "$name" "$version" 2>/dev/null || fail "C second run rc!=0 $head $name $version"
    cmp -s "$T/c-$tag.pe" "$T/c2-$tag.pe" || fail "C not deterministic $head $name $version"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pyadd.py" "$T/$head.pe" "$T/p-$tag.pe" "$name" "$version" 2>"$T/p-$tag.err" || fail "python refused a sampled point $head $name $version: $(tail -1 "$T/p-$tag.err")"
    cmp -s "$T/c-$tag.pe" "$T/p-$tag.pe" || fail "DIFF $head $name $version"
    same=$((same + 1)); echo "SAME $head $name $version ($(wc -c < "$T/c-$tag.pe" | tr -d ' ') B)"
}
case_pt base unisacc 1.2.3
case_pt base unisacc 1.2.3.4
case_pt base "Unisacc Test App" 65535.65535.65535.65535
case_pt base "Unisacc Test App" 0.0.1

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME HEAD NAME VERSION -- both sides must refuse and write nothing
    nm=$1; head=$2; name=$3; version=$4; tag=neg-$nm
    sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T/$tag-c.pe" "$T/$tag-p.pe" || exit 3
    "$B" 10 "$T/peversion" "$T/$head.pe" "$T/$tag-c.pe" "$name" "$version" 2>"$T/$tag-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pyadd.py" "$T/$head.pe" "$T/$tag-p.pe" "$name" "$version" 2>"$T/$tag-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -e "$T/$tag-c.pe" ] || fail "NEG-$nm C wrote output: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/$tag-c.err")"
}
neg overlay ov unisacc 1.2.3
neg pe32 pe32 unisacc 1.2.3
neg existing-rsrc rsrc unisacc 1.2.3
neg bad-version base unisacc 1.2
neg control-name base "bad$(printf '\001')name" 1.2.3

echo "---- identity ----"
echo "HEAD $H"
echo "peversion.py $(sha256sum unisa/peversion.py | cut -d' ' -f1)"
echo "unisa_peversion.c $(sha256sum seed/unisa_peversion.c | cut -d' ' -f1)"
echo "unisapeversioncheck.sh $(sha256sum tests/unisapeversioncheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisapeversion  same $same  diff 0  neg 5/5"
exit 0
