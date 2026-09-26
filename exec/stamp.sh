#!/bin/sh
# Stamped artefacts for the exec/ harnesses.  Three times a stale file in the
# shared /tmp passed for a current one (an old /tmp/ua_ref, a cached shard
# list, an old /tmp/ua_pre and /tmp/e1delta.json).  Every generated artefact
# now goes through `fresh`, which rebuilds it when its sources changed, and
# lives under a directory private to this checkout, so parallel worktrees do
# not overwrite each other's (tests/lib.sh ua_ready is the same idea for UA).
#
# Run from the repo root (the scripts cd there first).
# Sourced:  . exec/stamp.sh   ->  $X (the artefact dir), $REFSRC, fresh()
# Command:  sh exec/stamp.sh dir                         print $X
#           sh exec/stamp.sh fresh OUT CMD... -- INPUT...
#
# fresh OUT CMD... -- INPUT...
#   runs CMD (each word quoted as given) when OUT is missing or OUT.stamp
#   differs from the cksum of the CMD text, the INPUT names and their
#   contents; on success writes OUT.stamp, on failure leaves none.
# UNISACC_EXEC_TMP overrides the directory.
_SR=$(pwd)
if [ -n "${UNISACC_EXEC_TMP:-}" ]; then X=$UNISACC_EXEC_TMP
else X="${TMPDIR:-/tmp}"; X="${X%/}/unisacc-exec/$(printf %s "$_SR" | cksum | cut -d' ' -f1)"; fi
mkdir -p "$X"
# what tests/build_ref.sh builds the reference compiler from
REFSRC="$(ls kernel/*.inc kernel/*.c src/*.c | tr '\n' ' ')tests/refshim.h tests/reffoot.h tests/build_ref.sh"
# what a delta generator reads besides its own directory
PYSRC="$(find unisa -type f \( -name '*.py' -o -name '*.tsv' \) | sort | tr '\n' ' ')"

fresh() {
    _o=$1; shift
    _c=""
    while [ $# -gt 0 ] && [ "$1" != -- ]; do
        _c="$_c '$(printf %s "$1" | sed "s/'/'\\\\''/g")'"; shift; done
    [ "${1:-}" = -- ] && shift
    _w=$( { printf '%s\n' "$_c" "$@"; cat "$@"; } | cksum)
    [ -e "$_o" ] && [ "$(cat "$_o.stamp" 2>/dev/null)" = "$_w" ] && return 0
    echo "fresh: (re)building $_o" >&2
    rm -f "$_o.stamp"
    eval "$_c" || return $?
    printf '%s\n' "$_w" > "$_o.stamp"
}

case "$0" in
*stamp.sh)
    case "${1:-}" in
    dir)   echo "$X" ;;
    fresh) shift; fresh "$@" ;;
    *)     echo "usage: $0 dir | fresh OUT CMD... -- INPUT..." >&2; exit 2 ;;
    esac ;;
esac
