#!/bin/sh
# 0.0.32 B5: the product container without Python -- UA -b images + seed/ape.c (-DAPE_ZLIB) with a
# payload and VERSIONINFO -- byte-identical to `python3 -m unisa ape --via UA --payload ... --product-*`.
# UA is a private same-source reference (tests/build_ref.sh).     usage: tests/seedapecheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-seedape.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "seedape: $*"; exit 1; }
"$B" 50 tests/build_ref.sh "$T/ref.c" "$T/ua" >/dev/null 2>&1 || fail "same-source reference does not build"
"$B" 30 cc -std=c99 -O2 -w -DAPE_ZLIB -o "$T/ape" seed/ape.c -lz || fail "ape.c does not build"
printf 'P 3 0 0 0\n' > "$T/pkg"
"$B" 50 python3 -m unisa ape exec/c/asmcompiler.c --via "$T/ua" -O2 --payload "$T/pkg" --product-name Unisacc --product-version 0.0.32 -o "$T/py.com" >/dev/null 2>&1 &
pp=$!
for t in win/x86_64 lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64; do
    "$B" 50 "$T/ua" -O2 -b "$t" exec/c/asmcompiler.c -o "$T/slice-$(echo "$t" | tr / -)" || fail "UA -b $t"
done
"$B" 30 "$T/ape" --payload "$T/pkg" --product Unisacc 0.0.32 "$T/c.com" "$T/slice-win-x86_64" "$T/slice-lnx-x86_64" \
    "$T/slice-lnx-arm64" "$T/slice-osx-x86_64" "$T/slice-osx-arm64" >/dev/null || fail "seed ape failed"
wait $pp || fail "unisa ape reference failed"
cmp -s "$T/py.com" "$T/c.com" || fail "container differs"
"$B" 30 "$T/ape" --payload "$T/ref.c" "$T/x.com" "$T/slice-win-x86_64" "$T/slice-lnx-x86_64" \
    "$T/slice-lnx-arm64" "$T/slice-osx-x86_64" "$T/slice-osx-arm64" >/dev/null 2>&1 && fail "non-package payload accepted"
echo "seedape  same 1  ($(wc -c < "$T/c.com" | tr -d ' ') B, payload + VERSIONINFO)  refusals 1"
