#!/bin/bash
# Run suites on a FROZEN copy of the working tree, so the tree can keep
# changing while they run.  [owner, 2026-09-25: tests in the background,
# design in the foreground]
#
#   tests/snap.sh opt c99 "closure examples/*.c tests/c/*.c" corpus:1/4 ...
#
# A suite that overlapped an edit once reported 59 mismatches from images
# that were never written (AGENTS.md); another lost four corpus programs
# because parse.py changed under it.  The copy (minus .git, with corpus/
# linked back -- the suites only read it) gets its own reference compiler,
# so /tmp/ua_ref can be rebuilt meanwhile.  Each suite is bounded at 58 s.
# `corpus:K/N` runs one shard.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
S=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-snap.XXXXXX")
trap 'rm -rf "$S"' EXIT
(cd "$R" && tar cf - --exclude=.git --exclude=corpus --exclude=ujs .) | (cd "$S" && tar xf -)
[ -d "$R/corpus" ] && ln -s "$R/corpus" "$S/corpus"
cd "$S"
export UA="$S/ua_ref"
./tests/build_ref.sh "$S/ua_ref.c" "$UA" >/dev/null || { echo "snap: reference build failed"; exit 1; }
rc=0
for t in "$@"; do
    case "$t" in
    corpus:*) sh=${t#corpus:}; cmd="SHARD=$sh FETCH=0 ./tests/corpus.sh"; name="corpus $sh";;
    *) cmd="./tests/$t"; cmd="${cmd%% *}.sh ${t#* }"; [ "$t" = "${t%% *}" ] && cmd="./tests/$t.sh"; name=${t%% *};;
    esac
    s=$(date +%s)
    out=$(perl -e 'alarm 58; exec @ARGV' bash -c "$cmd" 2>&1); r=$?
    [ $r -ne 0 ] && rc=1
    printf "%-14s rc=%-3s %3ss :: %s\n" "$name" "$r" "$(( $(date +%s)-s ))" "$(printf '%s\n' "$out" | grep -v '^$' | tail -1)"
    [ $r -ne 0 ] && printf '%s\n' "$out" | grep -E "FAIL|DIFF|WRONG|REGRESSION|STALE" | head -5 | sed 's/^/    /'
done
exit $rc
