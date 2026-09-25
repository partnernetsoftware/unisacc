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
abi abi t i
irsel irsel t i
isel isel t i'
ALL=$(echo "$TABLE" | cut -d" " -f1 | tr "\n" " " | sed "s/ $//")
[ -n "$ALL" ] || { echo "construct check: stage table is empty"; exit 2; }
GLOBALS="qset wset sum reader capq capr capr81 caprn caph caphn capk capkn capn capo capg capgn capc capcn capcf"
# batch mode: each batch is "stages|global"; the union is checked below
# type alone is ~28 s (its trace/invariants dominate), so it gets its own batch
BATCHES='prec reloc tyinfo regmap pp lex scope|1
pfconv binsel enc opinfo peep parse irsel isel|0
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
    echo "batches: planned union = all $(echo $ALL | wc -w | tr -d ' ') stages + global checks, each exactly once"
    # The plan above is not the result.  Each batch must hand back a receipt
    # ("receipt stage <s>" / "receipt global <g>") for exactly what it was
    # asked to check -- nothing missing, nothing extra, no duplicates -- and
    # the receipts of all batches together must be every stage of TABLE and
    # every global check, each exactly once.  rc 0 without them is a failure.
    W=${TMPDIR:-/tmp}/construct_batches.$$; mkdir -p "$W"
    bf=0; n=0; : > "$W/union"
    echo "$BATCHES" > "$W/list"
    while IFS='|' read -r st gl; do
        n=$((n + 1))
        t0=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
        STAGES=$st GLOBAL=$gl perl -e 'alarm 60; exec @ARGV' "$0" "$UA" > "$W/out.$n" 2>&1; rc=$?
        t1=$(perl -MTime::HiRes=time -e 'printf "%.3f", time')
        sed "s/^/[batch $n] /" "$W/out.$n"
        el=$(perl -e "printf '%.1f', $t1 - $t0")
        echo "batch $n: stages [$st] global $gl: rc $rc, $el s (limit 60 s)"
        if [ $rc -ne 0 ]; then bf=1; echo "batch $n: FAILED (rc $rc)"; break; fi
        { for s in $st; do echo "receipt stage $s"; done
          [ "$gl" = 1 ] && for g in $GLOBALS; do echo "receipt global $g"; done; } | sort > "$W/want.$n"
        grep '^receipt ' "$W/out.$n" | sort > "$W/got.$n"
        cat "$W/got.$n" >> "$W/union"
        mis=$(comm -23 "$W/want.$n" "$W/got.$n" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        ext=$(comm -13 "$W/want.$n" "$W/got.$n" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        dup=$(uniq -d "$W/got.$n" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        if [ -n "$mis$ext$dup" ]; then bf=1
            echo "batch $n: RECEIPTS WRONG (rc $rc): missing [${mis}] extra [${ext}] duplicate [${dup}]"; break
        fi
        echo "batch $n: receipts $(wc -l < "$W/got.$n" | tr -d ' ') = requested, exactly"
    done < "$W/list"
    if [ $bf = 0 ]; then
        { for s in $ALL; do echo "receipt stage $s"; done; for g in $GLOBALS; do echo "receipt global $g"; done; } | sort > "$W/wantall"
        sort "$W/union" > "$W/gotall"
        mis=$(comm -23 "$W/wantall" "$W/gotall" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        ext=$(comm -13 "$W/wantall" "$W/gotall" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        dup=$(uniq -d "$W/gotall" | sed 's/^receipt //' | tr '\n' ',' | sed 's/,$//')
        if [ -n "$mis$ext$dup" ]; then bf=1; echo "batches: receipt union WRONG: missing [${mis}] extra [${ext}] duplicate [${dup}]"
        else echo "batches: receipt union = all $(echo $ALL | wc -w | tr -d ' ') stages + $(echo $GLOBALS | wc -w | tr -d ' ') global checks, each exactly once ($(wc -l < "$W/gotall" | tr -d ' ') receipts)"; fi
    fi
    rm -rf "$W"
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
# RECEIPTS.  Every individual check appends a pass mark to $PASS at the point
# where it has succeeded (P), never otherwise.  need() is the list of marks
# each stage / global check requires; an item gets its receipt only when all
# of them are present.  So a check that is skipped, removed, or fails leaves
# its item with no receipt, whatever the exit status of anything else.
PASS=$T/pass; : > "$PASS"
P() { echo "$1" >> "$PASS"; }
need() {    # need stage <s> | need global <g>: the required pass marks
    if [ $1 = stage ]; then s=$2
        for b in cc ua; do echo "s $s dump $b"; echo "s $s uns2 $b"; echo "s $s shipped $b"; echo "s $s round $b"; done
        echo "s $s ubsan"
        echo "$TABLE" | while read -r n tag tr iv; do [ $n = $s ] || continue
            [ $tr = t ] && echo "s $s trace"
            [ $iv = i ] && for t in bias act; do for b in cc ua; do echo "s $s inv $t $b"; done; done; done
        return
    fi
    g=$2
    case $g in
    qset) for b in cc ua san; do echo "g qset $b"; done ;;
    wset) for b in cc ua san; do echo "g wset $b"; done ;;
    sum) echo "g sum sites"; for b in cc ua san; do for t in summax sumover sumrun run7; do echo "g sum $t $b"; done; done ;;
    reader) for n in missing duplicate badlabel extracol badkey; do for b in cc ua; do echo "g reader $n $b"; done; done ;;
    capq) for b in cc ua; do echo "g capq dump $b"; echo "g capq uns2 $b"; echo "g capq round $b"; done ;;
    capr|capr81) for b in cc ua san; do echo "g $g pos $b"; done; echo "g $g round cc"; echo "g $g round ua" ;;
    caph) for b in cc ua san; do echo "g caph pos $b"; done; echo "g caph round cc"; echo "g caph round ua"; echo "g caph trace" ;;
    capg) for b in cc ua san; do echo "g capg pos $b"; done; echo "g capg round cc"; echo "g capg round ua" ;;
    capc) for b in cc ua san; do echo "g capc pos $b"; done; echo "g capc round cc"; echo "g capc round ua" ;;
    capcf) for b in cc ua san; do echo "g capcf pos $b"; done; echo "g capcf round cc"; echo "g capcf round ua"; echo "g capcf trace" ;;
    *) for b in cc ua san; do echo "g $g $b"; done ;;
    esac
}
missing() { need "$@" | while read -r m; do grep -qxF "$m" "$PASS" || printf "[%s] " "$m"; done; }
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
    if [ $rc -eq 0 ] && [ "$(grep -c ', ok$' "$T/q.$b")" = 13 ] && ! grep -q 'runtime error' "$T/q.$b"; then echo "qset self-test $b ok (13 sizes)"; P "g qset $b"
    else echo "qset self-test $b FAILED (rc $rc): $(tail -1 "$T/q.$b")"; fail=1; fi
