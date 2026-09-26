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
# All six Linux predefines have value 1; other platform/architecture names
# must be absent. Exercise the generated E2 and the product -E independently.
case $TARGET in lnx/arm64) arch=__aarch64__; other=__x86_64__;; *) arch=__x86_64__; other=__aarch64__;; esac
printf '%s\n' __linux__ __unix__ __ELF__ "$arch" __LP64__ __UNISA__ > "$T/macros.c"
for name in "$other" __APPLE__ __MACH__ _WIN32 _WIN64; do
    printf '#ifdef %s\nWRONG_TARGET\n#endif\n' "$name" >> "$T/macros.c"
done
b "$T/run" "$T/e2.tbl" "$T/macros.c" > "$T/macros.delta"
b "$UA" -b "$TARGET" -E "$T/macros.c" > "$T/macros.ref"
for file in "$T/macros.delta" "$T/macros.ref"; do
    [ "$(tr -d '[:space:]' < "$file")" = 111111 ] || { echo 'target predefines differ' >&2; exit 1; }
done
b "$UA" -O2 -b "$TARGET" -S unisacc.c -o "$T/ref.tape"
[ -s "$T/unisacc.e4" ] && cmp "$T/ref.tape" "$T/unisacc.e4"
b "$UA" -O2 -b "$TARGET" unisacc.c -o "$T/ref.elf"
[ -s "$T/unisacc.elf" ] && cmp "$T/ref.elf" "$T/unisacc.elf"
echo "delta self-source ELF: $(wc -c < "$T/unisacc.elf" | tr -d ' ') bytes equal; Linux execution checked separately"
