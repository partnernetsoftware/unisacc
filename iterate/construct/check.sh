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
# UBSan build: every signed shift, subtraction and overflow is checked and
# the first report aborts (no recovery).  Only exit 0 with no report counts.
B 60 cc -std=c99 -O1 -w -fsanitize=undefined -fno-sanitize-recover=undefined -o "$T/c_san" iterate/construct/construct.c || { echo "ubsan build failed"; exit 1; }
# quotient-key sets are QW-word bitsets of QB = 62 bits: the self-test (-Q)
# checks set, test, and, or, andnot (tail cut), popcount, in-order
# iteration, equality and bits 62/63 clear at keys QB-1, QB, 2QB-1, 2QB
# and the last key, for nq 1, 61, 62, 63, 123, 124, 125, 528, 1023, 1024
for b in cc ua san; do
    B 30 "$T/c_$b" -Q > "$T/q.$b" 2>&1; rc=$?
    if [ $rc -eq 0 ] && [ "$(grep -c ', ok$' "$T/q.$b")" = 10 ] && ! grep -q 'runtime error' "$T/q.$b"; then echo "qset self-test $b ok (10 sizes)"
    else echo "qset self-test $b FAILED (rc $rc): $(tail -1 "$T/q.$b")"; fail=1; fi
done
for s in prec reloc tyinfo regmap pp lex scope pfconv binsel enc opinfo peep; do
    B 60 python3 iterate/construct/tools/netdump.py -d "weights/gold/$s.tsv" > "$T/$s.py" || fail=1
    for b in cc ua; do
        B 30 "$T/c_$b" -d "weights/gold/$s.tsv" > "$T/$s.$b" || fail=1
        if cmp -s "$T/$s.py" "$T/$s.$b"; then echo "$s $b identical ($(wc -c < "$T/$s.py" | tr -d ' ') B)"
        else echo "$s $b DIFFERS"; diff "$T/$s.py" "$T/$s.$b" | head -5; fail=1; fi
    done
    # every stage's dump and UNS2 under UBSan: exit 0, no report, same bytes
    B 30 "$T/c_san" -d "weights/gold/$s.tsv" > "$T/$s.san" 2> "$T/$s.san.err"; rc=$?
    B 30 "$T/c_san" -u "$T/$s.san.uns2" "weights/gold/$s.tsv" > /dev/null 2>> "$T/$s.san.err"; rc2=$?
    B 30 "$T/c_cc" -u "$T/$s.cc.uns2" "weights/gold/$s.tsv" > /dev/null 2>&1
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && [ ! -s "$T/$s.san.err" ] && cmp -s "$T/$s.py" "$T/$s.san" && cmp -s "$T/$s.cc.uns2" "$T/$s.san.uns2"; then echo "$s ubsan clean (dump, UNS2)"
    else echo "$s ubsan FAILED (rc $rc/$rc2): $(head -1 "$T/$s.san.err")"; fail=1; fi
done
# UNS2: both builds write the prec+reloc blob.  It must equal, RAW BYTES, the
# blob uns2.dump writes from the Python constructor (uns2slice.py); each
# stage section must equal the same-named section of the shipped pack; and
# the blob decoded by the deployment loader (uns2.load + IntNet.predict)
# must give the TSV label, uniquely, on every key (uns2round.py).  The
# deployment invariants (activation in {0,1}, b1 = 1 - constrained fields)
# are checked inside construct over the full domain: a violation exits 3.
# Three blobs: prec+reloc (single-head), tyinfo alone (multi-head, T4),
# regmap alone (two fields, T5 factored).
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
uns2 regmap weights/gold/regmap.tsv
# acceptance batch: one blob per stage (single-stage packs; MAXS is 8)
for s in pp lex scope pfconv binsel enc opinfo peep; do uns2 $s weights/gold/$s.tsv; done
# the multi-head branch trace (-t): which candidate each head chose, whether
# pick moved, what T4 did.  A debug print, not compared with Python (the
# counts were cross-checked once by hand); the two builds must agree.
for s in tyinfo regmap pp lex scope pfconv binsel enc opinfo peep; do
    for b in cc ua; do B 30 "$T/c_$b" -t weights/gold/$s.tsv > "$T/t.$b" 2>&1 || fail=1; done
    if cmp -s "$T/t.cc" "$T/t.ua"; then sed "s/^/$s /" "$T/t.cc"; else echo "$s trace differs between builds"; fail=1; fi
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
# capacity POSITIVE: a synthetic table with 63 quotient keys.  It used to be
# the capacity negative (MAXQ was 62, one long); with QW-word sets it is
# inside capacity and must BUILD, verify over its full domain (construct's
# own check, exit 0), and match the Python reference, which builds the same
# TSV through tsvgold: -d dump, UNS2 blob, deployed round trip.  Made from a
# temp copy of regmap.tsv (its #head line and 16 classes): fields of 9 and 7
# values, label class[a] when b = 0 and class[8 + b] otherwise, so no two
# values of a field have the same slice and nothing merges: 9 x 7 = 63.
# (The old negative's label, class[(7a+b) % 16], needs 63 decision-list
# rules, more than MAXR 62 -- a different limit, not widened here; this
# label needs 15.)
cp weights/gold/regmap.tsv "$T/cap.src"
awk -F'	' 'NR==1{print "# stage capq: 63 keys, synthetic"; next}
/^#head/{print "#field\ta\tv0\tv1\tv2\tv3\tv4\tv5\tv6\tv7\tv8"; print "#field\tb\tw0\tw1\tw2\tw3\tw4\tw5\tw6"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<9;a++) for(b=0;b<7;b++) print "v" a "\tw" b "\t" c[b ? 8 + b : a]; exit}' "$T/cap.src" > "$T/capq.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capq.tsv" > "$T/capq.py" || fail=1
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.capq.uns2" "$T/capq.tsv" > /dev/null || fail=1
for b in cc ua; do
    B 30 "$T/c_$b" -d "$T/capq.tsv" > "$T/capq.$b.out" 2>&1; rc=$?
    # 9 + 7 singleton groups, i.e. 9 x 7 = 63 quotient keys, none merged
    grp=$(grep '^groups' "$T/capq.$b.out" | tr -cd '[' | wc -c | tr -d ' ')
    if [ $rc -eq 0 ] && [ "$grp" = 16 ] && grep -q "^exact 63$" "$T/capq.$b.out" && cmp -s "$T/capq.py" "$T/capq.$b.out"; then
        echo "capacity positive $b: 63 quotient keys built, exact over 63 keys, -d identical to Python ($(wc -c < "$T/capq.py" | tr -d ' ') B)"
    else
        echo "capacity positive $b FAILED (rc $rc): $(head -1 "$T/capq.$b.out")"; fail=1
    fi
    B 30 "$T/c_$b" -u "$T/$b.capq.uns2" "$T/capq.tsv" > /dev/null 2>&1 || fail=1
    if cmp -s "$T/py.capq.uns2" "$T/$b.capq.uns2"; then echo "capacity positive $b: UNS2 identical ($(wc -c < "$T/py.capq.uns2" | tr -d ' ') B)"
    else echo "capacity positive $b: UNS2 DIFFERS"; fail=1; fi
    B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.capq.uns2" "$T/capq.tsv" > "$T/r.$b.out" 2>&1 || fail=1
    sed "s/^/capacity positive $b deployed /" "$T/r.$b.out"
