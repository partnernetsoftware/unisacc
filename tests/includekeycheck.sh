#!/bin/sh
# includekeycheck (S2 prerequisite): the pp reference (exec/pp/sim.py) and the product request the same include keys.
# Builds the pp delta into a private temporary directory, then runs tests/includekeycheck.py on PRODUCT.
#   usage: tests/includekeycheck.sh PRODUCT
set -u
cd "$(dirname "$0")/.." || exit 2
[ $# -eq 1 ] || { echo "usage: tests/includekeycheck.sh PRODUCT" >&2; exit 2; }
d=$(mktemp -d "${TMPDIR:-/tmp}/includekey.XXXXXX") || exit 2
sh exec/pp/gen-delta.sh "$d/d.json"; r=$?
[ "$r" -eq 0 ] && python3 tests/includekeycheck.py "$1" "$d/d.json" "$d/work"; r=$?
rm -rf "$d"
exit "$r"
