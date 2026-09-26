#!/bin/sh
# E1 feasibility, reproducible.  Every step is bounded (macOS: no timeout).
# Usage: exec/lex/run.sh SHARD      SHARD = gen | xbuild | net | lexdiff | probes | self | corpus.aa .. corpus.ad
# xbuild: the delta as an exec/exec.c table, the executor built with cc and
# with unisacc, T1 of each build's loaded map against the delta.  After it,
# E1EXEC=$X/e1x/exec_cc:$X/e1x/e1.tbl (or exec_ua) makes the comparison
# shards also require the C executor's result to equal sim.py's.
# Artefacts live in $X (exec/stamp.sh: per checkout, stamped with their
# sources); every comparison shard first makes sure they are current.
set -u
cd "$(dirname "$0")/../.."
. exec/stamp.sh
B="perl -e alarm(58);exec(@ARGV)"
D=$X/e1delta.json
export E1REF=$X/ua_ref E1PRE=$X/ua_pre
ready() {
    fresh $X/ua_ref $B ./tests/build_ref.sh $X/ua_ref.c $X/ua_ref -- $REFSRC &&
    fresh $X/ua_pre $B exec/lex/mkpre.sh $X/ua_ref.c $X/ua_pre -- $X/ua_ref.c exec/lex/mkpre.sh &&
    fresh $D $B python3 exec/lex/gen.py $D -- exec/lex/*.py $PYSRC
}
case "$1" in
gen)     ready ;;
*)       ready || exit 1 ;;
esac
case "$1" in
xbuild)  mkdir -p $X/e1x && $B python3 exec/lex/tbl.py $D $X/e1x/e1.tbl &&
         $B cc -std=c99 -Os -w -o $X/e1x/exec_cc exec/exec.c &&
         $B $X/ua_ref exec/exec.c -o $X/e1x/exec_ua &&
         for x in cc ua; do
           $B $X/e1x/exec_$x -fdump $X/e1x/e1.tbl > $X/e1x/fd_$x &&
           printf '%s ' $x && $B python3 exec/lex/tbl.py check $D $X/e1x/fd_$x || exit 1
         done ;;
net)     $B python3 exec/lex/net.py $D $X/e1x && $B python3 exec/lex/tbl.py $X/e1x/e1net.json $X/e1x/e1net.tbl &&
         cmp $X/e1x/e1net.tbl $X/e1x/e1.tbl && echo "net-derived table identical to $X/e1x/e1.tbl" ;;
lexdiff) $B python3 exec/lex/compare.py $D examples/*.c tests/c/*.c ;;
probes)  $B python3 exec/lex/compare.py $D exec/lex/probes/*.c ;;
self)    $B python3 exec/lex/compare.py $D unisacc.c ;;
corpus.*) # the list is rebuilt when the corpus changes (a cached one skips files)
         find "${CORPUS:-../../corpus}" -name '*.c' | sort > $X/e1corp.new
         cmp -s $X/e1corp.new $X/e1corp.txt || { mv $X/e1corp.new $X/e1corp.txt; rm -f $X/e1corp.a?; split -l 63 $X/e1corp.txt $X/e1corp.; }
         $B python3 exec/lex/compare.py $D @$X/e1corp.${1#corpus.} ;;
esac
