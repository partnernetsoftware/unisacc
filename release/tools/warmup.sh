#!/bin/bash
# warmup.sh STATE ROOT -- one cache warm-up step per call, outside the queue (0.0.38: from tests/release.sh).
# Exit 75 after running (or failing) one step, 0 when every step is settled.  A marker means the cache
# was built; a step that does not apply to this host is .na and costs no window; a failed step writes
# no marker -- it is counted (.fail), and after two failures it is settled COLD (.cold), named, never
# as if the cache existed.  Markers are per checkout (ROOT): a new worktree warms again (0.0.21).
# STAGELOG_RUN/STAGELOG_PARENT add a paired warmup-N event to the private stage log.
set -u
S=${1:?state dir}; R=${2:?checkout}
host=${WARMUP_HOST:-$(uname -s)}   # WARMUP_HOST only for tests/warmupcheck.sh
sl() { [ -n "${STAGELOG_RUN:-}" ] && [ "$STAGELOG_RUN" != 0 ] || return 0
       python3 "$R/release/tools/stagelog.py" "$@" 2>/dev/null || :; }
wk=$(printf '%s' "$R" | cksum | cut -d' ' -f1)
for w in 1 2 3; do
    m="$S/warm.$w.$wk"
    { [ -f "$m" ] || [ -f "$m.na" ] || [ -f "$m.cold" ]; } && continue
    case $w in 1|2) [ "$host" = Darwin ] || { : > "$m.na"; echo "warm-up $w/3 not applicable on $host"; continue; };; esac
    sid=$(sl begin --run "${STAGELOG_RUN:-0}" --phase queue --subphase "warmup-$w" ${STAGELOG_PARENT:+--parent-id "$STAGELOG_PARENT"})
    wrc=0
    case $w in
        1) python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=arm64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1 || wrc=$?;;
        2) python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=x86_64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1 || wrc=$?;;
        3) python3 "$R/tests/bound.py" 50 "$R/exec/c/warningcheck.sh" ua Wall >/dev/null 2>&1 || wrc=$?;;
    esac
    [ -z "$sid" ] || sl end --id "$sid" --rc "$wrc" --execution-status warmup
    if [ "$wrc" = 0 ]; then
        : > "$m"; echo "warm-up $w/3 done in $R (cold model caches built outside the queue)"
    else
        n=$(( $(cat "$m.fail" 2>/dev/null || echo 0) + 1 )); echo "$n" > "$m.fail"
        if [ "$n" -ge 2 ]; then : > "$m.cold"; echo "warm-up $w/3 FAILED twice (rc=$wrc): continuing COLD -- its suites may time out"
        else echo "warm-up $w/3 failed (rc=$wrc, attempt $n): no cache marker; retried next window"; fi
    fi
    exit 75
done
exit 0
