#!/bin/sh
# c43 watchdog regression (DRAFT): run against a bound.py path.  usage: research/c43-watchdog-regression.sh BOUND_PY
# R1: an orphaned nested bound child (its middle shell exits first, the child runs in its own session) must be
#     gone shortly after the outer bound times out.  R2: a caller reading the bound's stdout through a pipe must
#     see EOF promptly, i.e. no escaped descendant still holds the pipe.  Both bounds: 3 s outer, <= 8 s total.
B=$1; [ -f "$B" ] || { echo "no bound.py at $B"; exit 2; }
T=$(mktemp -d "${TMPDIR:-/tmp}/c43wd.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail=0
# R1: the marker is the unique sleep length 37, so no other process can match it
st=$(date +%s)
timeout 30 python3 "$B" 3 sh -c "(python3 '$B' 60 sleep 37 &); sleep 100"; rc=$?
sleep 2
if ps -eo args | grep -q '^sleep 37$'; then echo "R1 FAIL: orphaned child survives the outer bound (rc=$rc)"; fail=1; else echo "R1 ok rc=$rc"; fi
# R2: the reader must get EOF within 8 s of the start
st=$(date +%s)
timeout 30 python3 "$B" 3 sh -c "(python3 '$B' 60 sleep 38 &); sleep 100" | cat > "$T/out"
w=$(( $(date +%s) - st ))
if [ "$w" -gt 8 ]; then echo "R2 FAIL: the pipe reader waited ${w}s (EOF held by an escaped child)"; fail=1; else echo "R2 ok ${w}s"; fi
ps -eo args | grep -q '^sleep 38$' && { echo "R2 note: sleep 38 still alive"; }
exit $fail
