#!/bin/bash
# The self-hosting closure, byte for byte. [S-7 item 7]
#
# unisacc now carries its own back end (src/unisacc_back.c): `unisacc FILE -b
# os/arch` writes the executable itself, with no Python anywhere.  That back
# end is a PORT of unisa/lower.py, assemble.py, the two encoders and the three
# image writers, so the check is the strictest one there is: for every probe
# and every target, the image unisacc writes must be the SAME BYTES the
# Python back end writes from the same tape.  One wrong field anywhere -- a
# header, a displacement, a REX prefix -- shows up here.
#
# On this host the images of this host's targets are also run, and must
# print what the reference VM prints for that tape.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || "$R/tests/build_ref.sh" >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
TARGETS="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64"
case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOSTT=osx/arm64;;
    Darwin/x86_64) HOSTT=osx/x86_64;;
    Linux/x86_64)  HOSTT=lnx/x86_64;;
    Linux/aarch64) HOSTT=lnx/arm64;;
    *) HOSTT=;;
esac
PAR_WAIT=4   # the host run waits on the first-launch scan
. "$R/tests/par.sh"
for f in "$@"; do
    b=$(basename "$f" .c)
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    "$UA" "$f" -c > "$D/tape" 2>/dev/null || { echo refused > "$D/verdict"; exit 0; }
    : > "$D/verdict"
    for t in $TARGETS; do
        tt=$(echo "$t" | tr / _)
        # a tape is compiled FOR an OS (<stdio.h> reads __linux__, _WIN32),
        # so each target's image is checked against that target's tape
        "$UA" "$f" -t "$t" > "$D/tape.$tt" 2>/dev/null
        python3 -m unisa compile "$D/tape.$tt" --from-tape -o "$D/py.$tt" \
            --target "$t" --drive built >/dev/null 2>&1
        "$UA" "$f" -b "$t" > "$D/ua.$tt" 2>/dev/null
        if cmp -s "$D/py.$tt" "$D/ua.$tt"; then echo "same $t" >> "$D/verdict"
        else echo "DIFF $t $(cmp "$D/py.$tt" "$D/ua.$tt" 2>&1 | head -1)" >> "$D/verdict"; fi
    done
    if [ -n "$HOSTT" ]; then
        tt=$(echo "$HOSTT" | tr / _)
        cp "$D/ua.$tt" "$D/run"; chmod +x "$D/run"
        command -v codesign >/dev/null && codesign -f -s - "$D/run" >/dev/null 2>&1
        (cd "$D" && perl -e 'alarm 30; exec @ARGV' ./run > "$D/run.out" 2>/dev/null)
        python3 -m unisa vm --os "${HOSTT%/*}" "$D/tape.$tt" > "$D/vm.out" 2>/dev/null
        cmp -s "$D/run.out" "$D/vm.out" && echo "ran" >> "$D/verdict" || echo "RANWRONG" >> "$D/verdict"
    fi
    ) &
done
wait
same=0; diff=0; refused=0; ran=0; ranwrong=0
for f in "$@"; do
    b=$(basename "$f" .c)
    v="$T/$b.d/verdict"
    if grep -q refused "$v"; then refused=$((refused+1)); continue; fi
    s=$(grep -c "^same" "$v"); d=$(grep -c "^DIFF" "$v")
    same=$((same+s)); diff=$((diff+d))
    grep "^DIFF" "$v" | sed "s/^/  $b: /"
    grep -q "^ran$" "$v" && ran=$((ran+1))
    grep -q RANWRONG "$v" && { ranwrong=$((ranwrong+1)); echo "  $b: the host image ran and disagreed with the VM"; }
done
echo
echo "closure  images identical $same   differ $diff   (refused $refused)   host-run ok $ran   wrong $ranwrong"
[ "$diff" -eq 0 ] && [ "$ranwrong" -eq 0 ]
