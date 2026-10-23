#!/bin/sh
# Standalone byte-codec gate, not a product-package switch.
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
cd "$R"
. ./tests/lib.sh
D=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-codec.XXXXXX")
trap '[ -z "$D" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$D"' EXIT HUP INT TERM
# Never use the shared reference default when this gate is invoked alone.
if [ "$UA" = /tmp/ua_ref ]; then UA="$D/ref"; fi
ua_ready
bound 30 "$UA" -O2 exec/c/codeccheck.c -o "$D/ua"
bound 30 "${CC:-cc}" -O2 exec/c/codeccheck.c -o "$D/cc"
bound 45 python3 exec/c/codeccheck.py "$D/cc" "$D/ua"
if [ "${CODEC_SANITIZE:-0}" = 1 ]; then
 bound 30 "${CC:-cc}" -O1 -g -fsanitize=address,undefined exec/c/codeccheck.c -o "$D/san"
 bound 45 python3 exec/c/codeccheck.py "$D/san"
fi
