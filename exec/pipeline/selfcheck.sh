#!/bin/sh
# Current compiler source through six deltas to the selected Linux ELF.
# The output compiles the existing C compiler; it is not E7 product adoption.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
TARGET=${TARGET:-lnx/x86_64}; export TARGET
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b ./exec/pipeline/elf.sh "$T" unisacc.c > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
b "$UA" -O2 -b "$TARGET" -S unisacc.c -o "$T/ref.tape"
[ -s "$T/unisacc.e4" ] && cmp "$T/ref.tape" "$T/unisacc.e4"
b "$UA" -O2 -b "$TARGET" unisacc.c -o "$T/ref.elf"
[ -s "$T/unisacc.elf" ] && cmp "$T/ref.elf" "$T/unisacc.elf"
echo "delta self-source ELF: $(wc -c < "$T/unisacc.elf" | tr -d ' ') bytes equal; Linux execution checked separately"
