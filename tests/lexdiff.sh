#!/bin/bash
# The C lexer in unisacc.c vs the Python one -- both driven by the same table.
set -u
UA=${UA:-/tmp/ua_ref}
pass=0; fail=0
for f in "$@"; do
    a=$($UA "$f" | sed '$d')
    b=$(python3 - "$f" <<'PY'
import sys
sys.path.insert(0, '.')
from unisa.__main__ import _built
from unisa.oracle import Oracle
from unisa.front.pp import preprocess, expand, expand_positional, predefines
from unisa.front.lex import lex
o = Oracle(_built(), drive="built")
src = open(sys.argv[1], encoding="latin-1").read()
# no predefines: unisacc's own preprocessor has none yet, so both sides see
# the same truth flags.  Macros are EXPANDED on both sides -- the C
# preprocessor substitutes text now, so a lexer comparison that skipped the
# expansion would be comparing two different programs.
# ...and headers are spliced on both sides: the C preprocessor implements
# #include now, looking beside the file and then in include/.
import os
# both front ends predefine the target's macros; a bare run is lnx/x86_64
src, macros = preprocess(src, o, predefines("lnx/x86_64"), sys.argv[1], ("include",))
# expanded POSITIONALLY: each stretch with the macros in force there
src = expand_positional(src, macros)
# the source was read as latin-1 (one char per byte), so write the spelling
# back out as bytes -- otherwise a UTF-8 literal comes out re-encoded and
# differs from the C side for no reason
w = sys.stdout.buffer.write
for t in lex(src, o):
    if t.kind in ("id", "num", "str"):
        w(("%s=%s\n" % (t.kind, t.text.replace("\n", "\\n")))
          .encode("latin-1"))
    else:
        w((t.kind + "\n").encode("latin-1"))
PY
)
    if [ "$a" = "$b" ]; then pass=$((pass+1)); printf "  ok   %s\n" "$(basename $f)"
    else fail=$((fail+1)); printf "  FAIL %s\n" "$(basename $f)"
         diff <(echo "$a") <(echo "$b") | head -6; fi
done
echo; echo "lexer agree $pass   differ $fail"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]
