#!/bin/sh
# Byte-compare construct.c (cc build and unisacc build) against the Python
# constructor for the stages in this slice, and check the C reader rejects
# damaged tables.  Every step is bounded (alarm); run from the repo root.
#   iterate/construct/check.sh [ua]      ua: a unisacc binary (default /tmp/ua_ref)
#   STAGES="peep type" GLOBAL=0 iterate/construct/check.sh [ua]   a subset
#   iterate/construct/check.sh --batches [ua]   the whole list, in batches
# Selection: with neither STAGES nor GLOBAL set, every stage and every
# global check runs.  Setting either one makes the selection explicit:
# STAGES names stages from $ALL (or "all"; the pseudo-stage "global" turns
# the global checks on), GLOBAL=1/0 forces them on/off.  A selection that
# checks nothing, or names an unknown stage, fails with exit 2.
set -u
# THE stage list: name, UNS2 blob tag, trace (t/-), invariant negatives (i/-).
# Every per-stage loop below is derived from this table.
TABLE='prec single - i
reloc single - -
tyinfo multi t i
regmap regmap t i
pp pp t i
lex lex t i
scope scope t i
pfconv pfconv t i
binsel binsel t i
enc enc t i
opinfo opinfo t i
peep peep t i
parse parse t i
type type t i
abi abi t i'
ALL=$(echo "$TABLE" | cut -d" " -f1 | tr "\n" " " | sed "s/ $//")
[ -n "$ALL" ] || { echo "construct check: stage table is empty"; exit 2; }
GLOBALS="qset sum reader capq capr caprn caph caphn capk capkn capn capo capg capc"
# batch mode: each batch is "stages|global"; the union is checked below
# type alone is ~28 s (its trace/invariants dominate), so it gets its own batch
BATCHES='prec reloc tyinfo regmap pp lex scope|1
pfconv binsel enc opinfo peep parse|0
type|0
abi|0'
if [ "${1:-}" = --batches ]; then
    shift; UA=${1:-/tmp/ua_ref}
    ub=$(echo "$BATCHES" | cut -d'|' -f1 | tr ' ' '\n' | sort | tr '\n' ' ')
    ua=$(echo "$ALL" | tr ' ' '\n' | sort | tr '\n' ' ')
    ng=$(echo "$BATCHES" | grep -c '|1$')
    dup=$(echo "$BATCHES" | cut -d'|' -f1 | tr ' ' '\n' | sort | uniq -d)
    if [ "$ub" != "$ua" ] || [ -n "$dup" ] || [ "$ng" != 1 ]; then
        echo "batches: union is not the full list plus globals once (union: $ub; globals in $ng batches)"; exit 2
    fi
    echo "batches: union = all $(echo $ALL | wc -w | tr -d ' ') stages + global checks, each exactly once"
    bf=0; n=0
    echo "$BATCHES" > "${TMPDIR:-/tmp}/construct_batches.$$"
    while IFS='|' read -r st gl; do
        n=$((n + 1))
        t0=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
        STAGES=$st GLOBAL=$gl perl -e 'alarm 60; exec @ARGV' "$0" "$UA" > "${TMPDIR:-/tmp}/construct_batch.$$.$n" 2>&1; rc=$?
        t1=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
        sed "s/^/[batch $n] /" "${TMPDIR:-/tmp}/construct_batch.$$.$n"; rm -f "${TMPDIR:-/tmp}/construct_batch.$$.$n"
        el=$(perl -e "printf '%.1f', $t1 - $t0")
        echo "batch $n: stages [$st] global $gl: rc $rc, $el s (limit 60 s)"
        [ $rc -eq 0 ] || bf=1
    done < "${TMPDIR:-/tmp}/construct_batches.$$"
    rm -f "${TMPDIR:-/tmp}/construct_batches.$$"
    [ $bf = 0 ] && echo "construct check batches: ok" || echo "construct check batches: FAILED"
    exit $bf
