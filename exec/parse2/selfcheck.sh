#!/bin/sh
# Real compiler source through three deltas, one generic C executor.
# Python generates transition tables; it does not process the source at runtime.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/pp/gen.py "$T/pp.json" > "$T/gen.log" 2>&1
b python3 exec/lex/gen.py --typed "$T/lex.json" >> "$T/gen.log" 2>&1
b python3 exec/parse2/gen2.py "$T/parse.json" >> "$T/gen.log" 2>&1
for s in pp lex parse; do b python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl"; done
b "$T/run" "$T/pp.tbl" unisacc.c unisacc.c "$R/include" > "$T/pp"
b "$T/run" "$T/lex.tbl" "$T/pp" unisacc.c > "$T/lex"
b "$T/run" "$T/parse.tbl" "$T/lex" unisacc.c > "$T/tape"
b "$UA" unisacc.c -S -o "$T/ref"
[ -s "$T/tape" ] && cmp "$T/ref" "$T/tape"
echo "E3 self-source: $(wc -c < "$T/tape" | tr -d ' ') bytes equal (tables, not nets)"
