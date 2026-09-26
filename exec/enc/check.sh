#!/bin/sh
# exec/enc/check.sh -- E5's slices: the x86_64 encoder delta on the hand-written
# fixtures, against unisa/assemble.py + emit_x86 through exec/enc/ref.py (which
# fails unless every instruction encodes).  Both executors must agree with it;
# the branch fixtures' bytes are also checked against hand-worked expectations
# (br-expect.txt); the neg-* fixtures must be rejected, not encoded.
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
D=exec/enc
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "e5: cc failed"; exit 1; }
b 60 python3 $D/gen.py "$T/d.json" 2>/dev/null || { echo "e5: gen failed"; exit 1; }
b 60 python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl" || { echo "e5: tbl failed"; exit 1; }
ok=0; bad=0
for F in $D/x86-fixture.txt $D/br-*.txt; do
    case $F in *br-expect.txt) continue ;; esac
    b 60 python3 $D/ref.py "$F" > "$T/ref" || { echo "  BAD $F: the referee failed"; bad=$((bad+1)); continue; }
    b 60 "$T/run" "$T/d.tbl" "$F" > "$T/c"; rc=$?
    b 60 python3 exec/pp/sim.py "$T/d.json" "$F" > "$T/p"; rp=$?
    if [ -s "$T/ref" ] && [ $rc -eq 0 ] && [ $rp -eq 0 ] && cmp -s "$T/c" "$T/ref" && cmp -s "$T/p" "$T/ref"; then :; else
        echo "  BAD $F: run.c $rc, sim.py $rp, or the bytes differ"; bad=$((bad+1)); continue; fi
    x=$(grep "^$(basename "$F") " $D/br-expect.txt | awk '{print $2, $3}')
    if [ -n "$x" ]; then
        set -- $x
        got=$(dd if="$T/c" bs=1 skip="$1" count=$(( ${#2} / 2 )) 2>/dev/null | xxd -p | tr -d '\n')
        [ "$got" = "$2" ] || { echo "  BAD $F: at $1 got $got, worked by hand $2"; bad=$((bad+1)); continue; }
    fi
    ok=$((ok+1))
done
for F in $D/neg-*.txt; do     # must be rejected (exit 1 with a reason), on both executors
    b 60 "$T/run" "$T/d.tbl" "$F" > /dev/null 2> "$T/e"; rc=$?
    b 60 python3 exec/pp/sim.py "$T/d.json" "$F" > /dev/null 2>&1; rp=$?
    if [ $rc -eq 1 ] && [ $rp -eq 1 ] && grep -q "not covered" "$T/e"; then ok=$((ok+1)); else
        echo "  BAD $F: not rejected (run.c $rc, sim.py $rp)"; bad=$((bad+1)); fi
done
echo "e5 x86  fixtures ok $ok   bad $bad"
[ $bad -eq 0 ] && [ $ok -gt 0 ]
