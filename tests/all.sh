#!/bin/bash
# Every suite, one summary.  Run from the repo root.
#
#   acceptance  the spec's own checklist [A-*]
#   vm          the tape interpreter on hand-written fixtures [TP-4]
#   difftest    unisa vs the system compiler -- the only instrument that can
#               see a gold defect [P-6] [A-17]
#   native      emitted images actually executed on this host [A-18]
#   artifacts   the shipped kit is usable, not merely well-formed [A-24]
#   ccrun       unisacc compiles C; the reference VM runs it [A-21]
#   selfhost    unisacc built two ways agrees with the Python front end [A-20]
#   bootstrap   B = C = U, the self-hosting fixed point [A-23]
set -u
CORPUS="examples/*.c tests/c/*.c"
declare -a NAME LINE CODE
# The verdict comes from each suite's EXIT STATUS, not from pattern-matching
# its last line -- a summary line that happens to end differently is not a
# failure, and a suite that dies silently must not read as green.
run() {
    local n="$1"; shift
    local out; out=$("$@" 2>/dev/null); local rc=$?
    NAME+=("$n"); LINE+=("$(printf '%s' "$out" | tail -1)"); CODE+=("$rc")
}

run acceptance ./tests/acceptance.sh
run vm         ./tests/vm.sh
run difftest   ./tests/difftest.sh
run native     bash -c "./tests/native.sh $CORPUS"
run artifacts  ./tests/artifacts.sh
run ccrun      bash -c "./tests/ccrun.sh examples/*.c tests/c/a_*.c"
run selfhost   bash -c "./tests/selfhost.sh $CORPUS"
run bootstrap  ./tests/bootstrap.sh
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
