#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
T=$(mktemp -d); trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
b() { "$_BOUND" 60 "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
# tests/lib.sh replaced the trap above: one trap for both scratch directories, through the one deletion guard
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}" "${_LIB_TMP:?}"' EXIT
b cc -O2 -o "$T/run" exec/c/run.c
b "$UA" -S examples/hello.c -o "$T/hello"
b "$UA" -O2 -S examples/fib.c -o "$T/fib"
for ARCH in x86_64 arm64; do
    case $ARCH in arm64) FLAG=--arm64;; *) FLAG=;; esac
    b python3 exec/build/gen.py lower "$T/d.json" --full --win $FLAG
    b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
    b env LOWER_TARGET=win/$ARCH python3 exec/lower/fullcheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/hello" "$T/fib"
done
