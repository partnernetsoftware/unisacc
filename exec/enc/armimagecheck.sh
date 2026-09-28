#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/arm.py "$T/elf.json" --elf
b python3 exec/c/tbl.py "$T/elf.json" "$T/elf.tbl"
b python3 exec/enc/imagecheck.py "$T/run" "$T/elf.tbl" "$T/elf.json" arm64
