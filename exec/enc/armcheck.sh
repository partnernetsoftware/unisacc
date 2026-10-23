#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/build/gen.py enc/arm "$T/arm.json"
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b python3 exec/enc/armcheck.py "$T/run" "$T/arm.tbl" "$T/arm.json"
b python3 exec/enc/realcheck.py "$T/run" "$T/arm.tbl" arm64
wc -c "$T/arm.tbl"