fi
UA=${1:-/tmp/ua_ref}
if [ -z "${STAGES+x}" ] && [ -z "${GLOBAL+x}" ]; then SEL=$ALL; GL=1
else
    SEL=; GL=0
    for s in ${STAGES:-}; do
        case $s in
        all) SEL="$SEL $ALL" ;;
        global) GL=1 ;;
        *) case " $ALL " in *" $s "*) SEL="$SEL $s" ;;
           *) echo "construct check: unknown stage '$s' (stages: $ALL global all)"; exit 2 ;; esac ;;
        esac
    done
    case ${GLOBAL:-} in 1) GL=1 ;; 0) GL=0 ;; '') ;; *) echo "construct check: GLOBAL must be 0 or 1"; exit 2 ;; esac
    if [ -z "$SEL" ] && [ $GL = 0 ]; then
        echo "construct check: empty selection -- no stage and no global check selected (STAGES='${STAGES:-}' GLOBAL='${GLOBAL:-}'); nothing would be checked"; exit 2
    fi
fi
sel() { case " $SEL " in *" $1 "*) return 0 ;; esac; return 1; }
stages() { echo "$TABLE" | while read -r n tag tr iv; do sel $n || continue; case $1 in all) echo $n ;; t) [ $tr = t ] && echo $n ;; i) [ $iv = i ] && echo $n ;; esac; done; }
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
# iteration, equality and bits 62/63 clear at keys QB-1, QB, 2QB-1, 2QB,
# the last word's boundary and the last key, for nq 1, 61, 62, 63, 123, 124,
# 125, 528 and, at the end of MAXQ 3200 (QW 52), 3161, 3162, 3163, 3199, 3200
if [ $GL = 1 ]; then
for b in cc ua san; do
    B 30 "$T/c_$b" -Q > "$T/q.$b" 2>&1; rc=$?
    if [ $rc -eq 0 ] && [ "$(grep -c ', ok$' "$T/q.$b")" = 13 ] && ! grep -q 'runtime error' "$T/q.$b"; then echo "qset self-test $b ok (13 sizes)"
    else echo "qset self-test $b FAILED (rc $rc): $(tail -1 "$T/q.$b")"; fail=1; fi
