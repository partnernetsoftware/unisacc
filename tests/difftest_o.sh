#!/bin/bash
# H3: every -O level against cc -O2.  Each probe is built by the system
# compiler at -O2 (with the same shim difftest.sh uses for our __mmap
# family) and run; unisacc runs it at -O0, -O1 and -O2.  All four must print
# the same bytes and exit the same way.
#
# cc's answer is cached by hash of source + cc version (as fuzz.sh does):
# each fresh reference binary costs a 0.5-0.9 s XProtect scan on first run,
# and a run may not take more than 60 s (AGENTS.md).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
CC=${CC:-cc}
T=$(scratch)
CACHE=${DIFFO_CACHE:-${TMPDIR:-/tmp}/unisacc-diffo-want}; mkdir -p "$CACHE"
# Freeze the input list once; execution and verdicts use exactly this shard.
SHARD=${SHARD:-1/1}
[[ "$SHARD" =~ ^[1-9][0-9]{0,5}/[1-9][0-9]{0,5}$ ]] || { echo 'invalid SHARD=k/n' >&2; exit 2; }
SH_K=${SHARD%/*}; SH_N=${SHARD#*/}
[ "$SH_K" -le "$SH_N" ] || { echo 'invalid SHARD: k > n' >&2; exit 2; }
FILES=(); i=0
for f in tests/c/*.c examples/*.c; do
    [ "$(basename "$f" .c)" = host ] && continue
    # 0.0.39 WF1: PROBES (space-separated paths) narrows the run to named probes only when WF1_PRECHECK=1 is
    # also set, so a PROBES inherited from the environment can never shrink a formal gate; numbering unchanged
    if [ $((i % SH_N)) -eq $((SH_K - 1)) ] && { [ "${WF1_PRECHECK:-}" != 1 ] || [[ " ${PROBES:-} " == *" $f "* ]]; }; then FILES+=("$f"); fi
    i=$((i + 1))
done
[ "${#FILES[@]}" -gt 0 ] || { echo "empty SHARD=$SHARD" >&2; exit 2; }
ua_ready
bound 10 "$CC" --version > "$T/cc-version" 2>&1 || { echo 'FAIL cc version'; exit 1; }
CCV=$(head -1 "$T/cc-version")
# Preserve normal exits 0..255, but reject signals and watchdog timeouts.
# The raw wait status avoids confusing e.g. exit(142) with SIGALRM.
run() { # output, error, command...
    local out=$1 err=$2 status; shift 2
    "$_BOUND_HELPER" --status "$out.status" 10 "$@" > "$out" 2> "$err" </dev/null
    local watchdog_rc=$?
    # Nonzero normal exits are valid outputs; timeout/signal statuses are not.
    status=$(cat "$out.status" 2>/dev/null)
    [[ "$status" =~ ^[0-9]+$ ]] && [ $((status & 127)) -eq 0 ] || return 1
    [ "$watchdog_rc" -eq $((status >> 8)) ] || return 1
    echo "rc=$((status >> 8))" >> "$out"
}
PAR_WAIT=4
. "$R/tests/par.sh"
for f in "${FILES[@]}"; do
    b=$(basename "$f" .c)
    [ "$b" = "host" ] && continue
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    { echo '#define _DEFAULT_SOURCE'   # glibc hides struct sigaction etc. under -std=c99 (0.0.35 Linux red)
      echo '#include <stdio.h>'; echo '#include <string.h>'
      echo '#include <stdlib.h>'; echo '#include <sys/mman.h>'
      echo 'static long unisa_mmap_(long a,long n,long p,long f,long d,long o){return (long)mmap((void *)a,(size_t)n,(int)p,(int)f,(int)d,(off_t)o);}'
      echo '#define __mmap(...) unisa_mmap_(__VA_ARGS__)'
      echo '#define __mprotect(a,n,p) mprotect((void *)(long)(a),(n),(p))'
      echo '#define __munmap(a,n) munmap((void *)(long)(a),(n))'
      # #line: the shim above shifts every physical line, and __LINE__ /
      # __FILE__ (N17) must see the probe as unisacc does -- its own
      # numbering and the path it was handed (the same "$f" both sides get)
      printf '#line 1 "%s"\n' "$f"
      cat "$f"; } > "$D/ref.c"
    key="$CACHE/$( (cat "$D/ref.c"; cat "$(dirname "$R/$f")"/*.h 2>/dev/null; echo "wait-status-v2 $CCV -O2") | shasum | cut -c1-40)"
    if [ -f "$key" ]; then cp "$key" "$D/want"
    else
        if ! bound 20 "$CC" -w -std=c99 -O2 -I "$(dirname "$R/$f")" -o "$D/ref" "$D/ref.c" -lm > "$D/cc.out" 2> "$D/cc.err" || [ ! -s "$D/ref" ]; then
            echo 'reference compile failed' > "$D/v"; exit 0; fi
        if ! (cd "$D" && run "$D/want" "$D/ref.err" ./ref); then
            echo 'reference run signaled/timed out' > "$D/v"; exit 0; fi
        cp "$D/want" "$key.$$" && mv "$key.$$" "$key" || { echo 'cache write failed' > "$D/v"; exit 0; }
    fi
    for o in -O0 -O1 -O2; do
        if ! (cd "$D" && run "$D/got$o" "$D/err$o" "$UA" "$o" -run "$R/$f"); then   # -run before the file: see difftest.sh
            echo 'run signaled/timed out' > "$D/fail$o"
        fi
    done
    echo done > "$D/v"
    ) &
done
wait
ok=0; bad=0; refuse=0; known=0; revived=0
# The same knownfail contract as difftest.sh, chosen by driver kind: a listed
# probe may be WRONG (counted as known), a listed probe that agrees at every
# level is REVIVED and fails the run (delete its line).  Before 2026-09-30 this
# suite had no list at all, so the gate's difftest_o-N were red for every open
# R13 defect the reference still has.
case "$UA" in *.com) KNOWN=$R/tests/difftest.com.knownfail;; *) KNOWN=$R/tests/difftest.knownfail;; esac
. "$R/tests/knownfail.sh"
knownfail_load "$KNOWN" "$T/known.keys" || exit 1
isknown() { knownfail_has "$1"; }
for f in "${FILES[@]}"; do
    b=$(basename "$f" .c); D="$T/$b.d"
    [ "$b" = "host" ] && continue
    if isknown "$b"; then
        kw=0; for o in -O0 -O1 -O2; do cmp -s "$D/want" "$D/got$o" || kw=1; done
        if [ "$kw" = 0 ]; then revived=$((revived+1)); printf "       %-14s is listed in %s but agrees at every level: delete its line\n" "$b" "$(basename "$KNOWN")"
        else known=$((known+1)); echo "  known $b"; fi
        continue
    fi
    case "$(cat "$D/v" 2>/dev/null)" in
    done) ;;
    *) bad=$((bad+1)); echo "  FAIL $b: $(cat "$D/v" 2>/dev/null || echo no-verdict)"; continue;;
    esac
    for o in -O0 -O1 -O2; do
        if [ -f "$D/fail$o" ]; then
            bad=$((bad+1)); echo "  FAIL $b $o: $(cat "$D/fail$o")"
        elif cmp -s "$D/want" "$D/got$o"; then ok=$((ok+1))
        elif [ "$(tail -1 "$D/got$o")" != rc=0 ] && grep -q 'not covered:' "$D/err$o"; then
            refuse=$((refuse+1)); echo "  REFUSE $b $o: $(head -1 "$D/err$o")"
        else bad=$((bad+1)); printf "  WRONG %-14s %s: got [%s] cc -O2 says [%s]\n" "$b" "$o" \
            "$(tr '\n' ' ' < "$D/got$o" | cut -c1-40)" "$(tr '\n' ' ' < "$D/want" | cut -c1-40)"; fi
    done
done
echo
echo "difftest_o SHARD=$SHARD probes=${#FILES[@]}  agree $ok   wrong $bad   refuse $refuse   known $known   revived $revived   (-O0 -O1 -O2 against cc -O2)"
[ "$bad" -eq 0 ] && [ "$refuse" -eq 0 ] && [ "$revived" -eq 0 ] && [ "$ok" -gt 0 ]
