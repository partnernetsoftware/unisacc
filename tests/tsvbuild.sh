#!/bin/bash
# The gold tables as data are enough to build the weights. [J10]
#
# weights/gold/*.tsv carry every stage's truth table AND its schema (field
# values and head classes, in order).  Constructing all stages from those
# files alone -- gold.py's rules never consulted -- must give the shipped
# weights/built.uns2 and the weights cache byte for byte.  This is the
# precondition for moving construction out of the Python seed: whatever
# builds the weights next only has to read these files.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
out=$(bound 58 python3 -m unisa build-weights --from-tsv --check 2>&1); rc=$?
printf '%s\n' "$out" | sed 's/^/  /'
if [ $rc -eq 0 ] && printf '%s\n' "$out" | grep -q "uns2 .* IDENTICAL"; then
    echo "tsvbuild  weights from tsv identical to shipped"
    exit 0
fi
echo "tsvbuild  FAIL (rc $rc)"
exit 1
