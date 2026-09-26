#!/bin/sh
# E2 feasibility (minimum slice), reproducible.  Every step bounded (58 s).
# Usage: exec/pp/run.sh SHARD    SHARD = gen | ex.aa .. ex.af | corpus.aa ..
# E2TMP (default $X/e2, exec/stamp.sh: per checkout) holds the reference
# binaries, table and shard lists, each stamped with its sources and rebuilt
# when they change; every shard first makes sure they are current.
set -u
cd "$(dirname "$0")/../.."
. exec/stamp.sh
T=${E2TMP:-$X/e2}; mkdir -p "$T"
B="perl -e alarm(58);exec(@ARGV)"
export E2REF=$T/ua_ref E2NOAUTO=$T/ua_noauto
ready() {
    fresh $T/ua_ref $B ./tests/build_ref.sh $T/ua_ref.c $T/ua_ref -- $REFSRC &&
    fresh $T/ua_noauto $B exec/pp/mknoauto.sh $T/ua_ref.c $T/ua_noauto -- $T/ua_ref.c exec/pp/mknoauto.sh &&
    fresh $T/d.json $B python3 exec/pp/gen.py $T/d.json -- exec/pp/*.py $PYSRC $T/ua_ref.stamp
}
case "$1" in
gen)      ready ;;
*)        ready || exit 1 ;;
esac
case "$1" in
ex.*)     # the list is rebuilt when the probe set changes: a cached one
         # silently skipped three probes added after it was written
         ls examples/*.c tests/c/*.c > "$T/ex.new"
         cmp -s "$T/ex.new" "$T/ex.txt" || { mv "$T/ex.new" "$T/ex.txt"; rm -f "$T"/ex.a?; split -l 20 "$T/ex.txt" "$T/ex."; }
          $B python3 exec/pp/compare.py "$T/d.json" "@$T/$1" ;;
corpus.*) find "${CORPUS:-../../corpus}" -name '*.c' | sort > "$T/corpus.new"
          cmp -s "$T/corpus.new" "$T/corpus.txt" || { mv "$T/corpus.new" "$T/corpus.txt"; rm -f "$T"/corpus.a?; split -l 40 "$T/corpus.txt" "$T/corpus."; }
          $B python3 exec/pp/compare.py "$T/d.json" "@$T/$1" ;;
esac
