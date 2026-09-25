#!/bin/bash
# kernel/*.inc and kernel/*.c are GENERATED -- by `python3 -m unisa
# emit-kernel`, from weights/built.* (constructed from the gold tables),
# the vocabularies in unisa/gold.py and unisa/catalog.py, and include/*.h.
# They are committed because unisacc.c is built from them by concatenation,
# with no Python in the loop.  This suite regenerates them and requires the
# committed copies to be identical: an edit to include/ that nobody
# re-emitted made the shipped compiler carry the OLD stdlib.h (getenv was
# "undefined" outside the repo, where the embedded copy is what is read).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
T=$(scratch)
bound 55 python3 -m unisa emit-kernel --out "$T" >/dev/null 2>&1 || {
    echo "  FAIL emit-kernel did not run"; exit 1; }
ok=0; bad=0
for f in "$T"/*; do
    b=$(basename "$f")
    if cmp -s "$f" "kernel/$b"; then ok=$((ok+1))
    else bad=$((bad+1)); echo "  STALE kernel/$b -- run: python3 -m unisa emit-kernel"; fi
done
# ...and the truth tables as data (weights/gold/*.tsv), from the same gold
bound 55 python3 -m unisa gold-export --out "$T/gold" >/dev/null 2>&1 || {
    echo "  FAIL gold-export did not run"; exit 1; }
for f in "$T"/gold/*.tsv; do
    b=$(basename "$f")
    if cmp -s "$f" "weights/gold/$b"; then ok=$((ok+1))
    else bad=$((bad+1)); echo "  STALE weights/gold/$b -- run: python3 -m unisa gold-export"; fi
done
echo
echo "kernel  generated files up to date $ok   stale $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
