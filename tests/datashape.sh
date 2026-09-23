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
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || ./tests/build_ref.sh >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
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

# 2. the shape is really there: at least one program defines a data symbol
#    twice, which is what puts a symbol out of address order
dup=0
for f in $files; do
    d=$(perl -e 'alarm 120; exec @ARGV' "$UA" "$f" -t lnx/x86_64 2>/dev/null |
        awk '/^\.(bss|str) /{print $2}' | sort | uniq -d | wc -l)
    dup=$((dup + d))
done
[ "$dup" -gt 0 ] || { echo "  FAIL the generator no longer produces a symbol defined twice"; bad=$((bad+1)); }

# 3. and the two back ends agree on all six targets
./tests/closure.sh $files || bad=$((bad+1))

echo
echo "datashape  programs $n   wrong $bad   (symbols defined twice: $dup)"
[ "$bad" -eq 0 ] && [ "$n" -gt 0 ]
