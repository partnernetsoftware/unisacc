#!/bin/sh
# Byte-compare construct.c (cc build and unisacc build) against the Python
# constructor for the stages in this slice, and check the C reader rejects
# damaged tables.  Every step is bounded (alarm); run from the repo root.
#   iterate/construct/check.sh [ua]      ua: a unisacc binary (default /tmp/ua_ref)
set -u
UA=${1:-/tmp/ua_ref}
T=${TMPDIR:-/tmp}/construct_check.$$
mkdir -p "$T"
B() { perl -e 'alarm shift; exec @ARGV' "$@"; }
fail=0
B 60 cc -std=c99 -O2 -w -o "$T/c_cc" iterate/construct/construct.c || { echo "cc build failed"; exit 1; }
B 60 "$UA" -O2 iterate/construct/construct.c -b osx/arm64 -o "$T/c_ua" || { echo "unisacc build failed"; exit 1; }
for s in prec reloc tyinfo; do
    B 60 python3 iterate/construct/tools/netdump.py -d "weights/gold/$s.tsv" > "$T/$s.py" || fail=1
    for b in cc ua; do
        B 30 "$T/c_$b" -d "weights/gold/$s.tsv" > "$T/$s.$b" || fail=1
        if cmp -s "$T/$s.py" "$T/$s.$b"; then echo "$s $b identical ($(wc -c < "$T/$s.py" | tr -d ' ') B)"
        else echo "$s $b DIFFERS"; diff "$T/$s.py" "$T/$s.$b" | head -5; fail=1; fi
    done
done
# UNS2: both builds write the prec+reloc blob.  It must equal, RAW BYTES, the
# blob uns2.dump writes from the Python constructor (uns2slice.py); each
# stage section must equal the same-named section of the shipped pack; and
# the blob decoded by the deployment loader (uns2.load + IntNet.predict)
# must give the TSV label, uniquely, on every key (uns2round.py).  The
# deployment invariants (activation in {0,1}, b1 = 1 - constrained fields)
# are checked inside construct over the full domain: a violation exits 3.
# Two blobs: prec+reloc (single-head) and tyinfo alone (multi-head, T4).
uns2() {    # uns2 <tag> <tsv>...
    tag=$1; shift
    B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.$tag.uns2" "$@" > /dev/null || { echo "uns2 $tag python reference failed"; fail=1; }
    for b in cc ua; do
        B 30 "$T/c_$b" -u "$T/$b.$tag.uns2" "$@" > "$T/u.$b.out" 2>&1; rc=$?
        if [ $rc -ne 0 ]; then echo "uns2 $tag $b construct failed (rc $rc): $(head -1 "$T/u.$b.out")"; fail=1; continue; fi
        echo "invariants $tag $b ok (activation 0/1, b1 = 1 - constrained fields; full domain)"
        if cmp -s "$T/py.$tag.uns2" "$T/$b.$tag.uns2"; then echo "uns2 $tag $b identical to uns2.dump ($(wc -c < "$T/py.$tag.uns2" | tr -d ' ') B)"
        else echo "uns2 $tag $b DIFFERS from uns2.dump"; cmp "$T/py.$tag.uns2" "$T/$b.$tag.uns2" | head -2; fail=1; fi
        B 60 python3 iterate/construct/tools/uns2slice.py --shipped "$T/$b.$tag.uns2" weights/built.uns2 > "$T/s.$b.out" 2>&1 || fail=1
        sed "s/^/uns2 $b shipped /" "$T/s.$b.out"
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.$tag.uns2" "$@" > "$T/r.$b.out" 2>&1 || fail=1
        sed "s/^/deployed $b /" "$T/r.$b.out"
    done
}
uns2 single weights/gold/prec.tsv weights/gold/reloc.tsv
uns2 multi weights/gold/tyinfo.tsv
# the multi-head branch trace (-t): which candidate each head chose, whether
# pick moved, what T4 did.  A debug print, not compared with Python (the
# counts were cross-checked once by hand); the two builds must agree.
for b in cc ua; do B 30 "$T/c_$b" -t weights/gold/tyinfo.tsv > "$T/t.$b" 2>&1 || fail=1; done
if cmp -s "$T/t.cc" "$T/t.ua"; then sed 's/^/tyinfo /' "$T/t.cc"; else echo "trace differs between builds"; fail=1; fi
# negative inputs: each damaged copy of prec.tsv must be REJECTED BY THE
# READER -- exit status exactly 1 and the diagnostic for that damage.  A
# signal (>128), the watchdog, or any other failure is not a rejection.
src=weights/gold/prec.tsv
first=$(grep -n -v '^#' $src | sed -n 2p | cut -d: -f1)     # first data row
second=$((first + 1))
sed "${first}d" $src > "$T/missing.tsv"
sed "${second}s/.*/$(sed -n ${first}p $src)/" $src > "$T/duplicate.tsv"
sed "${first}s/	1\$/	NOPE/" $src > "$T/badlabel.tsv"
sed "${first}s/\$/	x/" $src > "$T/extracol.tsv"
sed "${first}s/^/zz/" $src > "$T/badkey.tsv"
for c in "missing|keys do not cover" "duplicate|key repeats" "badlabel|not a class of its head" \
         "extracol|wrong number of columns" "badkey|not a value of its field"; do
    n=${c%%|*}; want=${c#*|}
    for b in cc ua; do
        B 30 "$T/c_$b" "$T/$n.tsv" > "$T/$n.$b.out" 2>&1; rc=$?
        if [ $rc -eq 1 ] && grep -q "$want" "$T/$n.$b.out"; then
            echo "$n $b rejected: $(head -1 "$T/$n.$b.out")"
        else
            echo "$n $b NOT A READER REJECTION (rc $rc): $(head -1 "$T/$n.$b.out")"; fail=1
        fi
    done
done
# the deployment invariants must be able to fire: the test entry breaks b1
# of unit 0 (-T bias), or breaks it and skips invariant 2 (-T act) so that
# invariant 1 is the one reached.  Only exit 3 with that invariant's
# diagnostic counts.
for s in prec tyinfo; do
for c in "bias|has b1" "act|activation"; do
    t=${c%%|*}; want=${c#*|}
    for b in cc ua; do
        B 30 "$T/c_$b" -T "$t" weights/gold/$s.tsv > "$T/inv.$t.$b" 2>&1; rc=$?
        if [ $rc -eq 3 ] && grep -q "$want" "$T/inv.$t.$b"; then
            echo "invariant negative $s $t $b fires: $(head -1 "$T/inv.$t.$b" | sed 's/.*broken: //')"
        else
            echo "invariant negative $s $t $b DID NOT FIRE (rc $rc): $(head -1 "$T/inv.$t.$b")"; fail=1
        fi
    done
done
done
rm -rf "$T"
[ $fail = 0 ] && echo "construct check: ok" || echo "construct check: FAILED"
exit $fail
