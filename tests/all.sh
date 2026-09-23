#!/bin/bash
# Every suite, one summary.  Run from the repo root.
#
#   acceptance  the spec's own checklist [A-*]
#   vm          the tape interpreter on hand-written fixtures [TP-4]
#   difftest    unisa vs the system compiler -- the only instrument that can
#               see a gold defect [P-6] [A-17]
#   native      emitted images actually executed on this host [A-18]
#   fat         one file, both macOS architectures, both executed [A-28]
#   crossnative the OTHER targets, executed in local Linux VMs [A-27];
#               skipped when limactl or the VMs are absent
#   artifacts   the shipped kit is usable, not merely well-formed [A-24]
#   ccrun       unisacc compiles C and the answers are compared against
#               the Python front end's, on EVERY probe [A-21].  Being
#               refused is a gap; disagreeing is a defect.
#   selfhost    unisacc built two ways agrees with the Python front end [A-20]
#   multi       several translation units, one program -- against cc [A-30]
#   tools       real library code by other people, with its own test
#               vectors, several files each [A-31]
#   stages      every stage the Python front end asks on a probe, the C
#               front end asks too: no table-shaped decision in code [A-33]
#   selfgap     how much C the two front ends DISAGREE about, as a ratchet:
#               the one number that was missing while the debt grew [A-29]
#   nativeboot  unisacc -b builds unisacc, which rebuilds itself to the same
#               bytes -- the bootstrap with no Python in it [S-6]
#   run         `unisacc -run` compiles a file and RUNS it in memory (no
#               image, no temp file): same stdout and exit as the cc [S-9]
#   cli         the compiler as a TOOL, from a scratch dir: built-in headers,
#               -I, -D, a shebang, argv, exit status [S-11]
#   diag        what the compiler says when the program is wrong: the user's
#               file, the user's line, the column, the line itself [S-12]
#   ape         unisacc.com: one file that is a PE for Windows and a shell
#               script for Unix, with a slice per target inside [S-10]
#   closure     unisacc -b writes the same image bytes as the Python back
#               end, all six targets, and the host image runs right [S-7]
#   layout      the data layout ENUMERATED, not sampled: every tape of up to
#               three data definitions, with and without a symbol defined
#               twice, both back ends, all six targets [A-41] [P-3]
#   datashape   generated programs whose globals are declared in one order
#               and allocated in another -- shapes the corpus never asks
#               for, because it grew from things people wanted [A-41]
#   bigclosure  the closure on the compiler itself: 707 KB and 1874 data
#               symbols, the only input ever big enough to break the back
#               end while all 90 probes stayed green [A-42]
#   bootstrap   B = C = U, the self-hosting fixed point [A-23]
#   acc         net == gold over the FULL gold, on the CONSTRUCTED weights
#               we ship.  The SGD control arm is not here: `tests/baseline.sh`
#               runs it on purpose, because it is minutes of full-core work
#               and is not on the shipping path.
#   corpus      c-testsuite -- 220 programs we did not write [A-25];
#               skipped unless corpus/ is already present
set -u
PROBES="examples/*.c tests/c/*.c"
# The verdict comes from each suite's EXIT STATUS, not from pattern-matching
# its last line -- a summary line that happens to end differently is not a
# failure, and a suite that dies silently must not read as green.
# No suite may hang the run.  macOS has no `timeout`, so each one gets a
# watchdog; 137 is the kill and reads as a failure with a clear line.
LIMIT=${SUITE_LIMIT:-900}
# Suites run CONCURRENTLY, JOBS at a time.  Each one writes its output to a
# file of its own; the summary is printed afterwards in the fixed order below.
# The default leaves cores free: this machine has overheated under full load.
JOBS=${JOBS:-6}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
ORDER=()
run() {
    local n="$1"; shift
    ORDER+=("$n")
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 0.5; done
    (
        t0=$(date +%s)
        "$@" > "$W/$n.out" 2>&1 & p=$!
        ( sleep "$LIMIT"; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 &
        w=$!; wait $p 2>/dev/null; rc=$?; kill $w 2>/dev/null
        [ "$rc" -eq 137 ] && echo "TIMED OUT after ${LIMIT}s" >> "$W/$n.out"
        echo "$rc" > "$W/$n.rc"; echo $(( $(date +%s) - t0 )) > "$W/$n.sec"
    ) &
}
# Serial, and before anything else: a failing one says why in a known place.
serial() { run "$@"; wait; }
T0=$(date +%s)

# One build of the self-hosted compiler for every suite that uses it.  ccrun
# never built it and ran BEFORE selfhost, so it could test a stale binary.
./tests/build_ref.sh >/dev/null || { echo "build_ref failed"; exit 1; }

# acceptance rewrites weights/, which every other suite reads: it goes alone.
serial acceptance ./tests/acceptance.sh
run vm         ./tests/vm.sh
run difftest   ./tests/difftest.sh
run native     bash -c "./tests/native.sh $PROBES"
run crossnative bash -c "./tests/crossnative.sh $PROBES"
run fat        bash -c "./tests/fat.sh $PROBES"
run artifacts  ./tests/artifacts.sh
run ccrun      bash -c "./tests/ccrun.sh $PROBES"
run selfhost   bash -c "./tests/selfhost.sh $PROBES"
run closure    bash -c "./tests/closure.sh $PROBES"
run run        ./tests/run.sh
run layout     ./tests/layout.sh
run datashape  ./tests/datashape.sh
run bigclosure ./tests/bigclosure.sh
run cli        ./tests/cli.sh
run diag       ./tests/diag.sh
run ape        ./tests/ape.sh
run multi      ./tests/multi.sh
if [ -d corpus/crypto-algorithms ]; then
    run tools  env FETCH=0 ./tests/tools.sh
fi
run selfgap    ./tests/selfgap.sh
run stages     bash -c "./tests/stages.sh $PROBES"
# bootstrap needs the host toolchain to turn a tape back into a binary
if [ "$(uname -s)" = "Darwin" ]; then
    run bootstrap ./tests/bootstrap.sh
fi
run nativeboot ./tests/nativeboot.sh
if [ -d corpus/c-testsuite ]; then
    run corpus env FETCH=0 ./tests/corpus.sh
fi
run acc        bash -c "python3 -m unisa acc"   # the SHIPPED weights
wait

bad=0
for n in "${ORDER[@]}"; do
    rc=$(cat "$W/$n.rc" 2>/dev/null || echo 1)
    [ "$rc" -eq 0 ] && continue
    # A swallowed failure is useless -- show the suite when it fails.  The
    # tail is not enough: a per-probe suite prints one `ok` line each and the
    # ONE that failed is usually alphabetically early, so it scrolls off.
    printf '\n--- %s failed ---\n' "$n"
    grep -vE '^  ok ' "$W/$n.out" | head -40
    printf '%s\n--- end %s ---\n' "$(tail -20 "$W/$n.out")" "$n"
done
echo "================ summary ================"
for n in "${ORDER[@]}"; do
    rc=$(cat "$W/$n.rc" 2>/dev/null || echo 1)
    if [ "$rc" -eq 0 ]; then m=ok; else m=FAIL; bad=$((bad+1)); fi
    printf "  %-4s %-11s %4ss  %s\n" "$m" "$n" "$(cat "$W/$n.sec" 2>/dev/null)" \
        "$(tail -1 "$W/$n.out" 2>/dev/null)"
done
echo "========================================="
echo "wall $(( $(date +%s) - T0 ))s   (JOBS=$JOBS)"
[ "$bad" -eq 0 ] && echo "all suites green" || echo "$bad suite(s) failing"
[ "$bad" -eq 0 ]
