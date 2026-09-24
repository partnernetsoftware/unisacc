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
# Each program in its own directory, PAR at a time: the reference binary's
# first run is a 0.5-0.9 s XProtect scan, and 160 of them in a row were
# over the 60 s a run may take (AGENTS.md).  Verdicts are read back in
# order, so the report is the same as the serial one.
PAR_WAIT=4
. "$R/tests/par.sh"
CACHE=${FUZZ_CACHE:-${TMPDIR:-/tmp}/unisacc-fuzz-want}; mkdir -p "$CACHE"
CCV=$(cc --version 2>&1 | head -1)
for f in "$T"/*_*.c; do
    b=$(basename "$f" .c)
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    # cc's answer for this exact source is cached: the programs are fixed by
    # their seeds, so a rerun needs neither cc nor the scan of its output
    key="$CACHE/$( (cat "$f"; echo "$CCV") | shasum | cut -c1-40)"
    if [ -f "$key" ]; then cp "$key" "$D/want"
    else
        if ! cc -w -std=c99 -o "$D/ref" "$f" 2>/dev/null; then echo notc > "$D/v"; exit 0; fi
        if ! bound 5 "$D/ref" > "$D/want" 2>/dev/null; then echo loops > "$D/v"; exit 0; fi
        cp "$D/want" "$key.$$" && mv "$key.$$" "$key"
    fi
    bound 30 "$UA" -run "$f" > "$D/got" 2>"$D/err"
    echo done > "$D/v"
    ) &
done
wait
ok=0; bad=0; uns=0; skip=0
for f in "$T"/*_*.c; do
    b=$(basename "$f" .c); D="$T/$b.d"
    cls=""; for c in $CLASSES; do [ "${b%%_*}" = "${c:0:1}" ] && cls=$c; done
    case "$(cat "$D/v" 2>/dev/null)" in
    notc)  echo "  FAIL cc refused $b -- the generator emitted something that is not C"
           bad=$((bad+1)); continue;;
    loops) echo "  FAIL $b does not terminate under cc -- a generator bug"
           bad=$((bad+1)); continue;;
    done)  ;;
    *)     echo "  FAIL $b: no verdict"; bad=$((bad+1)); continue;;
    esac
    if [ -s "$D/err" ]; then
        uns=$((uns+1))
        printf "  UNS   %-10s %s\n" "$b" "$(head -1 "$D/err" | cut -c1-58)"
        continue
    fi
    got=$(cat "$D/got"); want=$(cat "$D/want")
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
