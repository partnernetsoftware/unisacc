#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
b() { "$_BOUND" "$@"; }
# Build /tmp/ua_tdump: the reference (/tmp/ua_ref.c from tests/build_ref.sh)
# with one harness hook on the -dump-tokens path.  Nothing in src/ changes;
# the patch is applied to the generated single-file copy (as exec/lex/mkpre.sh).
#   UA_TYPESPELL=1   a `type` token is printed `type=SPELLING` (its source
#                    span), like id=/num=/str=; without it the dump is
#                    byte-identical to the reference's.
set -e
IN=${1:-/tmp/ua_ref.c}; OUT=${2:-/tmp/ua_tdump}
b 60 python3 - "$IN" "$OUT.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_bytes()
before = b'        if (tkind[i] == 2) { __write(1, "=", 1);'
after = b'        if (L == 4 && TOKV[p] == 116 && TOKV[p+1] == 121 && TOKV[p+2] == 112 && TOKV[p+3] == 101 && getenv("UA_TYPESPELL")) { __write(1, "=", 1); __write(1, src + tpos[i], tlen[i]); }\n        if (tkind[i] == 2) { __write(1, "=", 1);'
if source.count(before) != 1: raise SystemExit('expected exactly one patch anchor')
Path(sys.argv[2]).write_bytes(source.replace(before, after, 1))
PY
# The -dump-tokens path skips autoinc(), which the compile path runs (src/front_parse.c:
# splice, decomment, autoinc, preprocess): with two bundled headers the dump then lists
# them in the other order from what is compiled.  E3 must read what is compiled.
b 60 python3 - "$OUT.c" "$OUT.c" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_bytes()
before = b'    splice();\n    decomment();\n    preprocess();\n    expandsrc();\n    if (lex() < 0) return 1;\n    i = 0;'
after = b'    splice();\n    decomment();\n    autoinc();\n    preprocess();\n    expandsrc();\n    if (lex() < 0) return 1;\n    i = 0;'
if source.count(before) != 1: raise SystemExit('expected exactly one patch anchor')
Path(sys.argv[2]).write_bytes(source.replace(before, after, 1))
PY
grep -q "    autoinc();" "$OUT.c"
grep -q UA_TYPESPELL "$OUT.c"
b 60 cc -w -std=c99 ${CFLAGS:--O2} -o "$OUT" "$OUT.c"
