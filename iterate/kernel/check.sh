#!/bin/sh
# iterate/kernel/check.sh -- J10 step 3, first slice: genmodel.c writes the
# S_* defines, MODEL, DENSE and model_dims() of kernel/unisa_model.inc from
# declared inputs (order.tsv, weights/built.uns2, weights/gold/*.tsv).
# Run from the repo root.  Every step is bounded (alarm, <= 60 s).
#   iterate/kernel/check.sh [ua]              every check (ua: a unisacc binary, default /tmp/ua_ref)
#   CHECKS="region sem" iterate/kernel/check.sh [ua]   a subset
#   iterate/kernel/check.sh --batches [ua]    every check, in batches of <= 60 s
# Receipts: each check appends pass marks as its steps succeed; a selected
# check gets "receipt <check>" only when ALL of need(<check>) are present.
# A selected check without a receipt fails the run whatever else exited 0.
# SKIP=<check> (test entry) drops that check's body -- `fault` uses it to
# show a dropped check fails the run although nothing in it failed.
set -u
CHECKS_ALL="order build region empty sem oracle neg wfail label perm fault"
NEGS="notsv noorder nouns2 duporder missorder unkorder trunc trunchead trail badmagic dimval dimcls tsvtwice tsvunk hm15 hm17"
BATCHES='order build region empty sem
oracle
neg wfail label
perm fault'
need() {
    case $1 in
    order) echo "order gold"; echo "order source" ;;
    build) for b in cc ua san; do echo "build $b"; done ;;
    region) echo "region oracle=shipped"; for b in cc ua san; do echo "region $b"; done ;;
    empty) echo "empty source"; for b in cc ua; do echo "empty $b"; done ;;
    sem) for b in cc ua; do echo "sem $b"; done ;;
    oracle) echo "oracle build"; echo "oracle check" ;;
    neg) for n in $NEGS; do for b in cc ua san; do echo "neg $n $b"; done; done; echo "neg stride" ;;
    wfail) for t in opendir nodir rlimit; do for b in cc ua san; do echo "wfail $t $b"; done; done ;;
    label) echo "label differs"; echo "label dense-only"; for b in cc ua; do echo "label reject $b"; done ;;
    perm) echo "perm oracle"; echo "perm ids"; echo "perm sem" ;;
    fault) echo "fault dropped" ;;
    esac
}
B() { perl -e 'alarm shift; exec @ARGV' "$@"; }
now() { perl -MTime::HiRes=time -e 'printf "%.3f", time'; }
if [ "${1:-}" = --batches ]; then
    shift; UA=${1:-/tmp/ua_ref}
    plan=$(echo "$BATCHES" | tr ' ' '\n' | grep . | sort)
    full=$(echo $CHECKS_ALL | tr ' ' '\n' | sort)
    [ "$plan" = "$full" ] || { echo "batches: plan is not every check exactly once: [$(echo $plan)]"; exit 2; }
    W=${TMPDIR:-/tmp}/kernel_batches.$$; mkdir -p "$W"; : > "$W/union"; bf=0; n=0
    echo "$BATCHES" > "$W/list"
    while read -r cs; do
        n=$((n + 1)); t0=$(now)
        CHECKS=$cs B 60 sh "$0" "$UA" > "$W/out.$n" 2>&1; rc=$?
        el=$(perl -e "printf '%.1f', $(now) - $t0")
        sed "s/^/[batch $n] /" "$W/out.$n"
        echo "batch $n: [$cs]: rc $rc, $el s (limit 60 s)"
        [ $rc -eq 0 ] || { bf=1; echo "batch $n: FAILED (rc $rc)"; break; }
        for c in $cs; do echo "receipt $c"; done | sort > "$W/want.$n"
        grep '^receipt ' "$W/out.$n" | sort > "$W/got.$n"
        if ! cmp -s "$W/want.$n" "$W/got.$n"; then bf=1; echo "batch $n: RECEIPTS WRONG: want [$(cat "$W/want.$n" | tr '\n' ',')] got [$(cat "$W/got.$n" | tr '\n' ',')]"; break; fi
        cat "$W/got.$n" >> "$W/union"
    done < "$W/list"
    if [ $bf = 0 ]; then
        for c in $CHECKS_ALL; do echo "receipt $c"; done | sort > "$W/wantall"
        sort "$W/union" > "$W/gotall"
        if cmp -s "$W/wantall" "$W/gotall"; then echo "batches: receipt union = all $(echo $CHECKS_ALL | wc -w | tr -d ' ') checks, each exactly once"
        else bf=1; echo "batches: receipt union WRONG"; fi
    fi
    rm -rf "$W"
    [ $bf = 0 ] && echo "kernel check batches: ok" || echo "kernel check batches: FAILED"
    exit $bf
