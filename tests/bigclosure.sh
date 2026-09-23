#!/bin/bash
# The closure on the one input big enough to matter: the compiler itself.
# [A-42] [S-13]
#
# closure.sh compares the two back ends on 90 probes of a few dozen lines
# each.  unisacc.c is 707 KB and has 1874 data symbols, and it was the only
# input that ever had a data symbol declared out of address order -- which
# segfaulted the C back end for every target while all 90 probes stayed
# green.  So the compiler's own image is compared here, byte for byte,
# against the image the Python back end writes from the same tape.
#
# It costs about six minutes.  That is the price of the only test that has
# ever seen a program this size.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
TARGETS="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64"
HOSTT=$(host_target)
same=0; diff=0
for t in $TARGETS; do
    tt=$(echo "$t" | tr / _)
    perl -e 'alarm 600; exec @ARGV' "$UA" unisacc.c -t "$t" > "$T/tape.$tt" 2>/dev/null \
        || { echo "  FAIL $t: the front end did not finish"; diff=$((diff+1)); continue; }
    perl -e 'alarm 1200; exec @ARGV' python3 -m unisa compile "$T/tape.$tt" \
        --from-tape -o "$T/py.$tt" --target "$t" --drive built >/dev/null 2>&1
    perl -e 'alarm 600; exec @ARGV' "$UA" unisacc.c -b "$t" > "$T/ua.$tt" 2>/dev/null
    if [ ! -s "$T/ua.$tt" ]; then
        echo "  FAIL $t: the C back end wrote nothing"; diff=$((diff+1)); continue
    fi
    if cmp -s "$T/py.$tt" "$T/ua.$tt"; then
        same=$((same+1)); printf "  %-12s identical  %s B\n" "$t" "$(wc -c < "$T/ua.$tt")"
    else
        diff=$((diff+1)); printf "  %-12s DIFF  %s\n" "$t" \
            "$(cmp "$T/py.$tt" "$T/ua.$tt" 2>&1 | head -1)"
    fi
done
# the host's own image is not just bytes: it has to be a compiler
ranok=0; ranwrong=0
if [ -n "$HOSTT" ]; then
    tt=$(echo "$HOSTT" | tr / _)
    if [ -s "$T/ua.$tt" ]; then
        cp "$T/ua.$tt" "$T/self"; chmod +x "$T/self"
        command -v codesign >/dev/null && codesign -f -s - "$T/self" >/dev/null 2>&1
        got=$(perl -e 'alarm 120; exec @ARGV' "$T/self" -run examples/hello.c 2>&1)
        if [ "$got" = "hello from C99" ]; then ranok=1
        else ranwrong=1; echo "  the self-compiled compiler ran and said [$got]"; fi
    fi
fi
echo
echo "bigclosure  targets identical $same   differ $diff   self-compiled ran $ranok   wrong $ranwrong"
[ "$same" -eq 6 ] && [ "$diff" -eq 0 ] && [ "$ranwrong" -eq 0 ]
