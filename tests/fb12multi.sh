#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Fixture-directory red examples: the probes difftest cannot express.
# [R13-0 / R13-0b, plan node N0]
#
# tests/difftest.sh compiles one file and runs it once.  That covers most of
# the external trial's items, but not the ones where the *shape* of the build
# is the defect:
#
#   22  more than one -I directory (last wins)
#   24  a quoted include that resolves relative to the includer
#   27  thirty translation units -- the signature pool is exhausted at 30
#   28  a file-scope static anonymous struct that only breaks in a second unit
#   30  label naming across miniz's four units
#   03  the second path of the assert item: running with an argument must fire
#       the assertion, which one compile-and-run cannot express
#
# So a fixture is a directory, tests/fb12/NN-*/:
#
#   build       one line, run with `sh -c` inside the directory; $UC is the
#               compiler under test.  Must succeed (exit 0) and leave `prog`
#               behind.  Omit the file when the fixture is run-only (22, 24).
#   run         one line, run after a successful build; expected stdout goes
#               in expect.txt and its exit status in expect.rc.
#   expect.txt  expected stdout of `run`, byte for byte (empty = no output)
#   expect.rc   expected exit status of `run`
#
# build and run are separate on purpose.  The first version chained them with
# `&&` in a single `cmd`, and 27 then READ AS PASSING: the product fails to
# compile it (`function signature pool exhausted`, exit 1, no output), and
# `cmd1 && cmd2` returns 1 -- which was also the expected exit status of the
# program under gcc.  A fixture where "the compiler refused" and "the program
# returned 1" are indistinguishable cannot see the defect it exists for.
#
# The verdict compares against the reference compiler's answer, recorded from
# the trial's .out.txt.  A fixture listed in tests/fb12.knownfail may fail; a
# listed fixture that passes is a failure, so closing a defect means deleting
# its line -- the same contract as tests/c99.knownfail and the two difftest
# lists.
#
# Usage:  fb12multi.sh [NAME...]        (default: every fixture)
#         MODEL_COM=... fb12multi.sh    (default ./unisacc.com)
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
UC=${MODEL_COM:-${UA:-$R/unisacc.com}}
[ -x "$UC" ] || { echo "fb12multi: no compiler at $UC" >&2; exit 2; }
KNOWN=$R/tests/fb12.knownfail
isknown() { grep -qs "^$1[[:space:]]" "$KNOWN"; }

if [ "$#" -gt 0 ]; then SET="$*"; else
    SET=""
    for d in tests/fb12/*/; do [ -f "$d/run" ] && SET="$SET $(basename "$d")"; done
fi
[ -n "$SET" ] || { echo 'fb12multi: no fixtures' >&2; exit 2; }

pass=0; fail=0; known=0; revived=0
for name in $SET; do
    d="tests/fb12/$name"
    [ -f "$d/run" ] && [ -f "$d/expect.rc" ] || { echo "  ?    $name (not a fixture)"; fail=$((fail+1)); continue; }
    want_rc=$(tr -d ' \r\n' < "$d/expect.rc")
    # bound each step: a compile that loops would otherwise hold the gate
    # (AGENTS.md: 60 s a run, and this suite must stay under it overall).
    build_rc=0
    if [ -f "$d/build" ]; then
        "$_BOUND" 45 sh -c "cd '$d' && UC='$UC' && $(cat "$d/build")" >/tmp/fb12-$name.build 2>&1
        build_rc=$?
        [ "$build_rc" -eq 0 ] || {
            # A build that fails is a defect like any other, and a listed one
            # is a known defect -- the first version recorded FAIL here and
            # skipped the list, which would have turned the gate red the moment
            # this suite was committed.
            if isknown "$name"; then
                known=$((known+1))
                printf "  known %-32s build rc=%s  %s\n" "$name" "$build_rc" \
                    "$(grep -m1 "^$name[[:space:]]" "$KNOWN" | cut -c1-46)"
            else
                fail=$((fail+1))
                printf "  FAIL  %-32s build rc=%s\n" "$name" "$build_rc"
                printf "        %s\n" "$(head -1 /tmp/fb12-$name.build | cut -c1-100)"
            fi
            continue; }
    fi
    got=$("$_BOUND" 45 sh -c "cd '$d' && UC='$UC' && $(cat "$d/run")" 2>/tmp/fb12-$name.err)
    rc=$?
    want=$(cat "$d/expect.txt")
    if [ "$rc" = "$want_rc" ] && [ "$got" = "$want" ]; then
        pass=$((pass+1)); printf "  ok    %-32s rc=%s\n" "$name" "$rc"
        if isknown "$name"; then
            revived=$((revived+1))
            printf "        %-32s listed in fb12.knownfail but passes: delete its line\n" "$name"
        fi
    elif isknown "$name"; then
        # say WHICH dimension still fails: a compile failure and an assertion
        # can share an exit status (03-assert-fires: both rc=1), and a known
        # line that only quoted rc read as progress that had not happened
        dim="rc=$rc (want $want_rc)"; [ "$got" = "$want" ] || dim="$dim stdout differs"
        known=$((known+1)); printf "  known %-32s %s  %s\n" "$name" "$dim" \
            "$(grep -m1 "^$name[[:space:]]" "$KNOWN" | cut -c1-46)"
    else
        fail=$((fail+1)); printf "  FAIL  %-32s rc=%s (want %s)\n" "$name" "$rc" "$want_rc"
        printf "        got  %s\n        want %s\n" "$(printf '%s' "$got" | head -2 | tr '\n' '|')" \
            "$(printf '%s' "$want" | head -2 | tr '\n' '|')"
    fi
done
rm -f /tmp/fb12-*.err /tmp/fb12-*.build
echo
echo "fb12-multi $((pass+fail+known))   pass $pass   wrong $fail   knownwrong $known   revived $revived"
# A suite that checked nothing is not green -- but "checked something" here
# means a fixture ran, not that one passed.  Every fixture in this suite is a
# known defect by construction (that is what it is for), so requiring pass > 0
# would make the suite permanently red; the first version did exactly that.
[ "$fail" -eq 0 ] && [ $((pass + known)) -gt 0 ] && [ "$revived" -eq 0 ]
