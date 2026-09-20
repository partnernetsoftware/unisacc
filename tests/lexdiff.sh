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
from unisa.front.pp import preprocess
from unisa.front.lex import lex
o = Oracle(_built(), drive="built")
src = open(sys.argv[1], encoding="latin-1").read()
# no predefines: unisacc's own preprocessor has none yet, so both sides see
# the same truth flags
src, _ = preprocess(src, o, {})
for t in lex(src, o):
    if t.kind in ("id", "num", "str"):
        print("%s=%s" % (t.kind, t.text.replace("\n", "\\n")))
    else:
        print(t.kind)
PY
)
    if [ "$a" = "$b" ]; then pass=$((pass+1)); printf "  ok   %s\n" "$(basename $f)"
    else fail=$((fail+1)); printf "  FAIL %s\n" "$(basename $f)"
         diff <(echo "$a") <(echo "$b") | head -6; fi
done
echo; echo "lexer agree $pass   differ $fail"
[ "$fail" -eq 0 ]