fi
UA=${1:-/tmp/ua_ref}
SEL=${CHECKS:-$CHECKS_ALL}
[ -n "$SEL" ] || { echo "kernel check: empty selection"; exit 2; }
for c in $SEL; do case " $CHECKS_ALL " in *" $c "*) ;; *) echo "kernel check: unknown check '$c' ($CHECKS_ALL)"; exit 2 ;; esac; done
sel() { case " $SEL " in *" $1 "*) case " ${SKIP:-} " in *" $1 "*) echo "SKIP $1 (test entry: body dropped)"; return 1 ;; esac; return 0 ;; esac; return 1; }
[ -x "$UA" ] || { echo "kernel check: no unisacc at $UA (tests/build_ref.sh builds /tmp/ua_ref)"; exit 2; }
R=$(pwd)
[ -f iterate/kernel/genmodel.c ] || { echo "kernel check: run from the repo root"; exit 2; }
T=${TMPDIR:-/tmp}/kernel_check.$$
mkdir -p "$T"
PASS=$T/pass; : > "$PASS"
P() { echo "$1" >> "$PASS"; }
fail=0
K=iterate/kernel
GOLD=$(ls weights/gold/*.tsv)
# gm <build> <out> [order] [uns2] [tsv...]: run a genmodel build on the declared inputs
gm() { b=$1; o=$2; shift 2
    if [ $# = 0 ]; then set -- $K/order.tsv weights/built.uns2 $GOLD; fi
    B 30 "$T/gm.$b" -o "$o" "$@"; }
# mix <new> <out> [old]: the old model.inc with its region replaced by new's
mix() { old=${3:-kernel/unisa_model.inc}
    awk -v NEW="$1" '
        function span(f,   l, st, md, n) {   # print f from "/* S_<stage>" through model_dims "}"
            st = 0; md = 0
            while ((getline l < f) > 0) {
                if (!st && index(l, "/* S_<stage>") == 1) st = 1
                if (st) print l
                if (st && md && l == "}") break
                if (l == "int model_dims(void) {") md = 1
            }
            close(f)
        }
        phase == 0 && index($0, "/* S_<stage>") == 1 { phase = 1; span(NEW); next }
        phase == 0 { print; next }
        phase == 1 { if (md && $0 == "}") phase = 2; if ($0 == "int model_dims(void) {") md = 1; next }
        { print }' "$old" > "$2"
}
# semb <model.inc> <build> <out>: the real kernel (kernel/unisa_core.c minus
# its #include lines) + sem.c, over the given model, built by cc or unisacc
semb() { { echo '#include <stdio.h>'; cat "$1"; grep -v '^#include "unisa_' kernel/unisa_core.c; cat $K/sem.c; } > "$3.c"
    if [ $2 = cc ]; then B 60 cc -std=c99 -O2 -w -o "$3" "$3.c"; else B 60 "$UA" -O2 "$3.c" -b osx/arm64 -o "$3"; fi; }
region() { awk -f $K/region.awk "$1"; }

t0=$(now)
# ---- builds (every run needs them; the "build" check records them)
B 60 cc -std=c99 -O2 -w -o "$T/gm.cc" $K/genmodel.c; r1=$?
B 60 "$UA" -O2 $K/genmodel.c -b osx/arm64 -o "$T/gm.ua"; r2=$?
B 60 cc -std=c99 -O1 -w -fsanitize=undefined,address -fno-sanitize-recover=all -o "$T/gm.san" $K/genmodel.c; r3=$?
[ $r1 = 0 ] && [ $r2 = 0 ] && [ $r3 = 0 ] || { echo "genmodel build failed: cc $r1 ua $r2 san $r3"; exit 1; }
for b in cc ua san; do
    gm $b "$T/base.$b" > "$T/base.$b.log" 2>&1; rc=$?
    [ $rc = 0 ] && ! grep -q 'runtime error\|AddressSanitizer' "$T/base.$b.log" || { echo "genmodel $b failed (rc $rc): $(head -2 "$T/base.$b.log")"; exit 1; }
done
echo "builds: cc, unisacc -O2 (osx/arm64), cc -fsanitize=undefined,address; each ran on the declared inputs ($(perl -e "printf '%.1f', $(now) - $t0") s)"
if sel build; then for b in cc ua san; do echo "build $b ok: $(cat "$T/base.$b.log")"; P "build $b"; done; fi

if sel order; then
    B 30 python3 $K/order_check.py > "$T/oc" 2>&1 && P "order gold"; cat "$T/oc"
    # genmodel reads the order from order.tsv only: no stage name, no ALL
    # tuple is written into the C source
    n=$(grep -c -E '"(pp|lex|parse|type|scope|irsel|enc|reloc|regmap|tyinfo|pfconv|peep|opinfo|prec|binsel|isel|abi|combo)"' $K/genmodel.c)
    [ "$n" = 0 ] && { echo "order source: genmodel.c names no stage"; P "order source"; } || echo "order source: genmodel.c names a stage ($n lines)"
fi

if sel region; then
    # Python as ORACLE only: a scratch emit-kernel at this commit
    B 60 python3 -m unisa emit-kernel --out "$T/py" > /dev/null 2>&1 || echo "region: emit-kernel failed"
    region "$T/py/unisa_model.inc" > "$T/r.py"
    region kernel/unisa_model.inc > "$T/r.ship"
    if [ -s "$T/r.py" ] && cmp -s "$T/r.py" "$T/r.ship"; then echo "region: oracle region = kernel/unisa_model.inc region ($(wc -c < "$T/r.py" | tr -d ' ') B, $(wc -l < "$T/r.py" | tr -d ' ') lines)"; P "region oracle=shipped"
    else echo "region: oracle region differs from the shipped one"; fi
    for b in cc ua san; do
        region "$T/base.$b" > "$T/r.$b"
        if [ -s "$T/r.py" ] && cmp -s "$T/r.py" "$T/r.$b"; then echo "region $b: byte-identical to the oracle ($(wc -c < "$T/r.$b" | tr -d ' ') B)"; P "region $b"
        else echo "region $b: DIFFERS"; cmp "$T/r.py" "$T/r.$b" | head -2; diff "$T/r.py" "$T/r.$b" | head -4; fi
    done
fi

if sel empty; then
    # only fopen(path) of a command-line argument and fopen(opath) for the output
    n=$(grep -c 'fopen(' $K/genmodel.c); m=$(grep 'fopen(' $K/genmodel.c | grep -c -e 'fopen(path, "rb")' -e 'fopen(opath, "wb")')
    [ "$n" = 2 ] && [ "$m" = 2 ] && { echo "empty: genmodel.c opens only argv paths (fopen x2: input, output)"; P "empty source"; } || echo "empty: genmodel.c fopen sites $n (argv-only $m)"
    E=$T/empty; mkdir -p "$E/gold"
    cp $K/order.tsv weights/built.uns2 "$E/"; cp $GOLD "$E/gold/"
    for b in cc ua; do
        cp "$T/gm.$b" "$T/run.$b"
        ls -A "$E" | tr '\n' ' ' > "$T/ls.$b"
        ( cd "$E" && B 30 "$T/run.$b" -o "$T/empty.$b" order.tsv built.uns2 gold/*.tsv ) > "$T/e.$b" 2>&1; rc=$?
        if [ $rc = 0 ] && [ "$(cat "$T/ls.$b")" = "built.uns2 gold order.tsv " ] && cmp -s "$T/base.cc" "$T/empty.$b"; then
            echo "empty $b: cwd holds only [$(cat "$T/ls.$b")] (no kernel/, no built.json); output identical to the repo-root run ($(wc -c < "$T/empty.$b" | tr -d ' ') B)"; P "empty $b"
        else echo "empty $b: FAILED (rc $rc, cwd [$(cat "$T/ls.$b")]): $(head -1 "$T/e.$b")"; fi
    done
fi

if sel sem; then
    mix "$T/base.cc" "$T/mixed.inc"
    for b in cc ua; do
        semb "$T/mixed.inc" $b "$T/sem.$b" > "$T/sb.$b" 2>&1 || { echo "sem $b: build failed: $(head -2 "$T/sb.$b")"; continue; }
        B 30 "$T/sem.$b" > "$T/so.$b" 2>&1; rc=$?
        if [ $rc = 0 ] && grep -q '^sem: 18 stages, [0-9]* keys, [0-9]* (key, head) decisions, 0 wrong, 0 not unique, 0 layout errors$' "$T/so.$b"; then
            echo "sem $b (MIXED model.inc: genmodel region + shipped vocab/BF/BH/ENC/header; real unisa_core.c infer): $(cat "$T/so.$b")"; P "sem $b"
        else echo "sem $b: FAILED (rc $rc): $(tail -3 "$T/so.$b")"; fi
    done
fi

if sel oracle; then
    # the whole compiler rebuilt by tests/build_ref.sh in a scratch tree whose
    # kernel/unisa_model.inc is the MIXED file, then --check-oracle
    O=$T/tree; mkdir -p "$O/kernel" "$O/tests" "$O/src"
    cp kernel/unisa_headers.inc kernel/unisa_core.c "$O/kernel/"; cp src/*.c "$O/src/"
    cp tests/build_ref.sh tests/refshim.h tests/reffoot.h "$O/tests/"
    mix "$T/base.cc" "$O/kernel/unisa_model.inc"
    ( cd "$O" && B 60 sh tests/build_ref.sh "$O/ua.c" "$O/ua" ) > "$T/ob" 2>&1; rc=$?
    if [ $rc = 0 ] && [ -x "$O/ua" ] && grep -qxF '/* S_<stage>: the stage order of order.tsv */' "$O/ua.c"; then
        echo "oracle: tests/build_ref.sh built the compiler over the MIXED model.inc"; P "oracle build"
        B 60 "$O/ua" --check-oracle > "$T/oo" 2>&1; rc=$?
        if [ $rc = 0 ]; then echo "oracle: --check-oracle rc 0: $(tail -2 "$T/oo" | tr '\n' ' ')"; P "oracle check"
        else echo "oracle: --check-oracle FAILED (rc $rc): $(tail -3 "$T/oo")"; fi
    else echo "oracle: build_ref FAILED (rc $rc): $(tail -3 "$T/ob")"; fi
fi

if sel neg; then
    N=$T/neg; mkdir -p "$N"
    noprec=$(echo "$GOLD" | grep -v '/prec.tsv$')
    grep -v '^stage	combo$' $K/order.tsv > "$N/miss.tsv"
    sed 's/^stage	combo$/stage	prec/' $K/order.tsv > "$N/dup.tsv"
    sed 's/^stage	combo$/stage	combox/' $K/order.tsv > "$N/unk.tsv"
    head -c 9000 weights/built.uns2 > "$N/trunc.uns2"
    head -c 10 weights/built.uns2 > "$N/trunchead.uns2"
    { cat weights/built.uns2; printf 'x'; } > "$N/trail.uns2"
    { printf 'UNS3'; tail -c +5 weights/built.uns2; } > "$N/magic.uns2"
    mkdir -p "$N/dv" "$N/dc" "$N/tw" "$N/tu"
    # a field with one more value (and its row): valid TSV, 20 values vs UNS2's 19
    awk '/^#field/ { print $0 "\tzz"; next } { print } END { print "zz\tnone" }' weights/gold/prec.tsv > "$N/dv/prec.tsv"
    # a head with one more class no row uses: valid TSV, 12 classes vs UNS2's 11
    awk '/^#head/ { print $0 "\t11"; next } { print }' weights/gold/prec.tsv > "$N/dc/prec.tsv"
    sed 's/^# stage prec:/# stage precx:/' weights/gold/prec.tsv > "$N/tu/prec.tsv"
    for b in cc ua san; do
        for c in "notsv|17 gold tables given, want exactly 18" \
                 "noorder|$N/none/order.tsv: cannot open" \
                 "nouns2|$N/none.uns2: cannot open" \
                 "duporder|duplicate stage prec" \
                 "missorder|17 stages, want exactly 18" \
                 "unkorder|stage combox of order.tsv is not in UNS2" \
                 "trunc|truncated UNS2" \
                 "trunchead|truncated UNS2: header" \
                 "trail|1 trailing bytes after the last stage" \
                 "badmagic|not UNS2 (magic)" \
                 "dimval|stage prec: field 0 has 19 values in UNS2, 20 in the TSV" \
                 "dimcls|stage prec: head 0 has 11 classes in UNS2, 12 in the TSV" \
                 "tsvtwice|stage prec given twice" \
                 "tsvunk|stage prec of order.tsv has no gold table"; do
            n=${c%%|*}; want=${c#*|}
            case $n in
            notsv) set -- $K/order.tsv weights/built.uns2 $noprec ;;
            noorder) set -- "$N/none/order.tsv" weights/built.uns2 $GOLD ;;
            nouns2) set -- $K/order.tsv "$N/none.uns2" $GOLD ;;
            duporder) set -- "$N/dup.tsv" weights/built.uns2 $GOLD ;;
            missorder) set -- "$N/miss.tsv" weights/built.uns2 $GOLD ;;
            unkorder) set -- "$N/unk.tsv" weights/built.uns2 $GOLD ;;
            trunc) set -- $K/order.tsv "$N/trunc.uns2" $GOLD ;;
            trunchead) set -- $K/order.tsv "$N/trunchead.uns2" $GOLD ;;
            trail) set -- $K/order.tsv "$N/trail.uns2" $GOLD ;;
            badmagic) set -- $K/order.tsv "$N/magic.uns2" $GOLD ;;
            dimval) set -- $K/order.tsv weights/built.uns2 $noprec "$N/dv/prec.tsv" ;;
            dimcls) set -- $K/order.tsv weights/built.uns2 $noprec "$N/dc/prec.tsv" ;;
            tsvtwice) set -- $K/order.tsv weights/built.uns2 $(echo "$GOLD" | grep -v '/reloc.tsv$') weights/gold/prec.tsv ;;
            tsvunk) set -- $K/order.tsv weights/built.uns2 $noprec "$N/tu/prec.tsv" ;;
            esac
            rm -f "$N/out"
            gm $b "$N/out" "$@" > "$N/o" 2>&1; rc=$?
            if [ $rc = 1 ] && grep -qF "$want" "$N/o" && [ ! -e "$N/out" ] && ! grep -q 'runtime error\|AddressSanitizer' "$N/o"; then
                echo "neg $n $b: rc 1, no output: $(head -1 "$N/o")"; P "neg $n $b"
            else echo "neg $n $b: FAILED (rc $rc, want 1 and '$want'): $(head -1 "$N/o")"; fi
        done
    done
    # heads_max is the kernel's STAGE_NCLS stride, not a free choice: any other
    # value must be refused (exit 8) before a layout is made or the output
    # opened.  And the stride itself is checked against the REAL kernel source,
    # so KERNEL_HEADS in genmodel.c cannot silently go stale if the kernel
    # changes its indexing.
    sed 's/^heads_max	16$/heads_max	15/' $K/order.tsv > "$N/hm15.tsv"
    sed 's/^heads_max	16$/heads_max	17/' $K/order.tsv > "$N/hm17.tsv"
    for b in cc ua san; do
        for n in hm15 hm17; do
            v=${n#hm}
            rm -f "$N/out"
            gm $b "$N/out" "$N/$n.tsv" weights/built.uns2 $GOLD > "$N/o" 2>&1; rc=$?
            want="heads_max $v, the kernel's STAGE_NCLS stride is 16"
            if [ $rc = 8 ] && grep -qF "$want" "$N/o" && [ ! -e "$N/out" ] && ! grep -q 'runtime error\|AddressSanitizer' "$N/o"; then
                echo "neg $n $b: rc 8, no output: $(head -1 "$N/o")"; P "neg $n $b"
            else echo "neg $n $b: FAILED (rc $rc, want 8 and '$want'): $(head -1 "$N/o")"; fi
        done
    done
    ks=$(grep -c 'STAGE_NCLS\[(s << 4) + head\]' kernel/unisa_core.c)
    gs=$(grep -c '^#define KERNEL_HEADS 16 ' $K/genmodel.c)
    os=$(grep -c '^heads_max	16$' $K/order.tsv)
    if [ "$ks" -ge 1 ] && [ "$gs" = 1 ] && [ "$os" = 1 ]; then
        echo "neg stride: kernel indexes STAGE_NCLS[(s << 4) + head], genmodel KERNEL_HEADS 16, order.tsv heads_max 16"; P "neg stride"
    else echo "neg stride: FAILED (kernel '<< 4' lines $ks, genmodel KERNEL_HEADS 16 lines $gs, order.tsv heads_max 16 lines $os)"; fi
fi

if sel wfail; then
    mkdir -p "$T/wf"
    for b in cc ua san; do
        gm $b "$T/wf" > "$T/w.o" 2>&1; rc=$?
        [ $rc = 7 ] && grep -q "write: cannot open $T/wf for writing" "$T/w.o" && { echo "wfail opendir $b: rc 7: $(head -1 "$T/w.o")"; P "wfail opendir $b"; } || echo "wfail opendir $b: FAILED (rc $rc): $(head -1 "$T/w.o")"
        gm $b "$T/wf/none/x.inc" > "$T/w.o" 2>&1; rc=$?
        [ $rc = 7 ] && grep -q "write: cannot open" "$T/w.o" && { echo "wfail nodir $b: rc 7: $(head -1 "$T/w.o")"; P "wfail nodir $b"; } || echo "wfail nodir $b: FAILED (rc $rc): $(head -1 "$T/w.o")"
        # a real short write: file size limit 1 block, SIGXFSZ ignored so
        # write(2) returns EFBIG; the ~200 KB output must not come back whole
        rm -f "$T/wf/big.inc"
        B 30 sh -c 'ulimit -f 1; trap "" XFSZ; exec "$@"' sh "$T/gm.$b" -o "$T/wf/big.inc" $K/order.tsv weights/built.uns2 $GOLD > "$T/w.o" 2>&1; rc=$?
        [ $rc = 7 ] && grep -q -e "short write" -e "close failed" "$T/w.o" && { echo "wfail rlimit $b: rc 7: $(head -1 "$T/w.o")"; P "wfail rlimit $b"; } || echo "wfail rlimit $b: FAILED (rc $rc): $(head -1 "$T/w.o")"
    done
fi

if sel label; then
    # ONE label changed in a temp copy of prec.tsv (the "||" row: 1 -> 2);
    # built.uns2 unchanged.  genmodel must carry it into DENSE (it reads the
    # labels) and the real kernel must disagree with it (REJECT).
    L=$T/lab; mkdir -p "$L"
    awk '!d && $0 == "||\t1" { print "||\t2"; d = 1; next } { print }' weights/gold/prec.tsv > "$L/prec.tsv"
    if cmp -s weights/gold/prec.tsv "$L/prec.tsv"; then echo "label: the edit did not apply"; else
    gm cc "$L/out" $K/order.tsv weights/built.uns2 $(echo "$GOLD" | grep -v '/prec.tsv$') "$L/prec.tsv" > "$L/o" 2>&1; rc=$?
    if [ $rc = 0 ] && ! cmp -s "$L/out" "$T/base.cc"; then echo "label: genmodel output changed with the label"; P "label differs"; else echo "label: FAILED (rc $rc; output unchanged?)"; fi
    region "$L/out" > "$L/r"; region "$T/base.cc" > "$L/r0"
    d=$(diff "$L/r0" "$L/r" | grep -c '^[<>]'); dn=$(diff "$L/r0" "$L/r" | grep '^[<>]' | grep -c -v '^[<>]   "')
    # the only changed lines are string-continuation lines, and they lie in DENSE
    first=$(diff "$L/r0" "$L/r" | sed -n 's/^\([0-9]*\)[acd].*/\1/p' | head -1); dl=$(grep -n '^char \*DENSE =' "$L/r0" | cut -d: -f1)
    if [ "$d" = 2 ] && [ "$dn" = 0 ] && [ -n "$first" ] && [ "$first" -gt "$dl" ]; then echo "label: one line of the region changed, inside DENSE (line $first > DENSE at $dl)"; P "label dense-only"
    else echo "label: diff not confined to one DENSE line ($d lines, $dn non-string, first $first, DENSE $dl)"; fi
    mix "$L/out" "$L/mixed.inc"
    for b in cc ua; do
        semb "$L/mixed.inc" $b "$L/sem.$b" > "$L/sb" 2>&1 || { echo "label $b: build failed"; continue; }
        B 30 "$L/sem.$b" > "$L/so.$b" 2>&1; rc=$?
        if [ $rc != 0 ] && grep -q ' 1 wrong, ' "$L/so.$b"; then echo "label reject $b: rc $rc: $(head -1 "$L/so.$b") / $(tail -1 "$L/so.$b")"; P "label reject $b"
        else echo "label reject $b: NOT rejected (rc $rc): $(tail -1 "$L/so.$b")"; fi
    done
    fi
fi

if sel perm; then
    # a legal reordering: the 18 stages reversed
    M=$T/perm; mkdir -p "$M"
    { grep '^#' $K/order.tsv; grep '^stage	' $K/order.tsv | awk '{ l[NR] = $0 } END { for (i = NR; i >= 1; i--) print l[i] }'; grep '^heads_max	' $K/order.tsv; } > "$M/order.tsv"
    gm cc "$M/out" "$M/order.tsv" weights/built.uns2 $GOLD > "$M/o" 2>&1 || echo "perm: genmodel failed: $(head -1 "$M/o")"
    B 60 python3 $K/permoracle.py "$M/order.tsv" "$M/py" > "$M/po" 2>&1 || echo "perm: oracle failed: $(tail -1 "$M/po")"
    region "$M/py/unisa_model.inc" > "$M/r.py"; region "$M/out" > "$M/r.gm"
    if [ -s "$M/r.py" ] && cmp -s "$M/r.py" "$M/r.gm" && region "$T/base.cc" > "$M/r.base" && [ -s "$M/r.base" ] && ! cmp -s "$M/r.gm" "$M/r.base"; then echo "perm: reversed order: region byte-identical to the oracle with ALL permuted the same way ($(wc -c < "$M/r.gm" | tr -d ' ') B), and differs from the unpermuted region"; P "perm oracle"
    else echo "perm: region vs permuted oracle DIFFERS (or equals the unpermuted one)"; fi
    # structural: S_<X> is X's line index in the permuted order.tsv
    grep '^stage	' "$M/order.tsv" | cut -f2 | awk '{ printf "#define S_%s %d\n", toupper($1), NR - 1 }' > "$M/ids.want"
    grep '^#define S_' "$M/out" > "$M/ids.got"
    if cmp -s "$M/ids.want" "$M/ids.got" && [ "$(head -1 "$M/ids.got")" = "#define S_COMBO 0" ]; then echo "perm ids: S_* = position in the permuted order.tsv (S_COMBO 0 .. S_PP 17)"; P "perm ids"; else echo "perm ids: FAILED"; fi
    mix "$M/out" "$M/mixed.inc"
    semb "$M/mixed.inc" cc "$M/sem" > "$M/sb" 2>&1 && B 30 "$M/sem" > "$M/so" 2>&1; rc=$?
    if [ $rc = 0 ] && grep -q '0 wrong, 0 not unique, 0 layout errors' "$M/so"; then echo "perm sem: $(cat "$M/so")"; P "perm sem"; else echo "perm sem: FAILED (rc $rc): $(tail -1 "$M/so")"; fi
fi

if sel fault; then
    # a dropped check: `order` selected, its body skipped, nothing fails
    # inside it -- the run must still fail, on the missing receipt
    CHECKS=order SKIP=order B 60 sh "$0" "$UA" > "$T/fo" 2>&1; rc=$?
    if [ $rc != 0 ] && grep -q '^summary: order: attempted, FAILED, no receipt' "$T/fo" && ! grep -q '^receipt order' "$T/fo"; then echo "fault: dropped check -> rc $rc, $(grep '^summary: order' "$T/fo")"; P "fault dropped"
    else echo "fault: a dropped check did NOT fail the run (rc $rc)"; fi
fi

# ---- receipts
: > "$T/rcpt"; nf=0
for c in $SEL; do
    m=$(need $c | while read -r x; do grep -qxF "$x" "$PASS" || printf "[%s] " "$x"; done)
    if [ -z "$m" ]; then echo "receipt $c" >> "$T/rcpt"; echo "summary: $c: attempted, passed ($(need $c | wc -l | tr -d ' ') marks)"
    else nf=$((nf + 1)); echo "summary: $c: attempted, FAILED, no receipt; missing $m"; fi
done
[ $nf = 0 ] || fail=1
cat "$T/rcpt"
echo "total $(perl -e "printf '%.1f', $(now) - $t0") s"
rm -rf "$T"
[ $fail = 0 ] && echo "kernel check: ok" || echo "kernel check: FAILED"
exit $fail
