#!/bin/bash
# What the compiler costs, as a ratchet. [A-45]
#
# Compile speed was never measured, so it was free to rot: one lookup that
# walked a list from the front, called once per token, put 57% of the
# self-compile inside strlen and nobody knew.  Fixing it took the
# self-compile from 2.85s to 0.46s.  Nothing would have noticed it coming
# back.
#
# Three numbers, each with a reason to exist:
#
#   self     the reference build (cc -O2) compiling unisacc.c -- 707 KB and
#            1,874 data symbols, the largest real program we have
#   probe    a small file, where startup and model setup dominate
#   ratio    the SHIPPED binary (compiled by unisacc, which has no
#            optimiser) against the same source built by cc -O2.  This is
#            the price of emitting unoptimised code, and it is reported
#            rather than fixed: an optimiser is a scope decision, not a
#            debt [S-10 #11].
#
# The ratchet direction is DOWN for times and for the ratio.  Machines
# differ, so the baseline is per-machine (tests/bench.baseline.$(uname -m))
# and a run that is within TOLERANCE of it passes -- this measures a
# regression in the compiler, not the weather.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
TOL=${TOLERANCE:-30}          # percent slower than baseline that still passes
# One machine's numbers are not another's.  Keyed by `uname -m` alone,
# GitHub's arm64 macOS runners -- six times slower than the laptop that
# recorded it -- were held to the laptop's baseline and failed every push.
# A machine with no baseline of its own records one and passes.
BASE=$R/tests/bench.baseline.$(uname -m).$(hostname -s | tr -c 'A-Za-z0-9\n' '_')

# The reference the numbers are about is an OPTIMISED build: the shipped
# binary's own speed is the `ratio` line, further down.
cc -w -std=c99 -O2 -o "$T/opt" "${UA}.c" 2>/dev/null || {
    echo "  skip (no ${UA}.c to build an optimised reference from)"; exit 0; }

ms() {   # ms <cmd...> -- best of three, in milliseconds
    local best=999999 i t0 t1
    # No label argument: an earlier version took one and then ran "$@"
    # WITH it, so every measurement was "command not found" in 15 ms and
    # the suite cheerfully recorded that as the baseline.
    for i in 1 2 3; do
        t0=$(python3 -c 'import time;print(int(time.time()*1000))')
        "$@" >/dev/null 2>&1
        t1=$(python3 -c 'import time;print(int(time.time()*1000))')
        [ $((t1 - t0)) -lt "$best" ] && best=$((t1 - t0))
    done
    echo "$best"
}

self=$(ms "$T/opt" unisacc.c -b osx/arm64 -o "$T/a.bin")
[ -f "$T/a.bin" ] || self=$(ms "$T/opt" unisacc.c -b lnx/x86_64 -o "$T/a.bin")
probe=$(ms "$T/opt" examples/fib.c -b lnx/x86_64 -o "$T/b.bin")

# the shipped binary: built by unisacc itself, no optimiser anywhere
host=$(host_target)
ratio=0
if [ -n "$host" ]; then
    # built as make com builds it: -O2 [H1]
    if bound 60 "$T/opt" -O2 unisacc.c -b "$host" -o "$T/self.bin" 2>/dev/null; then
        chmod +x "$T/self.bin"
        command -v codesign >/dev/null && codesign -f -s - "$T/self.bin" >/dev/null 2>&1
        shipped=$(ms "$T/self.bin" unisacc.c -b "$host" -o "$T/c.bin")
        [ "$self" -gt 0 ] && ratio=$((shipped * 10 / self))
        printf "  %-26s %6d ms  (cc -O2 built: %d ms -> %d.%dx)\n" \
            "shipped binary on itself" "$shipped" "$self" \
            $((ratio / 10)) $((ratio % 10))
    fi
fi
printf "  %-26s %6d ms\n" "self-compile (707 KB)" "$self"
printf "  %-26s %6d ms\n" "one small probe" "$probe"

# A compile that took no time did not happen.  The first version of this
# suite measured "command not found" and recorded 15ms as the baseline.
if [ "$self" -lt 50 ] || [ "$probe" -lt 1 ]; then
    echo "  FAIL nothing was measured (self ${self}ms) -- did the command run?"
    exit 1
fi

rc=0
if [ -f "$BASE" ]; then
    read -r b_self b_probe < "$BASE"
    for pair in "self:$self:$b_self" "probe:$probe:$b_probe"; do
        n=${pair%%:*}; rest=${pair#*:}; got=${rest%%:*}; want=${rest#*:}
        [ -z "$want" ] && continue
        lim=$((want + want * TOL / 100 + 20))
        if [ "$got" -gt "$lim" ]; then
            echo "  REGRESSION $n: ${got}ms > baseline ${want}ms + ${TOL}%"
            rc=1
        fi
    done
    if [ "$rc" = 0 ] && [ "$self" -lt "$b_self" ]; then
        echo "  faster than baseline (${b_self}ms -> ${self}ms); RATCHET=1 records it"
        [ "${RATCHET:-0}" = "1" ] && { echo "$self $probe" > "$BASE"; echo "  recorded."; }
    fi
else
    echo "$self $probe" > "$BASE"
    echo "  baseline recorded for $(uname -m): self ${self}ms  probe ${probe}ms"
fi

echo
echo "bench  self ${self}ms   probe ${probe}ms   shipped/cc-O2 $((ratio / 10)).$((ratio % 10))x"
[ "$rc" -eq 0 ]
