#!/bin/sh
# E1 feasibility, reproducible.  Every step is bounded (macOS: no timeout).
# Usage: exec/lex/run.sh SHARD      SHARD = gen | lexdiff | probes | self | corpus.aa .. corpus.ad
set -u
cd "$(dirname "$0")/../.."
B="perl -e alarm(58);exec(@ARGV)"
D=/tmp/e1delta.json
case "$1" in
gen)     $B ./tests/build_ref.sh /tmp/ua_ref.c /tmp/ua_ref && $B exec/lex/mkpre.sh &&
         $B python3 exec/lex/gen.py $D ;;
lexdiff) $B python3 exec/lex/compare.py $D examples/*.c tests/c/*.c ;;
probes)  $B python3 exec/lex/compare.py $D exec/lex/probes/*.c ;;
self)    $B python3 exec/lex/compare.py $D unisacc.c ;;
corpus.*) [ -f /tmp/e1corp.aa ] || { find "${CORPUS:-../../corpus}" -name '*.c' | sort > /tmp/e1corp.txt
                                     split -l 63 /tmp/e1corp.txt /tmp/e1corp.; }
         $B python3 exec/lex/compare.py $D @/tmp/e1corp.${1#corpus.} ;;
esac
