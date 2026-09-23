#!/bin/bash
# unisacc compiles C -> tape; the reference VM runs it.  Compare against the
# Python compiler's own output for the same program.  [A-21]
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
UA=${UA:-/tmp/ua_ref}
BASE=$R/tests/ccrun.baseline
KNOWN=$R/tests/ccrun.knownwrong
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0; uns=0; known=0; revived=0
: > "$T/passing"
isknown() { grep -qs "^$1[[:space:]]" "$KNOWN"; }
for f in "$@"; do
    b=$(basename "$f" .c)
    want=$(python3 -m unisa run "$f" --drive built 2>/dev/null)
    if ! $UA "$f" -c > /tmp/cc_$b.tape 2>/tmp/cc_$b.err; then
        uns=$((uns+1))
        printf "  UNS  %-12s %s\n" "$b" "$(head -1 /tmp/cc_$b.err|cut -c1-48)"; continue
    fi
    got=$(python3 -m unisa vm /tmp/cc_$b.tape 2>/dev/null)
    if [ "$got" = "$want" ]; then
        if isknown "$b"; then
            revived=$((revived+1))
            printf "  REVIVED %s  now agrees -- drop it from ccrun.knownwrong\n" "$b"
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
ratchet "$BASE" "$pass" "$T/passing" || rc=1
exit $rc
