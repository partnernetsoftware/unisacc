#!/bin/bash
# Programs nobody wrote. [A-46]
#
# Every other suite tests something a person imagined: a probe, a corpus
# entry, a C99 feature, a data shape.  What survives is the COMBINATION
# nobody combined -- a cast inside a comparison inside a switch inside a
# loop.  tests/gen_prog.py writes those from a seed, the system compiler
# is the oracle, and a disagreement names a seed that regenerates the
# program byte for byte.
#
#   ./tests/fuzz.sh            the standing set: N seeds from FIRST, per class
#   N=125 FIRST=1000 ./tests/fuzz.sh     a longer hunt (8 classes x 125 = 1000)
#   CLASSES="struct unsigned" ./tests/fuzz.sh     only those shapes
#
# The generator has eight program SHAPES [S-15 A3] -- int, unsigned,
# narrow, struct, pointer, float, recursion, mixed -- and each gets N
# seeds, so one run is 8N programs.  The seven added shapes found three
# C-front-end bugs on their first run (tests/c/b_fuzzfound.c keeps them).
#
# The generator's rules keep every program's behaviour DEFINED -- if it
# were not, cc and unisacc would be allowed to differ and this would
# measure nothing.  The rule that took a bug to learn: a loop counter is
# READ-ONLY inside the body.  The first version let the body assign it,
# and seed 2 produced a program that ran for more than twenty seconds
# under cc as well as under unisacc; a generator that writes
# non-terminating programs measures the timeout.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
N=${N:-20}
FIRST=${FIRST:-1}
CLASSES=${CLASSES:-"int unsigned narrow struct pointer float recursion mixed"}
command -v cc >/dev/null || { echo "  skip (no system compiler to compare against)"; exit 0; }
total=0
for c in $CLASSES; do
    python3 tests/gen_prog.py "$T" "$N" "$FIRST" "$c" >/dev/null || {
        echo "  FAIL the generator did not run for class $c"; exit 1; }
    total=$((total + N))
done
ok=0; bad=0; uns=0; skip=0
for f in "$T"/*_*.c; do
    b=$(basename "$f" .c)
    cls=""; for c in $CLASSES; do [ "${b%%_*}" = "${c:0:1}" ] && cls=$c; done
    if ! cc -w -std=c99 -o "$T/ref" "$f" 2>/dev/null; then
        # the oracle refusing its own generator is a generator bug, and a
        # loud one: it means the programs are not C
        echo "  FAIL cc refused $b -- the generator emitted something that is not C"
        bad=$((bad+1)); continue
    fi
    if ! want=$(bound 5 "$T/ref" 2>/dev/null); then
        echo "  FAIL $b does not terminate under cc -- a generator bug"
        bad=$((bad+1)); continue
    fi
    got=$(bound 30 "$UA" -run "$f" 2>"$T/err")
    if [ -s "$T/err" ]; then
        uns=$((uns+1))
        printf "  UNS   %-10s %s\n" "$b" "$(head -1 "$T/err" | cut -c1-58)"
        continue
    fi
    if [ "$got" = "$want" ]; then ok=$((ok+1))
    else
        bad=$((bad+1))
        printf "  WRONG %-10s got [%s]  cc says [%s]\n" "$b" "$got" "$want"
        printf "        reproduce: python3 tests/gen_prog.py DIR 1 %s %s\n" "${b#*_}" "$cls"
    fi
done
echo
echo "fuzz  agreed $ok/$total   wrong $bad   refused $uns   (seeds $FIRST..$((FIRST + N - 1)) in each of: $CLASSES)"
[ "$bad" -eq 0 ] && [ "$uns" -eq 0 ] && [ "$ok" -gt 0 ]
