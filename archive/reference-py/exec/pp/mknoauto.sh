#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
b() { "$_BOUND" "$@"; }
# Build the reference WITHOUT autoinc() (the on-demand header prepend, not in
# the E2 minimum slice) from the generated single-file copy.  src/ is not
# touched.  Usage: exec/pp/mknoauto.sh REF.c OUT
set -e
IN=${1:?usage: mknoauto.sh REF.c OUT}; OUT=${2:?}
b 60 python3 - "$IN" "$OUT.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_bytes()
before = b'    decomment();\n    autoinc();\n    preprocess();'
after = b'    decomment();\n    preprocess();'
if source.count(before) != 1: raise SystemExit('expected exactly one patch anchor')
Path(sys.argv[2]).write_bytes(source.replace(before, after, 1))
PY
! grep -q '^    autoinc();$' "$OUT.c"
b 60 cc -w -std=c99 -O1 -o "$OUT" "$OUT.c"
