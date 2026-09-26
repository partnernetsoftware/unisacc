#!/bin/sh
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/lower/gen.py "$T/d.json" --full
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
b "$UA" -S examples/hello.c -o "$T/hello"
b "$UA" -O2 -S examples/fib.c -o "$T/fib"
b python3 exec/lower/fullcheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/hello" "$T/fib"
