#!/bin/sh
# Build the reference WITHOUT autoinc() (the on-demand header prepend, not in
# the E2 minimum slice) from the generated single-file copy.  src/ is not
# touched.  Usage: exec/pp/mknoauto.sh REF.c OUT
set -e
IN=${1:?usage: mknoauto.sh REF.c OUT}; OUT=${2:?}
perl -0pe 's/(    decomment\(\);\n)    autoinc\(\);\n(    preprocess\(\);)/$1$2/' "$IN" > "$OUT.c"
! grep -q '^    autoinc();$' "$OUT.c"
cc -w -std=c99 -O1 -o "$OUT" "$OUT.c"
