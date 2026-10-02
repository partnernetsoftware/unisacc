#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Separate bounded batch: same generic executor, an ELF-output delta.
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/gen.py "$T/elf.json" --elf
b python3 exec/c/tbl.py "$T/elf.json" "$T/elf.tbl"
b python3 exec/enc/imagecheck.py "$T/run" "$T/elf.tbl" "$T/elf.json"
b python3 exec/enc/arm.py "$T/arm.json" --elf
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b ./tests/build_ref.sh "$T/ref.c" "$T/ref"
b python3 exec/enc/dynelfcheck.py "$T/ref" "$T/run" "$T/elf.tbl" "$T/arm.tbl" "$T/elf.json"
