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
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
perl -e 'alarm 55; exec @ARGV' python3 -m unisa emit-kernel --out "$T" >/dev/null 2>&1 || {
    echo "  FAIL emit-kernel did not run"; exit 1; }
ok=0; bad=0
for f in "$T"/*; do
    b=$(basename "$f")
    if cmp -s "$f" "kernel/$b"; then ok=$((ok+1))
    else bad=$((bad+1)); echo "  STALE kernel/$b -- run: python3 -m unisa emit-kernel"; fi
done
echo
echo "kernel  generated files up to date $ok   stale $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