done
# checked accumulation: -T summax/sumover/sumrun drive the SAME ladd() that
# every weight/logit sum calls.  max must succeed (exit 0); one past LONG_MAX
# and a running sum of 2^60 crossing it must be rejected by the guard (exit
# exactly 6, the overflow diagnostic), never by a UBSan report or a signal.
# The static half: the four accumulation sites are ladd calls (README).
nl=$(grep -cE '(z\[c\]|cw\[ci \* MAXU \+ found\]\[rl\[r\]\]) = ladd\(' iterate/construct/construct.c)
raw=$(grep -cE 'z\[c\] = z\[c\] \+|= cw\[.*\] \+' iterate/construct/construct.c)
if [ "$nl" = 4 ] && [ "$raw" = 0 ]; then echo "sum sites: 4 accumulations call ladd (ranks, rep_from_dl, headfail, verifier)"
else echo "sum sites: expected 4 ladd accumulations and 0 raw ones, found $nl and $raw"; fail=1; fi
for b in cc ua san; do
    for c in "summax|0|= LONG_MAX ok" "sumover|6|9223372036854775802 + 6 exceeds LONG_MAX" \
             "sumrun|6|8070450532247928832 + 1152921504606846976 exceeds LONG_MAX"; do
        t=${c%%|*}; r=${c#*|}; want=${r#*|}; r=${r%%|*}
        B 10 "$T/c_$b" -T $t > "$T/sum.$t.$b" 2>&1; rc=$?
        if [ $rc -eq $r ] && grep -q "$want" "$T/sum.$t.$b" && ! grep -q 'runtime error' "$T/sum.$t.$b"; then
            echo "sum self-test $t $b ok (rc $rc): $(tail -1 "$T/sum.$t.$b")"
        else echo "sum self-test $t $b FAILED (rc $rc, want $r): $(tail -1 "$T/sum.$t.$b")"; fail=1; fi
    done
    [ "$(grep -c '^sum self-test: run: [1-7] x' "$T/sum.sumrun.$b")" = 7 ] || { echo "sum self-test sumrun $b: the 7 terms below the limit did not all succeed"; fail=1; }
done
fi
for s in $(stages all); do
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
# one blob per tag of $TABLE, over the selected stages that carry it:
# prec+reloc (single), tyinfo (multi), regmap, then one per stage for the
# acceptance batch (single-stage packs; MAXS is 8)
for tag in $(echo "$TABLE" | awk '!s[$2]++{print $2}'); do
    f=$(echo "$TABLE" | while read -r n t x y; do [ $t = $tag ] && sel $n && printf " weights/gold/%s.tsv" $n; done)
    [ -n "$f" ] && uns2 $tag $f
done
# the multi-head branch trace (-t): which candidate each head chose, whether
# pick moved, what T4 did.  A debug print, not compared with Python (the
# counts were cross-checked once by hand); the two builds must agree.
for s in $(stages t); do
    for b in cc ua; do B 30 "$T/c_$b" -t weights/gold/$s.tsv > "$T/t.$b" 2>&1 || fail=1; done
    if cmp -s "$T/t.cc" "$T/t.ua"; then sed "s/^/$s /" "$T/t.cc"; else echo "$s trace differs between builds"; fail=1; fi
done
if [ $GL = 1 ]; then
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
# rules; this label needs 15.  The 63-rule table is the rule positive below.)
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
# rule-capacity POSITIVE: 9 x 7, label class[(7a+b) % 16]: 63 decision-list
# rules, over the old MAXR 62, inside MAXR 80.  Must build, and match Python
# (tsvgold on the same TSV): -d, UNS2 blob, deployed round trip.
# rule-capacity NEGATIVE: 9 x 9, same label: 81 rules (Python's count), one
# past MAXR 80; decision_list's nr >= MAXR check must exit exactly 4.
awk -F'	' -v A=9 -v N=7 'NR==1{print "# stage capr: 63 rules, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<N;i++) printf "\tw%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<A;a++) for(b=0;b<N;b++) print "v" a "\tw" b "\t" c[(7*a+b)%16]; exit}' "$T/cap.src" > "$T/capr.tsv"
awk -F'	' -v A=9 -v N=9 'NR==1{print "# stage caprn: 81 rules, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<N;i++) printf "\tw%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<A;a++) for(b=0;b<N;b++) print "v" a "\tw" b "\t" c[(7*a+b)%16]; exit}' "$T/cap.src" > "$T/caprn.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capr.tsv" > "$T/capr.py" || fail=1
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.capr.uns2" "$T/capr.tsv" > /dev/null || fail=1
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/capr.tsv" > "$T/capr.$b.out" 2>&1; rc=$?
    B 30 "$T/c_$b" -u "$T/$b.capr.uns2" "$T/capr.tsv" > /dev/null 2>> "$T/capr.$b.out"; rc2=$?
    nrl=$(grep -c '^rule' "$T/capr.$b.out")
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && [ "$nrl" = 63 ] && cmp -s "$T/capr.py" "$T/capr.$b.out" && cmp -s "$T/py.capr.uns2" "$T/$b.capr.uns2"; then
        echo "rule positive $b: 63 rules built, -d identical to Python ($(wc -c < "$T/capr.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.capr.uns2" | tr -d ' ') B)"
    else echo "rule positive $b FAILED (rc $rc/$rc2, $nrl rules): $(head -1 "$T/capr.$b.out")"; fail=1; fi
    if [ $b != san ]; then
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.capr.uns2" "$T/capr.tsv" > "$T/r.$b.out" 2>&1 || fail=1
        sed "s/^/rule positive $b deployed /" "$T/r.$b.out"
    fi
    B 30 "$T/c_$b" "$T/caprn.tsv" > "$T/caprn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y needs more than 80 decision-list rules" "$T/caprn.$b.out" && ! grep -q 'runtime error' "$T/caprn.$b.out"; then
        echo "rule negative $b rejected: $(head -1 "$T/caprn.$b.out" | sed 's/.*: capacity/capacity/')"
    else echo "rule negative $b NOT A RULE-CAPACITY REJECTION (rc $rc): $(head -1 "$T/caprn.$b.out")"; fail=1; fi
done
# head-capacity POSITIVE: 4 x 3 keys, 6 heads (over the old MAXH 4); head h
# has k = 2 + h % 3 classes, label (a(h+1) + b(h+2) + abh) % k.  Must build
# and match Python: -d, UNS2 blob, deployed round trip; the -t trace must
# agree between cc and unisacc.  NEGATIVE: 2 x 2 keys, 17 heads: the reader
# must exit exactly 4 at the 17th #head line.
genh() {   # genh A B heads name
awk -v A=$1 -v N=$2 -v NH=$3 -v NM=$4 'BEGIN{
  print "# stage " NM ": " NH " heads, synthetic"
  printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n"
  printf "#field\tb"; for(i=0;i<N;i++) printf "\tw%d", i; printf "\n"
  for(h=0;h<NH;h++){ k=2+h%3; printf "#head\ty%d\t-", h; for(c=0;c<k;c++) printf "\tc%d", c; printf "\n" }
  printf "a\tb"; for(h=0;h<NH;h++) printf "\t=> y%d", h; printf "\n"
  for(a=0;a<A;a++) for(b=0;b<N;b++){ printf "v%d\tw%d", a, b; for(h=0;h<NH;h++){ k=2+h%3; printf "\tc%d", (a*(h+1)+b*(h+2)+a*b*h)%k } printf "\n" }
}'
}
genh 4 3 6 caph > "$T/caph.tsv"
genh 2 2 17 caphn > "$T/caphn.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/caph.tsv" > "$T/caph.py" || fail=1
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.caph.uns2" "$T/caph.tsv" > /dev/null || fail=1
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/caph.tsv" > "$T/caph.$b.out" 2>&1; rc=$?
    B 30 "$T/c_$b" -u "$T/$b.caph.uns2" "$T/caph.tsv" > /dev/null 2>> "$T/caph.$b.out"; rc2=$?
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && cmp -s "$T/caph.py" "$T/caph.$b.out" && cmp -s "$T/py.caph.uns2" "$T/$b.caph.uns2"; then
        echo "head positive $b: 6 heads built, -d identical to Python ($(wc -c < "$T/caph.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.caph.uns2" | tr -d ' ') B)"
    else echo "head positive $b FAILED (rc $rc/$rc2): $(head -1 "$T/caph.$b.out")"; fail=1; fi
    if [ $b != san ]; then
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.caph.uns2" "$T/caph.tsv" > "$T/r.$b.out" 2>&1 || fail=1
        sed "s/^/head positive $b deployed /" "$T/r.$b.out"
        B 30 "$T/c_$b" -t "$T/caph.tsv" > "$T/caph.$b.t" 2>&1 || fail=1
    fi
    B 30 "$T/c_$b" "$T/caphn.tsv" > "$T/caphn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y16 is head 17, more than 16" "$T/caphn.$b.out" && ! grep -q 'runtime error' "$T/caphn.$b.out"; then
        echo "head negative $b rejected: $(head -1 "$T/caphn.$b.out" | sed 's/.*: capacity/capacity/')"
    else echo "head negative $b NOT A HEAD-CAPACITY REJECTION (rc $rc): $(head -1 "$T/caphn.$b.out")"; fail=1; fi
