#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/arm.py "$T/elf.json" --elf
b python3 exec/c/tbl.py "$T/elf.json" "$T/elf.tbl"
b python3 exec/enc/imagecheck.py "$T/run" "$T/elf.tbl" "$T/elf.json" arm64
