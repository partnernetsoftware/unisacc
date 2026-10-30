#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/build/gen.py lower "$T/l.json" --full
b python3 exec/build/gen.py enc "$T/e.json" --elf
for s in l e; do b python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl"; done
b python3 exec/lower/sparsecheck.py "$T/run" "$T/l.tbl" "$T/l.json" "$T/e.tbl" "$T/e.json"
