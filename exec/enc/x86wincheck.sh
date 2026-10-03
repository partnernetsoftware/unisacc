#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/build/gen.py enc "$T/d.json"
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
b python3 exec/c/net.py "$T/d.tbl" "$T/d.net"
b "$T/run" --check-net "$T/d.tbl" "$T/d.net"
for model in tbl net; do
    b python3 exec/enc/x86wincheck.py "$T/run" "$T/d.$model" "$T/d.json"
    b env REAL_TARGETS=win/x86_64 python3 exec/enc/realcheck.py "$T/run" "$T/d.$model"
done
