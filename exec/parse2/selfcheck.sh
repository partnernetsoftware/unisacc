#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Real compiler source through three deltas, one generic C executor.
# The C99 seed constructor (seed/gen.c, seed/tbl.c) builds the transition tables -- the same bytes as
# exec/build/gen.py + exec/c/tbl.py (seedgen/seedparse2 gates; the product build uses this route since
# 0.0.32 B5).  Python only flattens the source (tests/sourceflat.py); run.c alone translates it.
# E3_PART (0.0.43, 69 s whole against the ~49 s queue limit, gen.py parse2 alone 40 s):
#   all        construct and run in one scratch directory (direct use)
#   construct  tables + executor into a per-tree state directory, then a content-keyed .ready proof
#   run        verifies .ready (source, inputs, tables, executor) and runs the three deltas against UA -S
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
E3_PART=${E3_PART:-all}
case $E3_PART in all|construct|run) ;; *) echo "unknown e3 selfcheck part: $E3_PART" >&2; exit 2;; esac
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
b() { "$_BOUND" 60 "$@"; }
# What the tables and the executor are built from: seed-gen reads exec/{facts,pp,lex,parse,parse2} TSVs.
inputs() {
    find exec/facts exec/pp exec/lex exec/parse exec/parse2 exec/c seed -type f ! -path '*/__pycache__/*' \
        | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256 | cut -d' ' -f1
}
if [ "$E3_PART" = all ]; then
    T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
else
    state_root=${E3_STAGE_DIR:-${GATE_STATE:-${TMPDIR:-/tmp}}}
    T="$state_root/exec-e3self-$(printf '%s' "$R" | cksum | cut -d' ' -f1)"
    mkdir -p "$T"
fi
SELF="$T/unisacc-flat.c"
if [ "$E3_PART" = run ]; then
    [ -s "$T/.ready" ] || { echo 'missing e3self construct proof (run exec-e3self-1 first)' >&2; exit 1; }
    b python3 tests/sourceflat.py "$T/unisacc-flat.current.c"
    cmp "$T/unisacc-flat.current.c" "$SELF" || { echo 'stale e3self construct source' >&2; exit 1; }
    rm "$T/unisacc-flat.current.c"
    shasum -a 256 -c "$T/.ready" >/dev/null || { echo 'stale e3self construct tables or executor' >&2; exit 1; }
    [ "$(cat "$T/inputs")" = "$(inputs)" ] || { echo 'stale e3self construct inputs' >&2; exit 1; }
else
    rm -f "$T/.ready"
    b python3 tests/sourceflat.py "$SELF"
    b cc -O2 -o "$T/run" exec/c/run.c
    b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/seed-gen"
    b cc -std=c99 -O2 -w -Iseed seed/tbl.c -o "$T/seed-tbl"
    b "$T/seed-gen" pp "$T/pp.json" > "$T/gen.log" 2>&1
    b "$T/seed-gen" lex "$T/lex.json" --typed >> "$T/gen.log" 2>&1
    b "$T/seed-gen" parse2 "$T/parse.json" >> "$T/gen.log" 2>&1
    for s in pp lex parse; do b "$T/seed-tbl" "$T/$s.json" "$T/$s.tbl"; done
    if [ "$E3_PART" = construct ]; then
        inputs > "$T/inputs"
        shasum -a 256 "$SELF" "$T/run" "$T/pp.tbl" "$T/lex.tbl" "$T/parse.tbl" "$T/inputs" \
            exec/parse2/selfcheck.sh > "$T/.ready.tmp"
        mv "$T/.ready.tmp" "$T/.ready"
        echo "E3 self-source: tables constructed by seed/gen.c ($(wc -c < "$T/parse.tbl" | tr -d ' ') byte parse table)"
        exit 0
    fi
fi
export UNISA_ATTRIBUTES="$T/attributes"
rm -f "$UNISA_ATTRIBUTES"
b "$T/run" "$T/pp.tbl" "$SELF" "$SELF" "$R/include" > "$T/pp"
b "$T/run" "$T/lex.tbl" "$T/pp" "$SELF" > "$T/lex"
b "$T/run" "$T/parse.tbl" "$T/lex" "$SELF" > "$T/tape"
b "$UA" "$SELF" -S -o "$T/ref"
[ -s "$T/tape" ] && cmp "$T/ref" "$T/tape"
echo "E3 self-source: $(wc -c < "$T/tape" | tr -d ' ') bytes equal (tables, not nets)"
