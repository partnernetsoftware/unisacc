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
#   ccrun       unisacc compiles C; the reference VM runs it [A-21]
#   selfhost    unisacc built two ways agrees with the Python front end [A-20]
#   multi       several translation units, one program -- against cc [A-30]
#   tools       real library code by other people, with its own test
#               vectors, several files each [A-31]
#   selfgap     how much C the two front ends DISAGREE about, as a ratchet:
#               the one number that was missing while the debt grew [A-29]
#   bootstrap   B = C = U, the self-hosting fixed point [A-23]
#   acc         net == gold over the FULL gold, on the CONSTRUCTED weights
#               we ship.  The SGD control arm is not here: `tests/baseline.sh`
#               runs it on purpose, because it is minutes of full-core work
#               and is not on the shipping path.
#   corpus      c-testsuite -- 220 programs we did not write [A-25];
#               skipped unless corpus/ is already present
set -u
PROBES="examples/*.c tests/c/*.c"
declare -a NAME LINE CODE
# The verdict comes from each suite's EXIT STATUS, not from pattern-matching
# its last line -- a summary line that happens to end differently is not a
# failure, and a suite that dies silently must not read as green.
# No suite may hang the run.  macOS has no `timeout`, so each one gets a
# watchdog; 137 is the kill and reads as a failure with a clear line.
LIMIT=${SUITE_LIMIT:-900}
run() {
    local n="$1"; shift
    local out rc
    out=$("$@" 2>&1 & p=$!
          ( sleep "$LIMIT"; kill -9 $p 2>/dev/null ) >/dev/null 2>&1 &
          w=$!; wait $p 2>/dev/null; rc=$?; kill $w 2>/dev/null; exit $rc)
    rc=$?
    [ "$rc" -eq 137 ] && out="$out
TIMED OUT after ${LIMIT}s"
    NAME+=("$n"); LINE+=("$(printf '%s' "$out" | tail -1)"); CODE+=("$rc")
    # A swallowed failure is useless in CI -- show the suite when it fails.
    if [ "$rc" -ne 0 ]; then
        # The tail is not enough: a per-probe suite prints one `ok` line each
        # and the ONE that failed is usually alphabetically early, so it
        # scrolls off.  CI showed `wrong 1` for three pushes without ever
        # naming the probe.  Show the lines that are not `ok` first.
        printf '\n--- %s failed ---\n' "$n"
        printf '%s\n' "$out" | grep -vE '^  ok ' | head -40
        printf '%s\n--- end %s ---\n' "$(printf '%s' "$out" | tail -20)" "$n"
    fi
}

run acceptance ./tests/acceptance.sh
run vm         ./tests/vm.sh
run difftest   ./tests/difftest.sh
run native     bash -c "./tests/native.sh $PROBES"
run crossnative bash -c "./tests/crossnative.sh $PROBES"
run fat        bash -c "./tests/fat.sh $PROBES"
run artifacts  ./tests/artifacts.sh
run ccrun      bash -c "./tests/ccrun.sh examples/*.c tests/c/a_*.c"
run selfhost   bash -c "./tests/selfhost.sh $PROBES"
run multi      ./tests/multi.sh
if [ -d corpus/crypto-algorithms ]; then
    run tools  env FETCH=0 ./tests/tools.sh
fi
run selfgap    ./tests/selfgap.sh
# bootstrap needs the host toolchain to turn a tape back into a binary
if [ "$(uname -s)" = "Darwin" ]; then
    run bootstrap ./tests/bootstrap.sh
fi
if [ -d corpus/c-testsuite ]; then
    run corpus env FETCH=0 ./tests/corpus.sh
fi
run acc        bash -c "python3 -m unisa acc"   # the SHIPPED weights

echo "================ summary ================"
bad=0
for i in "${!NAME[@]}"; do
    if [ "${CODE[$i]}" -eq 0 ]; then m=ok; else m=FAIL; bad=$((bad+1)); fi
    printf "  %-4s %-11s %s\n" "$m" "${NAME[$i]}" "${LINE[$i]}"
done
echo "========================================="
[ "$bad" -eq 0 ] && echo "all suites green" || echo "$bad suite(s) failing"
[ "$bad" -eq 0 ]
