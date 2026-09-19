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
#   bootstrap   B = C = U, the self-hosting fixed point [A-23]
#   corpus      c-testsuite -- 220 programs we did not write [A-25];
#               skipped unless corpus/ is already present
set -u
PROBES="examples/*.c tests/c/*.c"
declare -a NAME LINE CODE
# The verdict comes from each suite's EXIT STATUS, not from pattern-matching
# its last line -- a summary line that happens to end differently is not a
# failure, and a suite that dies silently must not read as green.
run() {
    local n="$1"; shift
    local out; out=$("$@" 2>&1); local rc=$?
    NAME+=("$n"); LINE+=("$(printf '%s' "$out" | tail -1)"); CODE+=("$rc")
    # A swallowed failure is useless in CI -- show the suite when it fails.
    if [ "$rc" -ne 0 ]; then
        printf '\n--- %s failed ---\n%s\n--- end %s ---\n' \
            "$n" "$(printf '%s' "$out" | tail -30)" "$n"
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
# bootstrap needs the host toolchain to turn a tape back into a binary
if [ "$(uname -s)" = "Darwin" ]; then
    run bootstrap ./tests/bootstrap.sh
fi
if [ -d corpus/c-testsuite ]; then
    run corpus env FETCH=0 ./tests/corpus.sh
fi
run acc        bash -c "python3 -m unisa acc"

echo "================ summary ================"
bad=0
for i in "${!NAME[@]}"; do
    if [ "${CODE[$i]}" -eq 0 ]; then m=ok; else m=FAIL; bad=$((bad+1)); fi
    printf "  %-4s %-11s %s\n" "$m" "${NAME[$i]}" "${LINE[$i]}"
done
echo "========================================="
[ "$bad" -eq 0 ] && echo "all suites green" || echo "$bad suite(s) failing"
[ "$bad" -eq 0 ]
