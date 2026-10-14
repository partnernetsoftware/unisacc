#!/bin/sh
# Watchdog regression (c43 scope-token fix).  usage: sh tests/watchdogregression.sh orphan|eof BOUND
#   BOUND: tests/bound.py (python) or tests/bound (C wrapper); paths relative to the repository root.
# orphan: a nested bound child whose middle shell exited first (reparented, own session) must be gone
#         shortly after the outer bound times out.
# eof:    a caller reading the bound's stdout through a pipe must see EOF within 8 s; an escaped child
#         must not keep the pipe open.
# Status: on macOS the scope-token branch of tests/bound.py is NOT verified on Darwin yet (m4pro pending);
#         a green run on macOS is not evidence until that check is recorded.
set -u
mode=${1:-}; B=${2:-}
case "$mode" in orphan|eof) ;; *) echo "usage: watchdogregression.sh orphan|eof BOUND"; exit 2;; esac
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
[ -f "$B" ] || { echo "no watchdog at $B"; exit 2; }
case "$B" in *.py) RUNB="python3 $B";; *) RUNB="$B";; esac
if [ "$mode" = orphan ]; then
    # the sleep length 37 is the marker: no other process in the gate uses it
    timeout 30 $RUNB 3 sh -c "($RUNB 60 sleep 37 &); sleep 100"; rc=$?
    sleep 2
    if ps -eo args | grep -q '^sleep 37$'; then echo "orphan FAIL: orphaned child survives the outer bound (rc=$rc)"; exit 1; fi
    echo "orphan ok rc=$rc"; exit 0
fi
T=$(mktemp -d "${TMPDIR:-/tmp}/watchdogeof.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
st=$(date +%s)
timeout 30 $RUNB 3 sh -c "($RUNB 60 sleep 38 &); sleep 100" | cat > "$T/out"
w=$(( $(date +%s) - st ))
if [ "$w" -gt 8 ]; then echo "eof FAIL: the pipe reader waited ${w}s (EOF held by an escaped child)"; exit 1; fi
echo "eof ok ${w}s"; exit 0
