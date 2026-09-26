#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/gen.py "$T/d.json"
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
b python3 exec/enc/x86wincheck.py "$T/run" "$T/d.tbl" "$T/d.json"