done
# capacity NEGATIVE: more quotient keys than MAXQ (1024) must be rejected
# before any set is built: exit exactly 4 and the capacity diagnostic.
# Fields of 11, 11 and 9 values, label class[(a + 3b + 5c) % 16]: a shift
# of one field's value by d changes the label by d, 3d or 5d mod 16, never
# 0 for d <= 10, so no two values share a slice: 11 x 11 x 9 = 1089.
awk -F'	' 'NR==1{print "# stage capn: 1089 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<11;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<11;i++) printf "\tw%d", i
  printf "\n#field\tc"; for(i=0;i<9;i++) printf "\tx%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\tc\t=> y"
  for(a=0;a<11;a++) for(b=0;b<11;b++) for(x=0;x<9;x++) print "v" a "\tw" b "\tx" x "\t" c[(a+3*b+5*x)%16]; exit}' "$T/cap.src" > "$T/capn.tsv"
for b in cc ua; do
    B 30 "$T/c_$b" "$T/capn.tsv" > "$T/capn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: 1089 quotient keys so far, more than 1024" "$T/capn.$b.out"; then
        echo "capacity negative $b rejected: $(head -1 "$T/capn.$b.out")"
    else
        echo "capacity negative $b NOT A CAPACITY REJECTION (rc $rc): $(head -1 "$T/capn.$b.out")"; fail=1
    fi
done
# field-group NEGATIVE: a field's group set is one long, so a field may have
# at most 62 value groups.  Raw values are legal (70 <= MAXV 128) and there
# are 140 <= 1024 quotient keys, but field a has 70 groups: exit exactly 4
# with the field-group diagnostic, from domain() before any group shift.
# Label (a, b) = class[b ? 8 + a / 16 : a % 16] with 16 classes: the pair
# (label(a,0), label(a,1)) = (a % 16, 8 + a / 16) is distinct for every a.
awk -F'	' 'NR==1{print "# stage capg: 70 x 2 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<70;i++) printf "\tv%d", i; printf "\n#field\tb\tw0\tw1\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<70;a++) for(b=0;b<2;b++) print "v" a "\tw" b "\t" c[b ? 8 + int(a/16) : a%16]; exit}' "$T/cap.src" > "$T/capg.tsv"
for b in cc ua san; do
    B 30 "$T/c_$b" "$T/capg.tsv" > "$T/capg.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: field a has 70 value groups, more than 62" "$T/capg.$b.out" && ! grep -q 'runtime error' "$T/capg.$b.out"; then
        echo "field-group negative $b rejected: $(head -1 "$T/capg.$b.out")"
    else
        echo "field-group negative $b NOT A FIELD-GROUP REJECTION (rc $rc): $(head -1 "$T/capg.$b.out")"; fail=1
    fi
done
# the deployment invariants must be able to fire: the test entry breaks b1
# of unit 0 (-T bias), or breaks it and skips invariant 2 (-T act) so that
# invariant 1 is the one reached.  Only exit 3 with that invariant's
# diagnostic counts.
for s in prec tyinfo regmap pp lex scope pfconv binsel enc opinfo peep; do
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
