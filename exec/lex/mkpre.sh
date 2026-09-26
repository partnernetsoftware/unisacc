#!/bin/sh
# Build /tmp/ua_pre: the reference (/tmp/ua_ref.c from tests/build_ref.sh)
# with two harness hooks on the -dump-tokens path, right where lex() is
# called.  Nothing in src/ changes; the patch is applied to the generated
# single-file copy.
#   UA_LEXIN=path   write the lexer's input buffer src[0..nsrc) to path, exit 0
#   UA_LEXREJ=p     render reject position p with the reference's own
#                   err_at (the file:line:col map belongs to the pp layer), exit 1
set -e
IN=${1:-/tmp/ua_ref.c}; OUT=${2:-/tmp/ua_pre}
perl -0pe 's/(    expandsrc\(\);\n)(    if \(lex\(\) < 0\) return 1;\n    i = 0;\n    while \(i < ntok\))/$1    { char *e_; e_ = getenv("UA_LEXIN"); if (e_) { int f_; f_ = open(e_, O_WRONLY|O_CREAT|O_TRUNC, 0644); write(f_, src, nsrc); close(f_); return 0; }\n      e_ = getenv("UA_LEXREJ"); if (e_) { err_at(atol(e_), "unexpected character"); return 1; } }\n$2/' "$IN" > "$OUT.c"
grep -q UA_LEXIN "$OUT.c"
cc -w -std=c99 -O1 -o "$OUT" "$OUT.c"
