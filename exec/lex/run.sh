#!/bin/sh
# E1 feasibility, reproducible.  Every step is bounded (macOS: no timeout).
# Usage: exec/lex/run.sh SHARD      SHARD = gen | xbuild | lexdiff | probes | self | corpus.aa .. corpus.ad
# xbuild: the delta as an exec/exec.c table, the executor built with cc and
# with unisacc, T1 of each build's loaded map against the delta.  After it,
# E1EXEC=/tmp/e1x/exec_cc:/tmp/e1x/e1.tbl (or exec_ua) makes the comparison
# shards also require the C executor's result to equal sim.py's.
set -u
cd "$(dirname "$0")/../.."
B="perl -e alarm(58);exec(@ARGV)"
D=/tmp/e1delta.json
case "$1" in
gen)     $B ./tests/build_ref.sh /tmp/ua_ref.c /tmp/ua_ref && $B exec/lex/mkpre.sh &&
         $B python3 exec/lex/gen.py $D ;;
xbuild)  mkdir -p /tmp/e1x && $B python3 exec/lex/tbl.py $D /tmp/e1x/e1.tbl &&
         $B cc -std=c99 -Os -w -o /tmp/e1x/exec_cc exec/exec.c &&
         $B /tmp/ua_ref exec/exec.c -o /tmp/e1x/exec_ua &&
         for x in cc ua; do
           $B /tmp/e1x/exec_$x -fdump /tmp/e1x/e1.tbl > /tmp/e1x/fd_$x &&
           printf '%s ' $x && $B python3 exec/lex/tbl.py check $D /tmp/e1x/fd_$x || exit 1
         done ;;
lexdiff) $B python3 exec/lex/compare.py $D examples/*.c tests/c/*.c ;;
probes)  $B python3 exec/lex/compare.py $D exec/lex/probes/*.c ;;
self)    $B python3 exec/lex/compare.py $D unisacc.c ;;
corpus.*) [ -f /tmp/e1corp.aa ] || { find "${CORPUS:-../../corpus}" -name '*.c' | sort > /tmp/e1corp.txt
                                     split -l 63 /tmp/e1corp.txt /tmp/e1corp.; }
         $B python3 exec/lex/compare.py $D @/tmp/e1corp.${1#corpus.} ;;
esac