done
if cmp -s "$T/caph.cc.t" "$T/caph.ua.t"; then sed "s/^/caph /" "$T/caph.cc.t"; else echo "caph trace differs between builds"; fail=1; fi
# candidate capacity: regmap with its one head copied N times (heads y0..).
# Every head keeps its dlist slot and 18 factored candidates, and T4 appends
# 18 more per head per accepted list, all in the global slot count ncand.
# POSITIVE N = 3: 3 + 216 = 219 slots (> the old MAXCAND 96): -d identical to
# Python; multi-head factored (pick moves heads).  NEGATIVE N = 4: factored()
# reaches slot 289 > MAXCAND 288 and must exit exactly 4.
capk() {   # capk N name
awk -F'	' -v NH=$1 -v NM=$2 'NR==1{print "# stage " NM ": regmap head x" NH ", synthetic"; next}
/^#head/{for(h=0;h<NH;h++){ printf "#head\ty%d", h; for(i=3;i<=NF;i++) printf "\t%s", $i; printf "\n" } next}
/^treg/{printf "treg\tarch"; for(h=0;h<NH;h++) printf "\t=> y%d", h; printf "\n"; next}
/^#/{print; next}
{printf "%s\t%s", $1, $2; for(h=0;h<NH;h++) printf "\t%s", $3; printf "\n"}' "$T/cap.src"
}
capk 3 capk > "$T/capk.tsv"
capk 4 capkn > "$T/capkn.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capk.tsv" > "$T/capk.py" || fail=1
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/capk.tsv" > "$T/capk.$b.out" 2>&1; rc=$?
    if [ $rc -eq 0 ] && cmp -s "$T/capk.py" "$T/capk.$b.out"; then
        echo "candidate positive $b: 3 heads, 219 candidate slots, -d identical to Python ($(wc -c < "$T/capk.py" | tr -d ' ') B)"
    else echo "candidate positive $b FAILED (rc $rc): $(head -1 "$T/capk.$b.out")"; fail=1; fi
    B 30 "$T/c_$b" "$T/capkn.tsv" > "$T/capkn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y3: candidate slot 289 (head's 70), more than 288" "$T/capkn.$b.out" && ! grep -q 'runtime error' "$T/capkn.$b.out"; then
        echo "candidate negative $b rejected: $(head -1 "$T/capkn.$b.out" | sed 's/.*: capacity/capacity/')"
    else echo "candidate negative $b NOT A CANDIDATE-CAPACITY REJECTION (rc $rc): $(head -1 "$T/capkn.$b.out")"; fail=1; fi
