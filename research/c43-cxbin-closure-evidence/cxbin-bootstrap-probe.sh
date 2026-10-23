#!/bin/sh
set -eu
P=/mnt/out/cxbin-bootstrap-probe
mkdir -p "$P/tree/tests" "$P/tree/include" "$P/cache" "$P/safe-target"
chmod 700 "$P/cache"
cp tests/cxbin tests/safedel.cx "$P/tree/tests/"
cp include/*.h "$P/tree/include/"
CXBIN_CACHE="$P/cache" sh tests/bound 55 sh "$P/tree/tests/cxbin" "$P/tree/tests/safedel.cx" --dry-run -- "$P/safe-target"
printf 'bootstrap-without-product\t0\n'
printf 'int main(void) { return 0; }\n' > "$P/plain source.cx"
CXBIN_CACHE="$P/cache" sh tests/bound 55 sh tests/cxbin "$P/plain source.cx"
printf 'plain-cx\t0\n'
