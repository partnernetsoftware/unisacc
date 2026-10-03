#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Real compiler source through three deltas, one generic C executor.
# Python generates transition tables; it does not process the source at runtime.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b python3 tests/sourceflat.py "$T/unisacc-flat.c"
SELF="$T/unisacc-flat.c"
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/build/gen.py pp "$T/pp.json" > "$T/gen.log" 2>&1
b python3 exec/build/gen.py lex "$T/lex.json" --typed >> "$T/gen.log" 2>&1
b python3 exec/build/gen.py parse2 "$T/parse.json" >> "$T/gen.log" 2>&1
for s in pp lex parse; do b python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl"; done
b "$T/run" "$T/pp.tbl" "$SELF" "$SELF" "$R/include" > "$T/pp"
b "$T/run" "$T/lex.tbl" "$T/pp" "$SELF" > "$T/lex"
b "$T/run" "$T/parse.tbl" "$T/lex" "$SELF" > "$T/tape"
b "$UA" "$SELF" -S -o "$T/ref"
[ -s "$T/tape" ] && cmp "$T/ref" "$T/tape"
echo "E3 self-source: $(wc -c < "$T/tape" | tr -d ' ') bytes equal (tables, not nets)"