done
# capacity NEGATIVE: more quotient keys than MAXQ (3200) must be rejected
# before any set is built: exit exactly 4 and the capacity diagnostic.
# Fields of 15, 15 and 15 values, label class[(a + 3b + 5c) % 16]: a shift
# of one field's value by d changes the label by d, 3d or 5d mod 16, never
# 0 for d <= 14, so no two values share a slice: 15^3 = 3375 quotient keys.
# 3375 raw keys <= MAXOK 4352 and 15 groups <= 62, so the reader and the
# field-group check pass and domain()'s quotient-key check is the one reached.
awk -F'	' 'NR==1{print "# stage capn: 3375 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<15;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<15;i++) printf "\tw%d", i
  printf "\n#field\tc"; for(i=0;i<15;i++) printf "\tx%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\tc\t=> y"
  for(a=0;a<15;a++) for(b=0;b<15;b++) for(x=0;x<15;x++) print "v" a "\tw" b "\tx" x "\t" c[(a+3*b+5*x)%16]; exit}' "$T/cap.src" > "$T/capn.tsv"
for b in cc ua san; do
    B 30 "$T/c_$b" "$T/capn.tsv" > "$T/capn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: 3375 quotient keys so far, more than 3200" "$T/capn.$b.out" && ! grep -q 'runtime error' "$T/capn.$b.out"; then
        echo "capacity negative $b rejected: $(head -1 "$T/capn.$b.out")"
    else
        echo "capacity negative $b NOT A CAPACITY REJECTION (rc $rc): $(head -1 "$T/capn.$b.out")"; fail=1
    fi
