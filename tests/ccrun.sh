#!/bin/bash
# unisacc compiles C -> tape; the reference VM runs it.  Compare against the
# Python compiler's own output for the same program.  [A-21]
set -u
UA=${UA:-/tmp/ua_ref}
pass=0; fail=0; uns=0
for f in "$@"; do
    b=$(basename "$f" .c)
    want=$(python3 -m unisa run "$f" --drive built 2>/dev/null)
    if ! $UA "$f" -c > /tmp/cc_$b.tape 2>/tmp/cc_$b.err; then
        uns=$((uns+1))
        printf "  UNS  %-12s %s\n" "$b" "$(head -1 /tmp/cc_$b.err|cut -c1-48)"; continue
    fi
    got=$(python3 -m unisa vm /tmp/cc_$b.tape 2>/dev/null)
    if [ "$got" = "$want" ]; then pass=$((pass+1)); printf "  ok   %-12s %s\n" "$b" "$got"
    else fail=$((fail+1)); printf "  FAIL %-12s got '%s' want '%s'\n" "$b" "$got" "$want"; fi
done
# `uns` is not a failure here -- unisacc covers a smaller subset on purpose,
# and tests/selfgap.sh is the ratchet that keeps that gap shrinking.  But it
# used to be invisible: a suite that refused every program still read `ok`.
echo; echo "unisacc-compiled $pass   wrong $fail   refused $uns"
[ "$fail" -eq 0 ]
