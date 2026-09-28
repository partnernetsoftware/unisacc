#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/arm.py "$T/arm.json"
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b python3 exec/enc/armwincheck.py "$T/run" "$T/arm.tbl" "$T/arm.json"
b env REAL_TARGETS=win/arm64 python3 exec/enc/realcheck.py "$T/run" "$T/arm.tbl" arm64