done
# raw-key NEGATIVE: more raw keys than MAXOK (4352) must be rejected by the
# reader at the header, before oseen/olab are indexed: exit exactly 4 and the
# raw-key diagnostic.  Fields of 67 and 66 values (<= MAXV 128): 4422 keys.
awk -F'	' 'NR==1{print "# stage capo: 4422 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<67;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<66;i++) printf "\tw%d", i
  printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<67;a++) for(b=0;b<66;b++) print "v" a "\tw" b "\t" c[(a+b)%16]; exit}' "$T/cap.src" > "$T/capo.tsv"
for b in cc ua san; do
    B 30 "$T/c_$b" "$T/capo.tsv" > "$T/capo.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: 4422 raw keys so far, more than 4352" "$T/capo.$b.out" && ! grep -q 'runtime error' "$T/capo.$b.out"; then
        echo "raw-key negative $b rejected: $(head -1 "$T/capo.$b.out")"
    else
        echo "raw-key negative $b NOT A RAW-KEY REJECTION (rc $rc): $(head -1 "$T/capo.$b.out")"; fail=1
    fi
done
# field-group NEGATIVE: a field's group set is one long, so a field may have
# at most 62 value groups.  Raw values are legal (70 <= MAXV 128) and there
# are 140 <= 3200 quotient keys, but field a has 70 groups: exit exactly 4
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
# class-count NEGATIVE: a class set is one long, so a head may have at most
# MAXC = 62 classes.  A 2 x 2 table whose head lists 63 classes (labels use
# only c0 and c1): raw values 2 and 2 (<= MAXV), 2 groups per field, 4
# quotient keys, 2 rules -- every other limit is met, and the reader must
# exit exactly 4 with the class diagnostic before any class shift.
{ echo "# stage capc: 4 keys, 63 classes, synthetic"; printf '#field\ta\tv0\tv1\n#field\tb\tw0\tw1\n#head\ty\t-'
  i=0; while [ $i -lt 63 ]; do printf '\tc%d' $i; i=$((i + 1)); done
  printf '\na\tb\t=> y\nv0\tw0\tc0\nv0\tw1\tc0\nv1\tw0\tc1\nv1\tw1\tc1\n'; } > "$T/capc.tsv"
for b in cc ua san; do
    B 30 "$T/c_$b" "$T/capc.tsv" > "$T/capc.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y has 63 classes, more than 62" "$T/capc.$b.out" && ! grep -q 'runtime error' "$T/capc.$b.out"; then
        echo "class-count negative $b rejected: $(head -1 "$T/capc.$b.out")"
    else
        echo "class-count negative $b NOT A CLASS-COUNT REJECTION (rc $rc): $(head -1 "$T/capc.$b.out")"; fail=1
    fi
done
# the deployment invariants must be able to fire: the test entry breaks b1
# of unit 0 (-T bias), or breaks it and skips invariant 2 (-T act) so that
# invariant 1 is the one reached.  Only exit 3 with that invariant's
# diagnostic counts.
fi
for s in $(stages i); do
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
# summary: exactly what this run covered
echo "summary: builds cc, ua, ubsan (always)"
echo "summary: stages run ($(stages all | wc -l | tr -d " ") of $(echo $ALL | wc -w | tr -d " ")): $(stages all | tr "\n" " ")"
for s in $(stages all); do
    neg=none; stages i | grep -qx $s && neg="invariant bias+act"
    tr=; stages t | grep -qx $s && tr=", trace"
    echo "summary: stage $s: positive (dump, ubsan, uns2$tr) yes; negative $neg"
done
skipped=$(for s in $ALL; do sel $s || printf "%s " $s; done)
echo "summary: stages skipped: ${skipped:-none}"
if [ $GL = 1 ]; then echo "summary: global checks run: $GLOBALS"; echo "summary: global checks skipped: none"
else echo "summary: global checks run: none"; echo "summary: global checks skipped: $GLOBALS"; fi
rm -rf "$T"
[ $fail = 0 ] && echo "construct check: ok" || echo "construct check: FAILED"
exit $fail
