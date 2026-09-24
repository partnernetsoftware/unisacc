#!/bin/bash
# What the compiler warns about, and what it does not. [S-15 C4]
#
# Four warnings under -Wall: an unused local, a non-void function whose
# body can fall off the end, a printf conversion that does not match its
# argument, and an integer handed to a pointer.  Each probe in tests/warn/
# is judged against `cc -Wall` on the SAME file: the set of (line, kind)
# pairs must be equal in both directions -- a warning cc does not give is
# as wrong as one we miss.  Kinds are clang's own tags, which unisacc
# prints too: unused-variable (clang also spells "set but not used"),
# return-type, format, int-conversion (an error in clang, a warning here).
#
# Then the corpus: a warning on a program cc -Wall accepts silently is a
# false positive, and 220 programs by other people are the instrument.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
kinds() {   # kinds <file> -- "line kind" per diagnostic of the four kinds, sorted
    sed -E -n 's/^[^:]*:([0-9]+):[0-9]+: (warning|error): .*\[-W([a-z-]+)\].*$/\1 \3/p' "$1" \
    | sed 's/unused-but-set-variable/unused-variable/' \
    | grep -E " (unused-variable|return-type|format|int-conversion)$" | sort -u
}
ok=0; bad=0
for f in tests/warn/*.c; do
    b=$(basename "$f" .c)
    cc -std=c99 -Wall -fsyntax-only "$f" > "$T/cc.out" 2>&1
    perl -e 'alarm 60; exec @ARGV' "$UA" -Wall "$f" -c -o "$T/x.tape" > "$T/ua.out" 2>&1
    kinds "$T/cc.out" > "$T/cc.k"; kinds "$T/ua.out" > "$T/ua.k"
    if [ ! -s "$T/cc.k" ]; then echo "  FAIL $b: cc -Wall gives none of the four kinds here"; bad=$((bad+1)); continue; fi
    if cmp -s "$T/cc.k" "$T/ua.k"; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %-10s\n" "$b"; diff "$T/cc.k" "$T/ua.k" | sed 's/^/        /'; fi
done
# no warning cc does not give: the corpus, file by file
fp=0; n=0
for f in corpus/c-testsuite/tests/single-exec/*.c; do
    b=$(basename "$f" .c); n=$((n+1))
    cc -std=c99 -Wall -fsyntax-only "$f" > "$T/cc.out" 2>&1
    perl -e 'alarm 60; exec @ARGV' "$UA" -Wall "$f" -c -o "$T/x.tape" > "$T/ua.out" 2>&1
    kinds "$T/cc.out" > "$T/cc.k"; kinds "$T/ua.out" > "$T/ua.k"
    extra=$(comm -13 "$T/cc.k" "$T/ua.k")
    if [ -n "$extra" ]; then fp=$((fp+1)); printf "  FALSE %-8s %s\n" "$b" "$(echo "$extra" | head -1)"; fi
done
# the parser must have read SOMETHING from cc over the corpus, or the
# false-positive count above is vacuous (the first version's sed matched
# nothing on BSD and reported a clean corpus)
ccany=0
for f in corpus/c-testsuite/tests/single-exec/*.c; do
    cc -std=c99 -Wall -fsyntax-only "$f" 2>&1 | kinds /dev/stdin | grep -q . && ccany=$((ccany+1))
done
[ "$ccany" -gt 0 ] || { echo "  FAIL the warning parser read nothing from cc over the corpus"; bad=$((bad+1)); }
echo
echo "warn  probes ok $ok   wrong $bad   corpus files $n (cc warns in $ccany)   false positives $fp"
[ "$bad" -eq 0 ] && [ "$fp" -eq 0 ] && [ "$ok" -gt 0 ]
