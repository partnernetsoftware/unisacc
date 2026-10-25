#!/bin/sh
# Watchdog regression (c43 scope-token fix).  usage: sh tests/watchdogregression.sh orphan|eof|success BOUND
#   BOUND: tests/bound.py (python) or tests/bound (C wrapper); paths relative to the repository root.
# orphan:  a nested bound child whose middle shell exited first (reparented, own session) must be gone
#          shortly after the outer bound times out.
# eof:     a caller reading the bound's stdout through a pipe must see EOF within 8 s; an escaped child
#          must not keep the pipe open.
# success: the outer command exits 0 but leaves an escaped nested child; the bound must clean it up anyway.
# Portable: no GNU timeout (macOS has none).  Each case runs in the background with a sleep-and-kill timer.
# Status: on macOS the scope-token branch of tests/bound.py is NOT verified on Darwin yet (m4pro pending);
#         a green run on macOS is not evidence until that check is recorded.
set -u
mode=${1:-}; B=${2:-}
case "$mode" in orphan|eof|success) ;; *) echo "usage: watchdogregression.sh orphan|eof|success BOUND"; exit 2;; esac
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
[ -f "$B" ] || { echo "no watchdog at $B"; exit 2; }
case "$B" in *.py) RUNB="python3 $B";; *) RUNB="$B";; esac
export RUNB
T=$(mktemp -d "${TMPDIR:-/tmp}/watchdogreg.XXXXXX") || exit 2
export T
# a marker sleep per run: derived from this run's temporary directory, so a leftover from another run never matches
d=$(( 100 + $(printf '%s' "$T" | cksum | cut -d' ' -f1) % 800 ))
export d
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
# limit SECONDS SCRIPT: run the case in the background and kill it if it overruns (no coreutils timeout)
limit() {
    sh "$2" &
    p=$!
    ( sleep "$1"; kill -9 "$p" 2>/dev/null ) &
    k=$!
    wait "$p"; rc=$?
    kill "$k" 2>/dev/null; wait "$k" 2>/dev/null
    return $rc
}
case $mode in
orphan)
    printf '%s\n' '$RUNB 3 sh -c "($RUNB 60 sleep $d &); sleep 100"' > "$T/case.sh"
    limit 30 "$T/case.sh"; rc=$?
    sleep 2
    if ps -eo args | grep -q "^sleep $d\$"; then echo "orphan FAIL: orphaned child survives the outer bound (rc=$rc)"; exit 1; fi
    echo "orphan ok rc=$rc"; exit 0 ;;
success)
    # the command keeps running 1 s so the nested child has started its sleep before the normal exit
    printf '%s\n' '$RUNB 10 sh -c "($RUNB 60 sleep $d &); sleep 1; exit 0"' > "$T/case.sh"
    limit 30 "$T/case.sh"; rc=$?
    sleep 1
    if ps -eo args | grep -q "^sleep $d\$"; then echo "success FAIL: escaped child survives a normal exit (rc=$rc)"; exit 1; fi
    echo "success ok rc=$rc"; exit 0 ;;
eof)
    printf '%s\n' 'st=$(date +%s); $RUNB 3 sh -c "($RUNB 60 sleep $d &); sleep 100" | cat > "$T/out"; echo $(( $(date +%s) - st )) > "$T/w"' > "$T/case.sh"
    limit 30 "$T/case.sh"
    w=$(cat "$T/w" 2>/dev/null || echo 99)
    if [ "$w" -gt 8 ]; then echo "eof FAIL: the pipe reader waited ${w}s (EOF held by an escaped child)"; exit 1; fi
    echo "eof ok ${w}s"; exit 0 ;;
esac
