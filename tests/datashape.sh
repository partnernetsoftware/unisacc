#!/bin/bash
# Data laid out in shapes the corpus never asks for. [A-41] [S-13]
#
# tests/gen_data.py writes programs whose globals are declared in one order
# and allocated in another, with tentative definitions completed further
# down, string literals in between, and blob lengths that are not multiples
# of 8.  Every one is a function of its seed, so a failure names a file that
# regenerates byte for byte.
#
# Then the ordinary closure runs on them: the two back ends must write the
# same bytes for all six targets.  The shape being checked is asserted
# FIRST -- a generator that stopped producing it would leave this suite
# passing while testing nothing, which is the failure mode this suite
# exists to rule out.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
N=${N:-12}
mkdir -p "$T/gen"
python3 tests/gen_data.py "$T/gen" "$N" >/dev/null || { echo "  FAIL generator"; exit 1; }
files=$(ls "$T/gen"/gen*.c 2>/dev/null)
[ -n "$files" ] || { echo "  FAIL the generator wrote nothing"; exit 1; }

# 1. the programs are C: the system compiler agrees with the tape VM
bad=0; n=0
for f in $files; do
    n=$((n+1))
    cc -w -o "$T/ref" "$f" 2>/dev/null || { echo "  FAIL cc refused $(basename "$f")"; bad=$((bad+1)); continue; }
    "$T/ref" > "$T/want" 2>/dev/null
    perl -e 'alarm 120; exec @ARGV' "$UA" -run "$f" > "$T/got" 2>/dev/null
    cmp -s "$T/want" "$T/got" || { bad=$((bad+1))
        printf "  FAIL %s: want [%s] got [%s]\n" "$(basename "$f")" \
            "$(cat "$T/want")" "$(cat "$T/got")"; }
done

# 2. the shape is really there, and the front end handles it: the generator
#    writes `int gN;` and completes it further down with `int gN = v;` --
#    ONE object, which C calls a tentative definition.  The tape must name
#    it once (two `.bss` lines reserved space nobody used, and put a symbol
#    out of address order [E-61]), and step 1 above has already checked the
#    value survives.
tent=0; dup=0
for f in $files; do
    t=$(awk '/^int g[0-9]+;$/{n[$2]++} END{for (k in n) if (n[k]) c++; print c+0}' "$f")
    c=$(grep -cE '^int g[0-9]+;$' "$f")
    v=$(grep -cE '^int g[0-9]+ = [0-9]+;$' "$f")
    [ "$c" -gt 0 ] && [ "$v" -gt 0 ] && tent=$((tent+1))
    d=$(perl -e 'alarm 120; exec @ARGV' "$UA" "$f" -t lnx/x86_64 2>/dev/null |
        awk '/^\.(bss|str) /{print $2}' | sort | uniq -d | wc -l)
    dup=$((dup + d))
done
[ "$tent" -gt 0 ] || { echo "  FAIL the generator no longer writes a tentative definition"; bad=$((bad+1)); }
[ "$dup" -eq 0 ] || { echo "  FAIL a global reached the tape twice ($dup names)"; bad=$((bad+1)); }

# 3. and the two back ends agree on all six targets
./tests/closure.sh $files || bad=$((bad+1))

echo
echo "datashape  programs $n   wrong $bad   (with a tentative definition: $tent, named twice in a tape: $dup)"
[ "$bad" -eq 0 ] && [ "$n" -gt 0 ]
