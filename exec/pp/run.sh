#!/bin/sh
# E2 feasibility (minimum slice), reproducible.  Every step bounded (58 s).
# Usage: exec/pp/run.sh SHARD    SHARD = gen | ex.aa .. ex.af | corpus.aa ..
# E2TMP (default /tmp/e2) holds the reference binaries, table and shard lists.
set -u
cd "$(dirname "$0")/../.."
T=${E2TMP:-/tmp/e2}; mkdir -p "$T"
B="perl -e alarm(58);exec(@ARGV)"
export E2REF=$T/ua_ref E2NOAUTO=$T/ua_noauto
case "$1" in
gen)      $B ./tests/build_ref.sh "$T/ua_ref.c" "$T/ua_ref" && $B exec/pp/mknoauto.sh "$T/ua_ref.c" "$T/ua_noauto" &&
          $B python3 exec/pp/gen.py "$T/d.json" ;;
ex.*)     [ -f "$T/ex.aa" ] || { ls examples/*.c tests/c/*.c > "$T/ex.txt"; split -l 20 "$T/ex.txt" "$T/ex."; }
          $B python3 exec/pp/compare.py "$T/d.json" "@$T/$1" ;;
corpus.*) [ -f "$T/corpus.aa" ] || { find "${CORPUS:-../../corpus}" -name '*.c' | sort > "$T/corpus.txt"
                                     split -l 40 "$T/corpus.txt" "$T/corpus."; }
          $B python3 exec/pp/compare.py "$T/d.json" "@$T/$1" ;;
esac
