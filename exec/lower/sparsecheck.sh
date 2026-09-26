#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/lower/gen.py "$T/l.json" --full
b python3 exec/enc/gen.py "$T/e.json" --elf
for s in l e; do b python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl"; done
b python3 exec/lower/sparsecheck.py "$T/run" "$T/l.tbl" "$T/l.json" "$T/e.tbl" "$T/e.json"
