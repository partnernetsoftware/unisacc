#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
b cc -O2 -o "$T/run" exec/c/run.c
b "$UA" -S examples/hello.c -o "$T/hello"
b "$UA" -O2 -S examples/fib.c -o "$T/fib"
for ARCH in x86_64 arm64; do
    case $ARCH in arm64) FLAG=--arm64;; *) FLAG=;; esac
    b python3 exec/lower/gen.py "$T/d.json" --full --win $FLAG
    b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
    b env LOWER_TARGET=win/$ARCH python3 exec/lower/fullcheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/hello" "$T/fib"
done