done
# the ws_ word-set layer (-G): domain sizes 0, 1, 61, 62, 63, 96, 123, 124
# in 3 storage words, junk-filled operands, unused words zeroed, tails,
# boundary members, empty/full, andnot against raw all-ones, every word in
# [0, 2^62), and ws_cmp against a member-list lexicographic oracle (fixed
# cases incl. {0,62} vs {1} and {3,70} vs {70}, plus pseudo-random pairs).
# The class half: rankset over 96 classes (2 words) under a reversed and a
# scrambled name order, cross-word members and pairs vs the oracle.
# The three builds must print the same 9 "ok" lines.
for b in cc ua san; do
    B 30 "$T/c_$b" -G > "$T/g.$b" 2>&1; rc=$?
    if [ $rc -eq 0 ] && [ "$(grep -c ', ok$' "$T/g.$b")" = 9 ] && ! grep -q 'runtime error' "$T/g.$b" && cmp -s "$T/g.cc" "$T/g.$b"; then echo "wset self-test $b ok (8 sizes + rankset, $(grep -v rankset "$T/g.$b" | awk '{s += $(NF-4)} END {print s}') comparisons vs oracle; $(grep rankset "$T/g.$b" | sed 's/.*: //')"; P "g wset $b"
    else echo "wset self-test $b FAILED (rc $rc): $(tail -1 "$T/g.$b")"; fail=1; fi
