#!/bin/sh
# LB-1 selftest only: model tables/network, not integrated product behavior.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-hostsymbols.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
b() { "$R/tests/bound" 15 "$@"; }
b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen"
b "$T/gen" lower/hostsymbols "$T/c.json"
b python3 exec/build/gen.py lower/hostsymbols "$T/hostsymbols.json"
cmp "$T/c.json" "$T/hostsymbols.json"
b python3 exec/c/tbl.py "$T/hostsymbols.json" "$T/hostsymbols.tbl"
b python3 exec/c/net.py "$T/hostsymbols.tbl" "$T/hostsymbols.net"
b cc -O2 exec/c/run.c -o "$T/run"
b "$T/run" --check-net "$T/hostsymbols.tbl" "$T/hostsymbols.net"
b python3 - "$T" <<'PY'
import sys, pathlib, struct
sys.path.insert(0,'exec/c')
from pack import build
p=pathlib.Path(sys.argv[1]);m=pathlib.Path('tests/seals/middleflow-step0/lower/model.net').resolve()
# Bind the historical witness to its sealed packet digest, not /tmp identity.
import hashlib
manifest=pathlib.Path('tests/seals/middleflow-step0/manifest.tsv').read_text()
row=[x.split('\t') for x in manifest.splitlines() if x.startswith('lower/model.net\t')]
assert len(row)==1 and len(m.read_bytes())==int(row[0][1]) and hashlib.sha256(m.read_bytes()).hexdigest()==row[0][2]
from networkformat import decode
canonical=p/'frozen-lower.net'; canonical.write_bytes(decode(m.read_bytes())); m=canonical
for kind,model in [('lower',m),('symbols',p/'hostsymbols.net')]:
 route=p/(kind+'.tsv');route.write_text('test\t'+kind+'\ttape\tbytes\t'+str(model)+'\n')
 for tag,address in [('a',0x11223344),('b',0x55667788)]:
  res=p/tag;res.mkdir(exist_ok=True)
  for i in range(4):(res/str(i)).write_bytes(struct.pack('<Q',address+i))
  (p/(kind+'-'+tag+'.pkg')).write_bytes(build([route],[('0070726f636573732f646c2f',res)]))
PY
runner=${MODEL_COM:-./unisacc.com}
"$R/tests/bound" 35 sh "$runner" -run tests/hostsymbolscheck.cx -- "$T"
