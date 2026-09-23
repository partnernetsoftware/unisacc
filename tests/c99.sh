#!/bin/bash
# C99, feature by feature. [A-44]
#
# `corpus` reports 214 of 220 and that number is too kind: c-testsuite's
# programs are short and they overlap, so the denominator is a sample of
# the language rather than the language.  Here each probe in tests/c99/
# exercises ONE feature -- the ones C99 added over C89, plus the C89
# constructs everything else rests on -- and the denominator is written
# from the standard's own list of changes, not from what we support.  That
# matters: a denominator that moves with the numerator measures nothing.
#
# A feature counts as supported when unisacc compiles the probe AND the
# program's output matches the system compiler's.  Compiling is not
# passing: three of these once compiled and printed the wrong number.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
D=$R/tests/c99
ok=0; bad=0; refused=0
: > "$T/passing"
for f in "$D"/*.c; do
    b=$(basename "$f" .c)
    # the reference: whatever this machine's cc does with it
    if ! cc -std=c99 -w -o "$T/ref" "$f" -lm 2>/dev/null; then
        printf "  skip %-24s the system cc will not build it\n" "$b"
        continue
    fi
    want=$("$T/ref" 2>/dev/null); wrc=$?
    if ! bound 60 "$UA" -run "$f" > "$T/got" 2>"$T/err"; then
        grc=$?
        # a refusal and a wrong answer are different failures
        if [ -s "$T/err" ]; then
            printf "  UNS  %-24s %s\n" "$b" "$(head -1 "$T/err" | cut -c1-52)"
            refused=$((refused+1)); continue
        fi
        got=$(cat "$T/got")
        if [ "$got" = "$want" ] && [ "$grc" = "$wrc" ]; then
            ok=$((ok+1)); echo "$b" >> "$T/passing"; continue
        fi
        printf "  WRONG %-23s exit %s, cc says %s\n" "$b" "$grc" "$wrc"
        bad=$((bad+1)); continue
    fi
    got=$(cat "$T/got")
    if [ "$got" = "$want" ]; then
        ok=$((ok+1)); echo "$b" >> "$T/passing"
    else
        printf "  WRONG %-23s got [%s] cc says [%s]\n" "$b" "$got" "$want"
        bad=$((bad+1))
    fi
done
total=$((ok + bad + refused))
echo
printf "c99  supported %d/%d   wrong %d   refused %d   (%d%%)\n" \
    "$ok" "$total" "$bad" "$refused" "$(( total ? ok * 100 / total : 0 ))"

# A ratchet, because this is the number the project's claim rests on.
BASE=$R/tests/c99.baseline
ratchet "$BASE" "$ok" "$T/passing" || bad=$((bad+1))
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