done
# checked accumulation: -T summax/sumover/sumrun drive the SAME ladd() that
# every weight/logit sum calls.  max must succeed (exit 0); one past LONG_MAX
# and a running sum of 2^60 crossing it must be rejected by the guard (exit
# exactly 6, the overflow diagnostic), never by a UBSan report or a signal.
# The static half: the four accumulation sites are ladd calls (README).
nl=$(grep -cE '(z\[c\]|cw\[ci \* MAXU \+ found\]\[rl\[r\]\]) = ladd\(' iterate/construct/construct.c)
raw=$(grep -cE 'z\[c\] = z\[c\] \+|= cw\[.*\] \+' iterate/construct/construct.c)
if [ "$nl" = 4 ] && [ "$raw" = 0 ]; then echo "sum sites: 4 accumulations call ladd (ranks, rep_from_dl, headfail, verifier)"; P "g sum sites"
else echo "sum sites: expected 4 ladd accumulations and 0 raw ones, found $nl and $raw"; fail=1; fi
for b in cc ua san; do
    for c in "summax|0|= LONG_MAX ok" "sumover|6|9223372036854775802 + 6 exceeds LONG_MAX" \
             "sumrun|6|8070450532247928832 + 1152921504606846976 exceeds LONG_MAX"; do
        t=${c%%|*}; r=${c#*|}; want=${r#*|}; r=${r%%|*}
        B 10 "$T/c_$b" -T $t > "$T/sum.$t.$b" 2>&1; rc=$?
        if [ $rc -eq $r ] && grep -q "$want" "$T/sum.$t.$b" && ! grep -q 'runtime error' "$T/sum.$t.$b"; then
            echo "sum self-test $t $b ok (rc $rc): $(tail -1 "$T/sum.$t.$b")"; P "g sum $t $b"
        else echo "sum self-test $t $b FAILED (rc $rc, want $r): $(tail -1 "$T/sum.$t.$b")"; fail=1; fi
    done
    [ "$(grep -c '^sum self-test: run: [1-7] x' "$T/sum.sumrun.$b")" = 7 ] && P "g sum run7 $b" || { echo "sum self-test sumrun $b: the 7 terms below the limit did not all succeed"; fail=1; }
done
fi
for s in $(stages all); do
    B 60 python3 iterate/construct/tools/netdump.py -d "weights/gold/$s.tsv" > "$T/$s.py"; prc=$?; [ $prc -eq 0 ] || fail=1
    for b in cc ua; do
        B 30 "$T/c_$b" -d "weights/gold/$s.tsv" > "$T/$s.$b"; rc=$?; [ $rc -eq 0 ] || fail=1
        if [ $prc -eq 0 ] && [ $rc -eq 0 ] && cmp -s "$T/$s.py" "$T/$s.$b"; then echo "$s $b identical ($(wc -c < "$T/$s.py" | tr -d ' ') B; raw keys $(sed -n 's/^exact //p' "$T/$s.py"), quotient keys $(grep '^groups' "$T/$s.py" | awk '{n = gsub(/\[/, "["); q = (NR == 1 ? n : q * n)} END {print q}'))"; P "s $s dump $b"
        else echo "$s $b DIFFERS"; diff "$T/$s.py" "$T/$s.$b" | head -5; fail=1; fi
    done
    # every stage's dump and UNS2 under UBSan: exit 0, no report, same bytes
    B 30 "$T/c_san" -d "weights/gold/$s.tsv" > "$T/$s.san" 2> "$T/$s.san.err"; rc=$?
    B 30 "$T/c_san" -u "$T/$s.san.uns2" "weights/gold/$s.tsv" > /dev/null 2>> "$T/$s.san.err"; rc2=$?
    B 30 "$T/c_cc" -u "$T/$s.cc.uns2" "weights/gold/$s.tsv" > /dev/null 2>&1
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && [ ! -s "$T/$s.san.err" ] && cmp -s "$T/$s.py" "$T/$s.san" && cmp -s "$T/$s.cc.uns2" "$T/$s.san.uns2"; then echo "$s ubsan clean (dump, UNS2)"; P "s $s ubsan"
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
PU() { for x in $us; do P "s $x $1"; done; }
uns2() {    # uns2 "<stages>" <tag> <tsv>...: the marks go to every stage in the blob
    us=$1; tag=$2; shift 2
    B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.$tag.uns2" "$@" > /dev/null || { echo "uns2 $tag python reference failed"; fail=1; return; }
    for b in cc ua; do
        B 30 "$T/c_$b" -u "$T/$b.$tag.uns2" "$@" > "$T/u.$b.out" 2>&1; rc=$?
        if [ $rc -ne 0 ]; then echo "uns2 $tag $b construct failed (rc $rc): $(head -1 "$T/u.$b.out")"; fail=1; continue; fi
        echo "invariants $tag $b ok (activation 0/1, b1 = 1 - constrained fields; full domain)"
        if cmp -s "$T/py.$tag.uns2" "$T/$b.$tag.uns2"; then echo "uns2 $tag $b identical to uns2.dump ($(wc -c < "$T/py.$tag.uns2" | tr -d ' ') B)"; PU "uns2 $b"
        else echo "uns2 $tag $b DIFFERS from uns2.dump"; cmp "$T/py.$tag.uns2" "$T/$b.$tag.uns2" | head -2; fail=1; fi
        B 60 python3 iterate/construct/tools/uns2slice.py --shipped "$T/$b.$tag.uns2" weights/built.uns2 > "$T/s.$b.out" 2>&1 && PU "shipped $b" || fail=1
        sed "s/^/uns2 $b shipped /" "$T/s.$b.out"
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.$tag.uns2" "$@" > "$T/r.$b.out" 2>&1 && PU "round $b" || fail=1
        sed "s/^/deployed $b /" "$T/r.$b.out"
    done
}
# one blob per tag of $TABLE, over the selected stages that carry it:
# prec+reloc (single), tyinfo (multi), regmap, then one per stage for the
# acceptance batch (single-stage packs; MAXS is 8)
for tag in $(echo "$TABLE" | awk '!s[$2]++{print $2}'); do
    ss=$(echo "$TABLE" | while read -r n t x y; do [ $t = $tag ] && sel $n && printf " %s" $n; done)
    f=$(for n in $ss; do printf " weights/gold/%s.tsv" $n; done)
    [ -n "$f" ] && uns2 "$ss" $tag $f
done
# the multi-head branch trace (-t): which candidate each head chose, whether
# pick moved, what T4 did.  A debug print, not compared with Python (the
# counts were cross-checked once by hand); the two builds must agree.
for s in $(stages t); do
    tf=0; for b in cc ua; do B 30 "$T/c_$b" -t weights/gold/$s.tsv > "$T/t.$b" 2>&1 || { fail=1; tf=1; }; done
    if cmp -s "$T/t.cc" "$T/t.ua"; then sed "s/^/$s /" "$T/t.cc"; [ $tf = 0 ] && P "s $s trace"; else echo "$s trace differs between builds"; fail=1; fi
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
            echo "$n $b rejected: $(head -1 "$T/$n.$b.out")"; P "g reader $n $b"
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
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capq.tsv" > "$T/capq.py" || { fail=1; echo "capq python reference failed" > "$T/capq.py"; }
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.capq.uns2" "$T/capq.tsv" > /dev/null || { fail=1; echo "capq python reference failed" > "$T/py.capq.uns2"; }
for b in cc ua; do
    B 30 "$T/c_$b" -d "$T/capq.tsv" > "$T/capq.$b.out" 2>&1; rc=$?
    # 9 + 7 singleton groups, i.e. 9 x 7 = 63 quotient keys, none merged
    grp=$(grep '^groups' "$T/capq.$b.out" | tr -cd '[' | wc -c | tr -d ' ')
    if [ $rc -eq 0 ] && [ "$grp" = 16 ] && grep -q "^exact 63$" "$T/capq.$b.out" && cmp -s "$T/capq.py" "$T/capq.$b.out"; then
        echo "capacity positive $b: 63 quotient keys built, exact over 63 keys, -d identical to Python ($(wc -c < "$T/capq.py" | tr -d ' ') B)"; P "g capq dump $b"
    else
        echo "capacity positive $b FAILED (rc $rc): $(head -1 "$T/capq.$b.out")"; fail=1
    fi
    B 30 "$T/c_$b" -u "$T/$b.capq.uns2" "$T/capq.tsv" > /dev/null 2>&1; rc=$?; [ $rc -eq 0 ] || fail=1
    if [ $rc -eq 0 ] && cmp -s "$T/py.capq.uns2" "$T/$b.capq.uns2"; then echo "capacity positive $b: UNS2 identical ($(wc -c < "$T/py.capq.uns2" | tr -d ' ') B)"; P "g capq uns2 $b"
    else echo "capacity positive $b: UNS2 DIFFERS"; fail=1; fi
    B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.capq.uns2" "$T/capq.tsv" > "$T/r.$b.out" 2>&1 && P "g capq round $b" || fail=1
    sed "s/^/capacity positive $b deployed /" "$T/r.$b.out"
done
# rule-capacity POSITIVES, label class[(7a+b) % 16] on a x b keys: capr is
# 9 x 7, 63 decision-list rules (over the old MAXR 62); capr81 is 9 x 9, 81
# rules (the old negative, one past the old MAXR 80), inside MAXR 96.  Both
# must build, and match Python (tsvgold on the same TSV): -d, UNS2 blob,
# deployed round trip.
# rule-capacity NEGATIVE: 10 x 10, same label: 100 rules (Python's count),
# past MAXR 96; decision_list's nr >= MAXR check must exit exactly 4.
genr() {   # genr A B name rules
awk -F'	' -v A=$1 -v N=$2 -v NM=$3 -v NR_=$4 'NR==1{print "# stage " NM ": " NR_ " rules, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<N;i++) printf "\tw%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<A;a++) for(b=0;b<N;b++) print "v" a "\tw" b "\t" c[(7*a+b)%16]; exit}' "$T/cap.src"
}
genr 9 7 capr 63 > "$T/capr.tsv"
genr 9 9 capr81 81 > "$T/capr81.tsv"
genr 10 10 caprn 100 > "$T/caprn.tsv"
for x in capr capr81; do
    B 60 python3 iterate/construct/tools/netdump.py -d "$T/$x.tsv" > "$T/$x.py" || { fail=1; echo "$x python reference failed" > "$T/$x.py"; }
    B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.$x.uns2" "$T/$x.tsv" > /dev/null || { fail=1; echo "$x python reference failed" > "$T/py.$x.uns2"; }
done
for b in cc ua san; do
    for xr in capr:63 capr81:81; do x=${xr%%:*}; want=${xr#*:}
    B 30 "$T/c_$b" -d "$T/$x.tsv" > "$T/$x.$b.out" 2>&1; rc=$?
    B 30 "$T/c_$b" -u "$T/$b.$x.uns2" "$T/$x.tsv" > /dev/null 2>> "$T/$x.$b.out"; rc2=$?
    nrl=$(grep -c '^rule' "$T/$x.$b.out")
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && [ "$nrl" = $want ] && cmp -s "$T/$x.py" "$T/$x.$b.out" && cmp -s "$T/py.$x.uns2" "$T/$b.$x.uns2"; then
        echo "rule positive $x $b: $want rules built, -d identical to Python ($(wc -c < "$T/$x.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.$x.uns2" | tr -d ' ') B)"; P "g $x pos $b"
    else echo "rule positive $x $b FAILED (rc $rc/$rc2, $nrl rules): $(head -1 "$T/$x.$b.out")"; fail=1; fi
    if [ $b != san ]; then
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.$x.uns2" "$T/$x.tsv" > "$T/r.$b.out" 2>&1 && P "g $x round $b" || fail=1
        sed "s/^/rule positive $x $b deployed /" "$T/r.$b.out"
    fi
    done
    B 30 "$T/c_$b" "$T/caprn.tsv" > "$T/caprn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y needs more than 96 decision-list rules" "$T/caprn.$b.out" && ! grep -q 'runtime error' "$T/caprn.$b.out"; then
        echo "rule negative $b rejected: $(head -1 "$T/caprn.$b.out" | sed 's/.*: capacity/capacity/')"; P "g caprn $b"
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
genh 4 3 6 caph > "$T/caph.tsv"; htf=0
genh 2 2 17 caphn > "$T/caphn.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/caph.tsv" > "$T/caph.py" || { fail=1; echo "caph python reference failed" > "$T/caph.py"; }
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.caph.uns2" "$T/caph.tsv" > /dev/null || { fail=1; echo "caph python reference failed" > "$T/py.caph.uns2"; }
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/caph.tsv" > "$T/caph.$b.out" 2>&1; rc=$?
    B 30 "$T/c_$b" -u "$T/$b.caph.uns2" "$T/caph.tsv" > /dev/null 2>> "$T/caph.$b.out"; rc2=$?
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && cmp -s "$T/caph.py" "$T/caph.$b.out" && cmp -s "$T/py.caph.uns2" "$T/$b.caph.uns2"; then
        echo "head positive $b: 6 heads built, -d identical to Python ($(wc -c < "$T/caph.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.caph.uns2" | tr -d ' ') B)"; P "g caph pos $b"
    else echo "head positive $b FAILED (rc $rc/$rc2): $(head -1 "$T/caph.$b.out")"; fail=1; fi
    if [ $b != san ]; then
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.caph.uns2" "$T/caph.tsv" > "$T/r.$b.out" 2>&1 && P "g caph round $b" || fail=1
        sed "s/^/head positive $b deployed /" "$T/r.$b.out"
        B 30 "$T/c_$b" -t "$T/caph.tsv" > "$T/caph.$b.t" 2>&1 || { fail=1; htf=1; }
    fi
    B 30 "$T/c_$b" "$T/caphn.tsv" > "$T/caphn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y16 is head 17, more than 16" "$T/caphn.$b.out" && ! grep -q 'runtime error' "$T/caphn.$b.out"; then
        echo "head negative $b rejected: $(head -1 "$T/caphn.$b.out" | sed 's/.*: capacity/capacity/')"; P "g caphn $b"
    else echo "head negative $b NOT A HEAD-CAPACITY REJECTION (rc $rc): $(head -1 "$T/caphn.$b.out")"; fail=1; fi
done
if cmp -s "$T/caph.cc.t" "$T/caph.ua.t"; then sed "s/^/caph /" "$T/caph.cc.t"; [ $htf = 0 ] && P "g caph trace"; else echo "caph trace differs between builds"; fail=1; fi
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
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capk.tsv" > "$T/capk.py" || { fail=1; echo "capk python reference failed" > "$T/capk.py"; }
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/capk.tsv" > "$T/capk.$b.out" 2>&1; rc=$?
    if [ $rc -eq 0 ] && cmp -s "$T/capk.py" "$T/capk.$b.out"; then
        echo "candidate positive $b: 3 heads, 219 candidate slots, -d identical to Python ($(wc -c < "$T/capk.py" | tr -d ' ') B)"; P "g capk $b"
    else echo "candidate positive $b FAILED (rc $rc): $(head -1 "$T/capk.$b.out")"; fail=1; fi
    B 30 "$T/c_$b" "$T/capkn.tsv" > "$T/capkn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y3: candidate slot 289 (head's 70), more than 288" "$T/capkn.$b.out" && ! grep -q 'runtime error' "$T/capkn.$b.out"; then
        echo "candidate negative $b rejected: $(head -1 "$T/capkn.$b.out" | sed 's/.*: capacity/capacity/')"; P "g capkn $b"
    else echo "candidate negative $b NOT A CANDIDATE-CAPACITY REJECTION (rc $rc): $(head -1 "$T/capkn.$b.out")"; fail=1; fi
done
# capacity NEGATIVE: more quotient keys than MAXQ (3200) must be rejected
# before any set is built: exit exactly 4 and the capacity diagnostic.
# Fields of 15, 15 and 15 values, label class[(a + 3b + 5c) % 16]: a shift
# of one field's value by d changes the label by d, 3d or 5d mod 16, never
# 0 for d <= 14, so no two values share a slice: 15^3 = 3375 quotient keys.
# 3375 raw keys <= MAXOK 4352 and 15 groups <= MAXG 124, so the reader and the
# field-group check pass and domain()'s quotient-key check is the one reached.
awk -F'	' 'NR==1{print "# stage capn: 3375 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<15;i++) printf "\tv%d", i; printf "\n#field\tb"; for(i=0;i<15;i++) printf "\tw%d", i
  printf "\n#field\tc"; for(i=0;i<15;i++) printf "\tx%d", i; printf "\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\tc\t=> y"
  for(a=0;a<15;a++) for(b=0;b<15;b++) for(x=0;x<15;x++) print "v" a "\tw" b "\tx" x "\t" c[(a+3*b+5*x)%16]; exit}' "$T/cap.src" > "$T/capn.tsv"
for b in cc ua san; do
    B 30 "$T/c_$b" "$T/capn.tsv" > "$T/capn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: 3375 quotient keys so far, more than 3200" "$T/capn.$b.out" && ! grep -q 'runtime error' "$T/capn.$b.out"; then
        echo "capacity negative $b rejected: $(head -1 "$T/capn.$b.out")"; P "g capn $b"
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
        echo "raw-key negative $b rejected: $(head -1 "$T/capo.$b.out")"; P "g capo $b"
    else
        echo "raw-key negative $b NOT A RAW-KEY REJECTION (rc $rc): $(head -1 "$T/capo.$b.out")"; fail=1
    fi
done
# field-group POSITIVE: field a has 70 value groups (the old negative:
# a group set was one long, at most 62).  Group sets are GW = 2 words now
# (MAXG 124), so it must BUILD, verify over its full domain (exit 0), and
# match Python on the same TSV: -d (which prints the 70 groups and every
# cube's group set, bits 62..69 included), UNS2 blob, deployed round trip.
# Label (a, b) = class[b ? 8 + a / 16 : a % 16] with 16 classes: the pair
# (label(a,0), label(a,1)) = (a % 16, 8 + a / 16) is distinct for every a.
# field-group NEGATIVE: the same label on 125 values of a (<= MAXV 128, 250
# raw and quotient keys, all inside their limits) gives 125 groups, one past
# MAXG: exit exactly 4 with the field-group diagnostic, from domain()
# before any field set is written.
geng() {   # geng A name
awk -F'	' -v A=$1 -v NM=$2 'NR==1{print "# stage " NM ": " A " x 2 keys, synthetic"; next}
/^#head/{printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n#field\tb\tw0\tw1\n"; print; for(i=4;i<=NF;i++) c[i-4]=$i; print "a\tb\t=> y"
  for(a=0;a<A;a++) for(b=0;b<2;b++) print "v" a "\tw" b "\t" c[b ? 8 + int(a/16) : a%16]; exit}' "$T/cap.src"
}
geng 70 capg > "$T/capg.tsv"
geng 125 capgn > "$T/capgn.tsv"
B 60 python3 iterate/construct/tools/netdump.py -d "$T/capg.tsv" > "$T/capg.py" || { fail=1; echo "capg python reference failed" > "$T/capg.py"; }
B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.capg.uns2" "$T/capg.tsv" > /dev/null || { fail=1; echo "capg python reference failed" > "$T/py.capg.uns2"; }
for b in cc ua san; do
    B 30 "$T/c_$b" -d "$T/capg.tsv" > "$T/capg.$b.out" 2>&1; rc=$?
    B 30 "$T/c_$b" -u "$T/$b.capg.uns2" "$T/capg.tsv" > /dev/null 2>> "$T/capg.$b.out"; rc2=$?
    grp=$(grep '^groups 0' "$T/capg.$b.out" | tr -cd '[' | wc -c | tr -d ' ')
    if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && [ "$grp" = 70 ] && grep -q "^exact 140$" "$T/capg.$b.out" && cmp -s "$T/capg.py" "$T/capg.$b.out" && cmp -s "$T/py.capg.uns2" "$T/$b.capg.uns2"; then
        echo "field-group positive $b: 70 groups built, exact over 140 keys, -d identical to Python ($(wc -c < "$T/capg.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.capg.uns2" | tr -d ' ') B)"; P "g capg pos $b"
    else echo "field-group positive $b FAILED (rc $rc/$rc2, $grp groups): $(head -1 "$T/capg.$b.out")"; fail=1; fi
    if [ $b != san ]; then
        B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.capg.uns2" "$T/capg.tsv" > "$T/r.$b.out" 2>&1 && P "g capg round $b" || fail=1
        sed "s/^/field-group positive $b deployed /" "$T/r.$b.out"
    fi
    B 30 "$T/c_$b" "$T/capgn.tsv" > "$T/capgn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: field a has 125 value groups, more than 124" "$T/capgn.$b.out" && ! grep -q 'runtime error' "$T/capgn.$b.out"; then
        echo "field-group negative $b rejected: $(head -1 "$T/capgn.$b.out")"; P "g capgn $b"
    else
        echo "field-group negative $b NOT A FIELD-GROUP REJECTION (rc $rc): $(head -1 "$T/capgn.$b.out")"; fail=1
    fi
done
# class-count POSITIVE: a 2 x 2 table whose head lists 63 classes (labels
# use c0 and c1) -- the old negative, when a class set was one long (62).
# Class sets are CW = 2 words now (MAXC 96): it must build and match Python
# (-d, UNS2, deployed round trip).  class-count NEGATIVE: the same table
# with 97 classes: every other limit is met, and the reader must exit
# exactly 4 with the class diagnostic before any class set is written.
genc() {   # genc classes name
{ echo "# stage $2: 4 keys, $1 classes, synthetic"; printf '#field\ta\tv0\tv1\n#field\tb\tw0\tw1\n#head\ty\t-'
  i=0; while [ $i -lt $1 ]; do printf '\tc%d' $i; i=$((i + 1)); done
  printf '\na\tb\t=> y\nv0\tw0\tc0\nv0\tw1\tc0\nv1\tw0\tc1\nv1\tw1\tc1\n'; }
}
genc 63 capc > "$T/capc.tsv"
genc 97 capcn > "$T/capcn.tsv"
# class-set SYNTHETIC POSITIVE (capcf): fields a (8 values) x b (9), 96
# classes named so that name order REVERSES index order (class c is
# "k<95-c>": high indices have low ranks), label of (a, b) = class
# (37(9a + b) + 5) % 96 -- 72 distinct classes, one per key.  The decision
# list needs 72 rules (<= MAXR 96); rep_factored's buckets (one per value
# of a, one per value of b, each a 8- or 9-class set) give a 17-unit
# factored candidate that is KEPT and CHOSEN.  Every bucket's class set,
# and its rank set, has members on both sides of bit 61/62, and the bucket
# order is decided by ws_cmp over the rank sets: comparing index sets,
# comparing the words as one integer, or not sorting all change the unit
# order, so the -d dump would differ from Python (measured once by mutation,
# README).  Must match Python byte for byte: -d, UNS2, round trip; the
# trace must show the factored candidate chosen.
awk 'BEGIN{A=8;N=9;K=96
 print "# stage capcf: factored, 96 classes, synthetic"
 printf "#field\ta"; for(i=0;i<A;i++) printf "\tv%d", i; printf "\n"
 printf "#field\tb"; for(i=0;i<N;i++) printf "\tw%d", i; printf "\n"
 printf "#head\ty\t-"; for(c=0;c<K;c++) printf "\tk%02d", K-1-c; printf "\n"
 print "a\tb\t=> y"
 for(a=0;a<A;a++) for(b=0;b<N;b++) printf "v%d\tw%d\tk%02d\n", a, b, K-1-((37*(9*a+b)+5)%96) }' > "$T/capcf.tsv"
