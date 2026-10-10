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
B="tests/bound 58"
export E2REF=$T/ua_ref E2NOAUTO=$T/ua_noauto
ready() {
    # 0.0.40 K5-1e: each step stops ready on its own failure (the old && chain ended at the d.json step)
    fresh $T/ua_ref $B ./tests/build_ref.sh $T/ua_ref.c $T/ua_ref -- $REFSRC || return 1
    fresh $T/ua_noauto $B exec/pp/mknoauto.sh $T/ua_ref.c $T/ua_noauto -- $T/ua_ref.c exec/pp/mknoauto.sh || return 1
    # 0.0.40 K5-1e (机房主任 02:59): pp δ through exec/pp/gen-delta.sh (seed-gen first, no Python fallback).  The
    # seed-gen cache is a one-time directory made inside the command, so its random path never enters the fresh
    # command text; the text carries only the helper-relevant environment (SEED_GEN, SEED_GEN_BIN, SEED_GEN_CC and
    # the compiler path it resolves to), so a warm d.json is missed when that configuration changes.  The seed
    # sources and the helper are inputs: an edited seed header rebuilds d.json.
    _sgcc=${SEED_GEN_CC:-cc}
    _sgenv="SEED_GEN=${SEED_GEN-<unset>} SEED_GEN_BIN=${SEED_GEN_BIN-<unset>} SEED_GEN_CC=${SEED_GEN_CC-<unset>} cc=$(command -v "$_sgcc" || echo '<none>')"
    fresh $T/d.json $B sh -c 'd=$(mktemp -d "${TMPDIR:-/tmp}/seedgen-pprun.XXXXXX") || exit 2; SEED_GEN_DIR="$d" sh exec/pp/gen-delta.sh "$1"; r=$?; rm -rf "$d"; exit $r' "$_sgenv" $T/d.json -- exec/finite_rules.py exec/assemble.py exec/build/*.py exec/pp/*.py exec/pp/*.tsv exec/facts/pp-*.tsv weights/gold/pp.tsv $PYSRC $T/ua_ref.stamp exec/pp/gen-delta.sh seed/*.c seed/*.h
}
case "$1" in
gen)      ready || exit 1 ;;   # 0.0.40 K5-1e: a failed ready is a failed gen (it ended 0 through the next case)
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
