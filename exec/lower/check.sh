#!/bin/sh
# Raw -S tape (same format as E3/E4) into the delta data pass.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/lower/gen.py "$T/d.json"
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
b "$UA" -S examples/hello.c -o "$T/hello"
b "$UA" -S examples/fib.c -o "$T/fib0"
b python3 exec/opt/gen.py "$T/e4.json" 2
b python3 exec/c/tbl.py "$T/e4.json" "$T/e4.tbl"
b "$T/run" "$T/e4.tbl" "$T/fib0" > "$T/fib"
b python3 exec/lower/check.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/hello" "$T/fib"
# Same layout transitions, explicit ARM target; instructions remain raw tape.
b python3 exec/lower/gen.py "$T/arm.json" --arm64
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b env LOWER_TARGET=lnx/arm64 python3 exec/lower/check.py "$T/run" "$T/arm.tbl" "$T/arm.json" "$T/hello" "$T/fib"
# Windows has extra runtime cells and a logical software-stack BSS region.
for ARCH in x86_64 arm64; do
    case $ARCH in arm64) FLAG=--arm64;; *) FLAG=;; esac
    b python3 exec/lower/gen.py "$T/win.json" --win $FLAG
    b python3 exec/c/tbl.py "$T/win.json" "$T/win.tbl"
    b env LOWER_TARGET=win/$ARCH python3 exec/lower/check.py "$T/run" "$T/win.tbl" "$T/win.json" "$T/hello" "$T/fib"
done