for x in capc capcf; do
    B 60 python3 iterate/construct/tools/netdump.py -d "$T/$x.tsv" > "$T/$x.py" || { fail=1; echo "$x python reference failed" > "$T/$x.py"; }
    B 60 python3 iterate/construct/tools/uns2slice.py "$T/py.$x.uns2" "$T/$x.tsv" > /dev/null || { fail=1; echo "$x python reference failed" > "$T/py.$x.uns2"; }
done
cft=0
for b in cc ua san; do
    for x in capc capcf; do
        B 30 "$T/c_$b" -d "$T/$x.tsv" > "$T/$x.$b.out" 2>&1; rc=$?
        B 30 "$T/c_$b" -u "$T/$b.$x.uns2" "$T/$x.tsv" > /dev/null 2>> "$T/$x.$b.out"; rc2=$?
        if [ $rc -eq 0 ] && [ $rc2 -eq 0 ] && cmp -s "$T/$x.py" "$T/$x.$b.out" && cmp -s "$T/py.$x.uns2" "$T/$b.$x.uns2"; then
            echo "class positive $x $b: built, -d identical to Python ($(wc -c < "$T/$x.py" | tr -d ' ') B), UNS2 identical ($(wc -c < "$T/py.$x.uns2" | tr -d ' ') B)"; P "g $x pos $b"
        else echo "class positive $x $b FAILED (rc $rc/$rc2): $(head -1 "$T/$x.$b.out")"; fail=1; fi
        if [ $b != san ]; then
            B 60 python3 iterate/construct/tools/uns2round.py "$T/$b.$x.uns2" "$T/$x.tsv" > "$T/r.$b.out" 2>&1 && P "g $x round $b" || fail=1
            sed "s/^/class positive $b deployed /" "$T/r.$b.out"
        fi
    done
    [ $b != san ] && { B 30 "$T/c_$b" -t "$T/capcf.tsv" > "$T/capcf.$b.t" 2>&1 || cft=1; }
    B 30 "$T/c_$b" "$T/capcn.tsv" > "$T/capcn.$b.out" 2>&1; rc=$?
    if [ $rc -eq 4 ] && grep -q "capacity: head y has 97 classes, more than 96" "$T/capcn.$b.out" && ! grep -q 'runtime error' "$T/capcn.$b.out"; then
        echo "class-count negative $b rejected: $(head -1 "$T/capcn.$b.out")"; P "g capcn $b"
    else
        echo "class-count negative $b NOT A CLASS-COUNT REJECTION (rc $rc): $(head -1 "$T/capcn.$b.out")"; fail=1
    fi
