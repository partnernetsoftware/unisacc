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
for s in prec reloc; do
    B 60 python3 iterate/construct/tools/netdump.py -d "weights/gold/$s.tsv" > "$T/$s.py" || fail=1
    for b in cc ua; do
        B 30 "$T/c_$b" -d "weights/gold/$s.tsv" > "$T/$s.$b" || fail=1
        if cmp -s "$T/$s.py" "$T/$s.$b"; then echo "$s $b identical ($(wc -c < "$T/$s.py" | tr -d ' ') B)"
        else echo "$s $b DIFFERS"; diff "$T/$s.py" "$T/$s.$b" | head -5; fail=1; fi
    done
done
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
rm -rf "$T"
[ $fail = 0 ] && echo "construct check: ok" || echo "construct check: FAILED"
exit $fail
