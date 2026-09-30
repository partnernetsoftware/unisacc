#!/bin/bash
# unisacc compiles C -> tape; the reference VM runs it.  Compare against the
# Python compiler's own output for the same program.  [A-21]
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
_BOUND=$("$R/tests/bound" --helper) || exit 2
UA=${UA:-/tmp/ua_ref}
BASE=$R/tests/ccrun.baseline
KNOWN=$R/tests/ccrun.knownwrong
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0; uns=0; known=0; revived=0
: > "$T/passing"
PYFRONT=$R/tests/pyfront.knownfail   # Python-front-end gaps, shared with fat.sh/native.sh
REFUSED=$R/tests/difftest.knownfail  # probes the C reference refuses on purpose (e.g. #21), as closure.sh/e4 treat them
. "$R/tests/knownfail.sh"
knownfail_load_many "$T/known.keys" "$KNOWN" "$PYFRONT" "$REFUSED" || exit 1
isknown() { knownfail_has "$1"; }
for f in "$@"; do
    b=$(basename "$f" .c)
    rm -f "$T/ref.status"
    # Independent VM routes overlap; the full allocator churn stays intact.
    ("$_BOUND" --status "$T/ref.status" 45 python3 -m unisa run "$f" --drive built >"$T/ref.out" 2>"$T/ref.err"; echo $? >"$T/ref.rc") & refpid=$!
    if ! "$_BOUND" 25 "$UA" "$f" -c > "$T/$b.tape" 2>"$T/$b.err" || [ ! -s "$T/$b.tape" ]; then
        wait "$refpid"
        uns=$((uns+1))
        printf "  UNS  %-12s %s\n" "$b" "$(head -1 "$T/$b.err"|cut -c1-48)"; continue
    fi
    rm -f "$T/vm.status"
    got=$("$_BOUND" --status "$T/vm.status" 45 python3 -m unisa vm "$T/$b.tape" 2>"$T/vm.err"); grc=$?
    wait "$refpid"
    want=$(cat "$T/ref.out"); wrc=$(cat "$T/ref.rc")
    ws=$(cat "$T/ref.status" 2>/dev/null)
    if [ -z "$ws" ] || [ $((ws & 127)) -ne 0 ]; then fail=$((fail+1)); echo "  FAIL $b reference timeout/signal $wrc"; cat "$T/ref.err"; continue; fi
    gs=$(cat "$T/vm.status" 2>/dev/null)
    if [ -z "$gs" ] || [ $((gs & 127)) -ne 0 ]; then fail=$((fail+1)); echo "  FAIL $b VM timeout/signal $grc"; continue; fi
    if [ "$got" = "$want" ] && [ "$wrc" -eq "$grc" ]; then
        if isknown "$b"; then
            revived=$((revived+1))
            printf "  REVIVED %s  now agrees -- drop it from ccrun.knownwrong / pyfront.knownfail\n" "$b"
        else
            pass=$((pass+1)); echo "$b" >> "$T/passing"
            printf "  ok   %-12s %s\n" "$b" "$got"
        fi
    elif isknown "$b"; then
        known=$((known+1))
    else
        fail=$((fail+1)); printf "  FAIL %-12s got '%s' want '%s'\n" "$b" "$got" "$want"
    fi
done
# `uns` is not a failure here -- unisacc covers a smaller subset on purpose,
# and tests/selfgap.sh is the ratchet that keeps that gap shrinking.  But it
# used to be invisible: a suite that refused every program still read `ok`.
echo
echo "unisacc-compiled $pass   wrong $fail   knownwrong $known   refused $uns"

rc=0
[ "$fail" -eq 0 ] || rc=1
[ "$revived" -eq 0 ] || rc=1
# The ratchet counts the WHOLE probe set; a caller that passes three files is
# asking a different question, and must say so.
if [ "${NOBASE:-0}" = "1" ]; then
    exit $rc
fi
if [ "${CCRUN_SHARD:-0}" = 1 ]; then
    [ -s "$BASE.list" ] || { echo 'ccrun baseline list missing'; exit 1; }
    for f in "$@"; do basename "$f" .c; done | sort > "$T/selected"
    comm -12 "$BASE.list" "$T/selected" > "$T/required"
    sort "$T/passing" > "$T/passed"
    comm -23 "$T/required" "$T/passed" > "$T/lost"
    if [ -s "$T/lost" ]; then echo 'REGRESSION: missing required names in this shard'; cat "$T/lost"; rc=1; fi
    [ "$pass" -gt 0 ] || rc=1
else
    ratchet "$BASE" "$pass" "$T/passing" || rc=1
fi
exit $rc