done
if [ $cft = 0 ] && cmp -s "$T/capcf.cc.t" "$T/capcf.ua.t" && grep -q "^trace head y: 19 candidates, chose 1 (factored), 17 units$" "$T/capcf.cc.t" && grep -q "kept 18$" "$T/capcf.cc.t"; then
    sed "s/^/capcf /" "$T/capcf.cc.t"; P "g capcf trace"
else echo "capcf trace: not the factored choice, or differs between builds"; fail=1; fi
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
            echo "invariant negative $s $t $b fires: $(head -1 "$T/inv.$t.$b" | sed 's/.*broken: //')"; P "s $s inv $t $b"
        else
            echo "invariant negative $s $t $b DID NOT FIRE (rc $rc): $(head -1 "$T/inv.$t.$b")"; fail=1
        fi
    done
done
done
# summary: built from the pass marks (results), not from the selection.
# attempted = selected; passed = every required mark present (receipt);
# failed = attempted without a receipt; skipped = not selected.  An attempted
# item with no receipt fails the run even when no check set fail (a check
# that silently never ran).  The "receipt" lines, last, are what --batches
# reads: one per item that PASSED, nothing else.
echo "summary: builds cc, ua, ubsan (always)"
: > "$T/rcpt"; np=0; nf=0; ns=0
for s in $ALL; do
    if ! sel $s; then echo "summary: stage $s: skipped"; ns=$((ns + 1)); continue; fi
    neg=none; stages i | grep -qx $s && neg="invariant bias+act"
    tr=; stages t | grep -qx $s && tr=", trace"
    m=$(missing stage $s)
    if [ -z "$m" ]; then echo "receipt stage $s" >> "$T/rcpt"; np=$((np + 1))
        echo "summary: stage $s: attempted, passed (dump, ubsan, uns2, shipped, round trip$tr; negative $neg)"
    else nf=$((nf + 1)); echo "summary: stage $s: attempted, FAILED, no receipt; missing $m"; fi
done
for g in $GLOBALS; do
    if [ $GL = 0 ]; then echo "summary: global $g: skipped"; ns=$((ns + 1)); continue; fi
    m=$(missing global $g)
    if [ -z "$m" ]; then echo "receipt global $g" >> "$T/rcpt"; np=$((np + 1)); echo "summary: global $g: attempted, passed"
    else nf=$((nf + 1)); echo "summary: global $g: attempted, FAILED, no receipt; missing $m"; fi
done
echo "summary: attempted $((np + nf)), passed $np, failed $nf, skipped $ns"
[ $nf = 0 ] || fail=1    # self-check: an attempted item without a receipt fails this run too
cat "$T/rcpt"
rm -rf "$T"
[ $fail = 0 ] && echo "construct check: ok" || echo "construct check: FAILED"
exit $fail
