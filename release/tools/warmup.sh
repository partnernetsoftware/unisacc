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
# step 3's cache has a real identity: exec/pipeline/models.py's key (checkout, target, network, compiler,
# full construction closure) and valid() of its manifest.  Its marker is that key, and "settled" means the
# cache is valid now -- not a file left from another input state, not a deleted or damaged cache (cdx2 10-10)
case ${WARMUP_TARGET:-$(uname -s)/$(uname -m)} in Darwin/arm64|osx/arm64) wtg=osx/arm64;; Darwin/x86_64|osx/x86_64) wtg=osx/x86_64;;
     Linux/aarch64|Linux/arm64|lnx/arm64) wtg=lnx/arm64;; Linux/x86_64|lnx/x86_64) wtg=lnx/x86_64;; *) wtg=;; esac
mk() { [ -n "$wtg" ] && "${WARMUP_MODELKEY:-$R/release/tools/modelkey.py}" "$R" "$wtg" 2>/dev/null; }
for w in 1 2 3; do
    m="$S/warm.$w.$wk"
    if [ "$w" = 3 ]; then
        set -- $(mk); key=${1:-unknown}; state=${2:-invalid}; m="$S/warm.3.$key"
        [ "$state" = valid ] && { [ -f "$m" ] || { : > "$m"; echo "warm-up 3/3 cache already valid (key ${key%"${key#????????????}"})"; }; continue; }
        [ -f "$m.cold" ] && continue
    else
        { [ -f "$m" ] || [ -f "$m.na" ] || [ -f "$m.cold" ]; } && continue
    fi
    case $w in 1|2) [ "$host" = Darwin ] || { : > "$m.na"; echo "warm-up $w/3 not applicable on $host"; continue; };; esac
    sid=$(sl begin --run "${STAGELOG_RUN:-0}" --phase queue --subphase "warmup-$w" ${STAGELOG_PARENT:+--parent-id "$STAGELOG_PARENT"})
    wrc=0
    case $w in
        1) python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=arm64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1 || wrc=$?;;
        2) python3 "$R/tests/bound.py" 50 env CORE_ASM_ARCH=x86_64 "$R/exec/c/asm/bindprep.sh" >/dev/null 2>&1 || wrc=$?;;
        # 0.0.38: the cache the warning drivers need is the host model preparation; warningcheck.sh as a
        # whole runs 48 s even warm on the cloud host, so it can never settle inside 50 s (0.0.37's
        # "3/3 done" was written after a failure).  Build only the cache, in a scratch dir.
        3) wt=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-warm.XXXXXX")
           if [ -n "$wtg" ]; then (cd "$R" && TARGET=$wtg python3 "$R/tests/bound.py" 50 ./exec/pipeline/elf.sh "$wt" examples/hello.c >/dev/null 2>&1) || wrc=$?
           else wrc=2; fi
           rm -rf "$wt";;
    esac
    [ -z "$sid" ] || sl end --id "$sid" --rc "$wrc" --execution-status warmup
    # step 3 counts as built only if the cache under the real key is valid afterwards
    if [ "$w" = 3 ] && [ "$wrc" = 0 ]; then set -- $(mk); [ "${2:-invalid}" = valid ] || wrc=3; m="$S/warm.3.${1:-$key}"; fi
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
