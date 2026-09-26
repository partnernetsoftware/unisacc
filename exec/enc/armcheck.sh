#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/arm.py "$T/arm.json"
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b python3 exec/enc/armcheck.py "$T/run" "$T/arm.tbl" "$T/arm.json"
wc -c "$T/arm.tbl"
