#!/bin/sh
# ARM common lowering: entry, registers and syscall ABI shapes, both executors.
# ARM-specific instruction fusions are still pending.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/lower/gen.py "$T/d.json" --full --arm64
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
printf 'imm r0, 0\nret\n' > "$T/plain"
b env LOWER_TARGET=lnx/arm64 python3 exec/lower/fullcheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/plain"
