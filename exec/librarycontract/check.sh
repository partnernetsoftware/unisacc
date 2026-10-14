#!/bin/sh
# 2a model only. Existing product is a dynamic-C observation tool.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-librarycontract.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
b(){ "$R/tests/bound" 15 "$@"; }
b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen"
b "$T/gen" librarycontract/gen "$T/c.json"
b python3 exec/build/gen.py librarycontract/gen "$T/model.json"
cmp "$T/c.json" "$T/model.json"
b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
b cc -O2 exec/c/run.c -o "$T/run"
b "$T/run" --check-net "$T/model.tbl" "$T/model.net"
"$R/tests/bound" 40 sh "${MODEL_COM:-./unisacc.com}" -run tests/librarycontractcheck.cx -- "$T"
