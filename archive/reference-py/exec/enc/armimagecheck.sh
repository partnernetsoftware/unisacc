#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/build/gen.py enc/arm "$T/elf.json" --elf
b python3 exec/c/tbl.py "$T/elf.json" "$T/elf.tbl"
b python3 exec/enc/imagecheck.py "$T/run" "$T/elf.tbl" "$T/elf.json" arm64
