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
CHECKS_ALL="order build region empty sem oracle neg wfail label perm fault vocab region2 vneg vpos pool rename integ tneg tpos tfunc"
TNEGS="nofile dup empty quote bslash hibyte trigraph oct0 oct7 norow duprow"
VNEGS="unkstage nofield nohead dupsym dupbfbh missrow extrarow unkkind quote bslash hibyte trigraph oct0 oct7 badident casefold existing"
NEGS="notsv noorder nouns2 duporder missorder unkorder trunc trunchead trail badmagic dimval dimcls tsvtwice tsvunk hm15 hm17"
BATCHES='order build region empty sem vocab region2
oracle
neg wfail label
perm fault
vneg vpos pool rename
tneg tpos tfunc
integ'
need() {
    case $1 in
    order) echo "order gold"; echo "order source" ;;
    build) for b in cc ua san; do echo "build $b"; done ;;
    region) echo "region oracle=shipped"; for b in cc ua san; do echo "region $b"; done ;;
    empty) echo "empty source"; for b in cc ua; do echo "empty $b"; done; echo "empty typev" ;;
    tneg) for n in $TNEGS; do for b in cc ua san; do echo "tneg $n $b"; done; done ;;
    tpos) for n in oct8 oct9; do for b in cc ua san; do echo "tpos $n $b"; done; done ;;
    tfunc) for n in rename reorder; do echo "tfunc $n bytes"; echo "tfunc $n judge"; for b in ua san; do echo "tfunc $n $b"; done; done ;;
    sem) for b in cc ua; do echo "sem $b"; done ;;
    oracle) echo "oracle build"; echo "oracle check" ;;
    neg) for n in $NEGS; do for b in cc ua san; do echo "neg $n $b"; done; done; echo "neg stride" ;;
    wfail) for t in opendir nodir rlimit; do for b in cc ua san; do echo "wfail $t $b"; done; done ;;
    label) echo "label differs"; echo "label dense-only"; for b in cc ua; do echo "label reject $b"; done ;;
    perm) echo "perm oracle"; echo "perm ids"; echo "perm sem" ;;
    fault) echo "fault dropped" ;;
    vocab) echo "vocab ckernel"; echo "vocab typekw"; for n in ckempty cknone lexempty lexnone; do echo "vocab judge $n"; done ;;
    region2) echo "region2 oracle=shipped"; for b in cc ua san; do echo "region2 $b"; done ;;
    vneg) for n in $VNEGS; do for b in cc ua san; do echo "vneg $n $b"; done; done ;;
    vpos) for n in oct8 oct9; do for b in cc ua san; do echo "vpos $n $b"; done; done ;;
    pool) echo "pool kept"; echo "pool only"; for b in ua san; do echo "pool $b"; done ;;
    rename) echo "rename predicted"; echo "rename region1"; for b in ua san; do echo "rename $b"; done ;;
    integ) echo "integ mixed=shipped"; echo "integ closure"; echo "integ nativeboot" ;;
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
        grep -a '^receipt ' "$W/out.$n" | sort > "$W/got.$n"
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
    ord=$1; shift     # the vocab mapping goes second: VOC overrides it
    B 30 "$T/gm.$b" -o "$o" "$ord" "${VOC:-$K/vocab.tsv}" "${TKW:-$K/typekw.tsv}" "$@"; }
# mix <new> <out> [old]: the scratch integrator (mix.awk): old's header and
# ENC_.. tail around new's S_* .. TYPEV .. BF/BH region
mix() { awk -v NEW="$1" -f $K/mix.awk "${3:-kernel/unisa_model.inc}" > "$2"; }
# vr <py|gm> <file>: the slice-2 region (vregion.awk), checked against $T/exp
vr() { awk -v END_MODE=$1 -v EXP="$T/exp" -f $K/vregion.awk "$2"; }
# ren <old> <new> <in> <out>: rename one value everywhere it is a whole TSV
# cell (schema and rows alike: a consistent rename); ENVIRON, so no escapes
ren() { A="$1" Z="$2" LC_ALL=C awk -F'\t' -v OFS='\t' 'index($0, "# stage ") == 1 { print; next } { for (i = 1; i <= NF; i++) if ($i == ENVIRON["A"]) { $i = ENVIRON["Z"]; c++ } print } END { if (!c) exit 1 }' "$3" > "$4"; }
# semb <model.inc> <build> <out>: the real kernel (kernel/unisa_core.c minus
# its #include lines) + sem.c, over the given model, built by cc or unisacc
semb() { { echo '#include <stdio.h>'; cat "$1"; grep -v '^#include "unisa_' kernel/unisa_core.c; cat $K/sem.c; } > "$3.c"
    if [ $2 = cc ]; then B 60 cc -std=c99 -O2 -w -o "$3" "$3.c"; else B 60 "$UA" -O2 "$3.c" -b osx/arm64 -o "$3"; fi; }
region() { awk -f $K/region.awk "$1"; }
# the expected slice-2 symbols, from vocab.tsv and the TSV schemas (an
# independent list: awk, not genmodel)
awk -F'\t' -v G=weights/gold '$1 == "vocab" || $1 == "typekw" { print $2; print "N" $2 }
    $1 == "bfbh" { st = $2; f = G "/" st ".tsv"; i = 0
        while ((getline l < f) > 0) { split(l, a, "\t")
            if (a[1] == "#field") { print "BF_" toupper(st) "_" i; print "NBF_" toupper(st) "_" i; i++ }
            else if (a[1] == "#head") { h = toupper(st) "_" toupper(a[2]); print "BH_" h; print "NBH_" h; print "HD_" h } }
        close(f) }' $K/vocab.tsv > "$T/exp"

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
    cp $K/order.tsv $K/vocab.tsv $K/typekw.tsv weights/built.uns2 "$E/"; cp $GOLD "$E/gold/"
    for b in cc ua; do
        cp "$T/gm.$b" "$T/run.$b"
        ls -A "$E" | tr '\n' ' ' > "$T/ls.$b"
        ( cd "$E" && B 30 "$T/run.$b" -o "$T/empty.$b" order.tsv vocab.tsv typekw.tsv built.uns2 gold/*.tsv ) > "$T/e.$b" 2>&1; rc=$?
        if [ $rc = 0 ] && [ "$(cat "$T/ls.$b")" = "built.uns2 gold order.tsv typekw.tsv vocab.tsv " ] && cmp -s "$T/base.cc" "$T/empty.$b"; then
            echo "empty $b: cwd holds only [$(cat "$T/ls.$b")] (no kernel/, no built.json, no lex.py); output identical to the repo-root run ($(wc -c < "$T/empty.$b" | tr -d ' ') B)"; P "empty $b"
        else echo "empty $b: FAILED (rc $rc, cwd [$(cat "$T/ls.$b")]): $(head -1 "$T/e.$b")"; fi
    done
    # TYPEV comes from the declared typekw.tsv alone: the empty-dir output's
    # TYPEV / NTYPEV lines equal the shipped ones, and no placeholder is left
    if [ -f "$T/empty.cc" ] && [ "$(grep -c -e '^char \*TYPEV = ' -e '^#define NTYPEV ' "$T/empty.cc")" = 2 ] \
       && [ "$(grep -e '^char \*TYPEV = ' -e '^#define NTYPEV ' "$T/empty.cc")" = "$(grep -e '^char \*TYPEV = ' -e '^#define NTYPEV ' kernel/unisa_model.inc)" ] \
       && ! grep -q 'placeholder' "$T/empty.cc"; then
        echo "empty typev: TYPEV/NTYPEV from typekw.tsv only, equal to the shipped lines: $(grep '^#define NTYPEV ' "$T/empty.cc")"; P "empty typev"
    else echo "empty typev: FAILED"; fi
fi

if sel sem; then
    mix "$T/base.cc" "$T/mixed.inc"
    for b in cc ua; do
        semb "$T/mixed.inc" $b "$T/sem.$b" > "$T/sb.$b" 2>&1 || { echo "sem $b: build failed: $(head -2 "$T/sb.$b")"; continue; }
        B 30 "$T/sem.$b" > "$T/so.$b" 2>&1; rc=$?
        if [ $rc = 0 ] && grep -q '^sem: 18 stages, [0-9]* keys, [0-9]* (key, head) decisions, 0 wrong, 0 not unique, 0 layout errors$' "$T/so.$b"; then
            echo "sem $b (MIXED model.inc: both genmodel regions incl. TYPEV + shipped header/ENC; real unisa_core.c infer): $(cat "$T/so.$b")"; P "sem $b"
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
    if [ $rc = 0 ] && [ -x "$O/ua" ] && grep -qxF '/* S_<stage>: the stage order of order.tsv */' "$O/ua.c" && grep -qxF '/* TOKV: gold TSV parse #field tok */' "$O/ua.c"; then
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
        B 30 sh -c 'ulimit -f 1; trap "" XFSZ; exec "$@"' sh "$T/gm.$b" -o "$T/wf/big.inc" $K/order.tsv $K/vocab.tsv $K/typekw.tsv weights/built.uns2 $GOLD > "$T/w.o" 2>&1; rc=$?
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

# ================================================ second slice: vocab/BF/BH
if sel vocab; then
    B 60 python3 $K/vocab_check.py > "$T/vc" 2>&1 && P "vocab ckernel"; cat "$T/vc"
    B 30 python3 $K/typekw_check.py $K/typekw.tsv > "$T/tc" 2>&1 && P "vocab typekw"; cat "$T/tc"
    # the judges fail CLOSED: a source whose expected structure is missing or
    # EMPTY is an error (rc != 0, no traceback), never a silent pass
    JN=$T/jn; mkdir -p "$JN"
    awk '/^ *for nm, vals in \(\("TOKV", TOKS\)/ { print "    for nm, vals in ():"; skip = 1; next } skip { if (/\("IRRECV", RECIPE\)\):/) skip = 0; next } { print }' unisa/ckernel.py > "$JN/ckempty.py"
    awk '/^ *for nm, vals in \(\("TOKV", TOKS\)/ { print "    for nm, vals in []:"; skip = 1; next } skip { if (/\("IRRECV", RECIPE\)\):/) skip = 0; next } { print }' unisa/ckernel.py | sed 's/^    for nm, vals in \[\]:$/    for nm, vals in VOCABS:/' > "$JN/cknone.py"
    awk '/^TYPEKW = \(/ { print "TYPEKW = ()"; skip = 1; next } skip { if (/"restrict"\)/) skip = 0; next } { print }' unisa/front/lex.py > "$JN/lexempty.py"
    sed 's/^TYPEKW = /TYPEKWX = /' unisa/front/lex.py > "$JN/lexnone.py"
    for n in ckempty cknone lexempty lexnone; do
        case $n in
        ck*) CKERNEL_SRC="$JN/$n.py" B 30 python3 $K/vocab_check.py > "$JN/o" 2>&1; rc=$? ;;
        *) LEX_SRC="$JN/$n.py" B 30 python3 $K/typekw_check.py > "$JN/o" 2>&1; rc=$? ;;
        esac
        if [ $rc != 0 ] && ! grep -q Traceback "$JN/o" && B 30 python3 -c 'import ast,sys; ast.parse(open(sys.argv[1]).read())' "$JN/$n.py"; then echo "vocab judge $n: rc $rc: $(tail -1 "$JN/o")"; P "vocab judge $n"
        else echo "vocab judge $n: FAILED (rc $rc): $(tail -1 "$JN/o")"; fi
    done
fi

if sel region2; then
    # the same extractor (vregion.awk) on the Python oracle, the shipped file
    # and genmodel's three builds; it fails on a missed anchor, an empty
    # region, or a symbol set other than $T/exp
    [ -f "$T/py/unisa_model.inc" ] || B 60 python3 -m unisa emit-kernel --out "$T/py" > /dev/null 2>&1 || echo "region2: emit-kernel failed"
    vr py "$T/py/unisa_model.inc" > "$T/v.py" 2> "$T/v.py.e"; r1=$?
    vr py kernel/unisa_model.inc > "$T/v.ship" 2> "$T/v.ship.e"; r2=$?
    if [ $r1 = 0 ] && [ $r2 = 0 ] && [ -s "$T/v.py" ] && cmp -s "$T/v.py" "$T/v.ship"; then
        echo "region2: oracle = shipped ($(wc -c < "$T/v.py" | tr -d ' ') B; $(cat "$T/v.py.e"))"; P "region2 oracle=shipped"
    else echo "region2: oracle vs shipped FAILED (rc $r1/$r2): $(cat "$T/v.py.e" "$T/v.ship.e")"; fi
    for b in cc ua san; do
        vr gm "$T/base.$b" > "$T/v.$b" 2> "$T/v.$b.e"; rc=$?
        if [ $rc = 0 ] && [ $r1 = 0 ] && [ -s "$T/v.py" ] && cmp -s "$T/v.py" "$T/v.$b"; then
            echo "region2 $b: byte-identical to the oracle ($(wc -c < "$T/v.$b" | tr -d ' ') B; $(cat "$T/v.$b.e"))"; P "region2 $b"
        else echo "region2 $b: DIFFERS (rc $rc): $(cat "$T/v.$b.e")"; diff "$T/v.py" "$T/v.$b" | head -4 | cut -c1-160; fi
    done
fi

# vv <awk program> <name>: a broken copy of vocab.tsv
vv() { awk -F'\t' -v OFS='\t' "$1" $K/vocab.tsv > "$V/$2.tsv"; }
if sel vneg; then
    V=$T/vn; mkdir -p "$V"
    noprec=$(echo "$GOLD" | grep -v '/prec.tsv$'); noparse=$(echo "$GOLD" | grep -v '/parse.tsv$')
    vv '$1 == "vocab" && $2 == "TOKV" { $3 = "parsex" } { print }' unkstage
    vv '$2 == "TOKV" { $5 = "tokx" } { print }' nofield
    vv '$2 == "PRODV" { $5 = "yy" } { print }' nohead
    vv '$2 == "NTV" { $2 = "TOKV" } { print }' dupsym
    vv '$1 == "bfbh" && $2 == "abi" { $2 = "enc" } { print }' dupbfbh
    vv '$2 == "IRRECV" { next } { print }' missrow
    vv '{ print } END { print "vocab\tXTRAV\tparse\tfield\ttok" }' extrarow
    vv '{ print } END { print "vocabx\tXTRAV" }' unkkind
    vv '$2 == "NTV" { $2 = "1NTV" } { print }' badident
    vv '$2 == "NTV" { $2 = "tokv" } { print }' casefold
    vv '$2 == "NTV" { $2 = "DENSE" } { print }' existing
    ok=1
    for n in quote bslash hibyte trigraph oct0 oct7; do mkdir -p "$V/$n"; done
    ren '||' 'a"b' weights/gold/prec.tsv "$V/quote/prec.tsv" || ok=0
    ren '||' 'a\b' weights/gold/prec.tsv "$V/bslash/prec.tsv" || ok=0
    ren '||' "$(printf 'a\200b')" weights/gold/prec.tsv "$V/hibyte/prec.tsv" || ok=0
    ren '||' 'a??b' weights/gold/prec.tsv "$V/trigraph/prec.tsv" || ok=0
    ren if 0if weights/gold/parse.tsv "$V/oct0/parse.tsv" || ok=0
    ren if 7if weights/gold/parse.tsv "$V/oct7/parse.tsv" || ok=0
    [ $ok = 1 ] || echo "vneg: a fixture rename did not apply"
    for b in cc ua san; do
        for c in "unkstage|unknown stage parsex" \
                 "nofield|stage parse has no #field tokx" \
                 "nohead|stage parse has no #head yy" \
                 "dupsym|duplicate symbol TOKV" \
                 "dupbfbh|bfbh stage given twice: enc" \
                 "missrow|missing row: 15 vocab/typekw rows, want 16" \
                 "extrarow|extra row: 17 vocab/typekw rows, want 16" \
                 "unkkind|unknown row kind vocabx" \
                 "quote|BF_PREC_0 value 0 is outside this tool's input domain: a quote" \
                 "bslash|BF_PREC_0 value 0 is outside this tool's input domain: a backslash" \
                 "hibyte|BF_PREC_0 value 0 is outside this tool's input domain: a control or non-ASCII byte" \
                 "trigraph|BF_PREC_0 value 0 is outside this tool's input domain: \"??\"" \
                 "oct0|TOKV value 5 is outside this tool's input domain: a non-first vocab value starting 0-7" \
                 "oct7|TOKV value 5 is outside this tool's input domain: a non-first vocab value starting 0-7" \
                 "badident|symbol 1NTV is not a legal C identifier" \
                 "casefold|symbol tokv collides with TOKV (case-folded)" \
                 "existing|symbol DENSE collides with DENSE"; do
            n=${c%%|*}; want=${c#*|}
            case $n in
            quote|bslash|hibyte|trigraph) voc=$K/vocab.tsv; set -- $K/order.tsv weights/built.uns2 $noprec "$V/$n/prec.tsv" ;;
            oct0|oct7) voc=$K/vocab.tsv; set -- $K/order.tsv weights/built.uns2 $noparse "$V/$n/parse.tsv" ;;
            *) voc="$V/$n.tsv"; set -- $K/order.tsv weights/built.uns2 $GOLD ;;
            esac
            rm -f "$V/out"
            VOC=$voc; gm $b "$V/out" "$@" > "$V/o" 2>&1; rc=$?; unset VOC   # a function call keeps a prefix assignment in sh: set and unset
            if [ $rc = 1 ] && grep -qF "$want" "$V/o" && [ ! -e "$V/out" ] && ! grep -q 'runtime error\|AddressSanitizer' "$V/o"; then
                echo "vneg $n $b: rc 1, no output: $(head -1 "$V/o" | LC_ALL=C tr -c '\n -~' '?')"; P "vneg $n $b"
            else echo "vneg $n $b: FAILED (rc $rc, want 1 and '$want'): $(head -1 "$V/o" | LC_ALL=C tr -c '\n -~' '?')"; fi
        done
    done
fi

if sel vpos; then
    # a non-first vocab value starting 8 or 9 is legal: not an octal digit,
    # so "\08if" is \0 then '8'.  cc compiles the emitted TOKV line and reads
    # value 5 back.
    V=$T/vp; noparse=$(echo "$GOLD" | grep -v '/parse.tsv$')
    for n in oct8 oct9; do
        d=${n#oct}; mkdir -p "$V/$n"
        ren if ${d}if weights/gold/parse.tsv "$V/$n/parse.tsv" || echo "vpos: rename did not apply"
        for b in cc ua san; do
            rm -f "$V/out"
            gm $b "$V/out" $K/order.tsv weights/built.uns2 $noparse "$V/$n/parse.tsv" > "$V/o" 2>&1; rc=$?
            { grep '^char \*TOKV = ' "$V/out"; echo 'int puts(const char *); int main(void) { char *p = TOKV; int i; for (i = 0; i < 5; i = i + 1) while (*p++) ; puts(p); return 0; }'; } > "$V/x.c" 2>/dev/null
            B 60 cc -w -o "$V/x" "$V/x.c" > /dev/null 2>&1 && got=$(B 10 "$V/x") || got="(no build)"
            if [ $rc = 0 ] && grep -q "^char \*TOKV = \".*\\\\0${d}if\\\\0" "$V/out" && [ "$got" = "${d}if" ]; then
                printf '%s\n' "vpos $n $b: rc 0, TOKV holds '\\0${d}if\\0'; cc reads value 5 back as '$got'"; P "vpos $n $b"
            else echo "vpos $n $b: FAILED (rc $rc, value 5 '$got'): $(head -1 "$V/o")"; fi
        done
    done
fi

if sel pool; then
    # enc.tsv (loaded FIRST) and reloc.tsv (loaded LAST) each get a different
    # renamed string.  With one reused read buffer the earlier table's strings
    # would point into the later file's bytes; with the pool, each keeps its own.
    PL=$T/pool; mkdir -p "$PL"
    ren lnx POOLA_lnx weights/gold/enc.tsv "$PL/enc.tsv" && ren jmp POOLB_a_different_jmp weights/gold/reloc.tsv "$PL/reloc.tsv" || echo "pool: rename did not apply"
    mid=$(echo "$GOLD" | grep -v -e '/enc.tsv$' -e '/reloc.tsv$')
    for b in cc ua san; do
        gm $b "$PL/out.$b" $K/order.tsv weights/built.uns2 "$PL/enc.tsv" $mid "$PL/reloc.tsv" > "$PL/o.$b" 2>&1 || echo "pool $b: genmodel failed: $(head -1 "$PL/o.$b")"
    done
    vr gm "$T/base.cc" > "$PL/v0" 2>/dev/null; vr gm "$PL/out.cc" > "$PL/v1" 2>/dev/null
    grep '^char \*BF_ENC_1 = ' "$PL/v0" | sed 's/"lnx\\000/"POOLA_lnx\\000/' > "$PL/want"
    grep '^char \*BF_RELOC_0 = ' "$PL/v0" | sed 's/"jmp\\000/"POOLB_a_different_jmp\\000/' >> "$PL/want"
    grep -e '^char \*BF_ENC_1 = ' -e '^char \*BF_RELOC_0 = ' "$PL/v1" > "$PL/got"
    if [ -s "$PL/v1" ] && [ "$(wc -l < "$PL/want" | tr -d ' ')" = 2 ] && cmp -s "$PL/want" "$PL/got"; then
        printf '%s\n' "pool kept: $(tr '\n' ' ' < "$PL/got")"; P "pool kept"
    else echo "pool kept: FAILED: $(tr '\n' ' ' < "$PL/got")"; fi
    nd=$(diff "$PL/v0" "$PL/v1" | grep -c '^[<>]'); ns=$(diff "$PL/v0" "$PL/v1" | grep '^>' | sed 's/^> char \*\([A-Z0-9_]*\) = .*/\1/' | sort | tr '\n' ' ')
    if [ "$nd" = 4 ] && [ "$ns" = "BF_ENC_1 BF_RELOC_0 " ] && region "$T/base.cc" > "$PL/r0" && region "$PL/out.cc" > "$PL/r1" && cmp -s "$PL/r0" "$PL/r1"; then
        echo "pool only: exactly BF_ENC_1 and BF_RELOC_0 changed; first-slice region unchanged"; P "pool only"
    else echo "pool only: FAILED ($nd diff lines: $ns)"; fi
    for b in ua san; do cmp -s "$PL/out.cc" "$PL/out.$b" && { echo "pool $b: output identical to cc"; P "pool $b"; } || echo "pool $b: DIFFERS from cc"; done
fi

if sel rename; then
    # one value renamed consistently in opinfo.tsv (schema and rows): add64
    # -> addq.  Predicted from the TSV schema and vocab.tsv alone: every list
    # of the stage that holds the value, mapped to its symbol.
    RN=$T/ren; mkdir -p "$RN"; st=opinfo; old=add64; new=addq
    ren $old $new weights/gold/$st.tsv "$RN/$st.tsv" || echo "rename: did not apply"
    awk -F'\t' -v st=$st -v v=$old -v VOC=$K/vocab.tsv '
        BEGIN { while ((getline l < VOC) > 0) { split(l, a, "\t"); if (a[1] == "bfbh" && a[2] == st) bf = 1
                    if (a[1] == "vocab" && a[3] == st) m[a[4] "|" a[5]] = a[2] } }
        $1 == "#field" { hit = 0; for (i = 3; i <= NF; i++) if ($i == v) hit = 1
            if (hit) { if (bf) print "BF_" toupper(st) "_" nf + 0; if (("field|" $2) in m) print m["field|" $2] }; nf++ }
        $1 == "#head" { hit = 0; for (i = 4; i <= NF; i++) if ($i == v) hit = 1
            if (hit) { if (bf) print "BH_" toupper(st) "_" toupper($2); if (("head|" $2) in m) print m["head|" $2] } }' weights/gold/$st.tsv | sort | tr '\n' ' ' > "$RN/pred"
    others=$(echo "$GOLD" | grep -v "/$st.tsv\$")
    for b in cc ua san; do gm $b "$RN/out.$b" $K/order.tsv weights/built.uns2 $others "$RN/$st.tsv" > "$RN/o.$b" 2>&1 || echo "rename $b: genmodel failed: $(head -1 "$RN/o.$b")"; done
    vr gm "$T/base.cc" > "$RN/v0" 2>/dev/null; vr gm "$RN/out.cc" > "$RN/v1" 2>/dev/null
    diff "$RN/v0" "$RN/v1" | grep '^>' | sed 's/^> char \*\([A-Z0-9_]*\) = .*/\1/' | sort | tr '\n' ' ' > "$RN/obs"
    diff "$RN/v0" "$RN/v1" | grep '^<' | sed -e 's/^< //' -e "s/\"$old\\\\000/\"$new\\\\000/" -e "s/\\\\000$old\\\\000/\\\\000$new\\\\000/g" > "$RN/wantl"
    diff "$RN/v0" "$RN/v1" | grep '^>' | sed 's/^> //' > "$RN/gotl"
    nl=$(diff "$RN/v0" "$RN/v1" | grep -c '^<')
    if [ -s "$RN/v1" ] && [ -s "$RN/pred" ] && [ "$(cat "$RN/pred")" = "$(cat "$RN/obs")" ] && [ "$nl" = "$(wc -w < "$RN/pred" | tr -d ' ')" ] && cmp -s "$RN/wantl" "$RN/gotl"; then
        echo "rename predicted: $st $old -> $new: predicted [$(cat "$RN/pred")] = observed [$(cat "$RN/obs")]; each changed line is the old line with the value replaced"; P "rename predicted"
    else echo "rename predicted: FAILED: predicted [$(cat "$RN/pred")] observed [$(cat "$RN/obs")] ($nl lines)"; fi
    region "$T/base.cc" > "$RN/r0"; region "$RN/out.cc" > "$RN/r1"
    if [ -s "$RN/r0" ] && cmp -s "$RN/r0" "$RN/r1"; then echo "rename region1: first-slice region (MODEL, DENSE, ...) unchanged: a consistent rename moves no key and no label"; P "rename region1"; else echo "rename region1: FAILED"; fi
    for b in ua san; do cmp -s "$RN/out.cc" "$RN/out.$b" && { echo "rename $b: output identical to cc"; P "rename $b"; } || echo "rename $b: DIFFERS from cc"; done
fi

if sel integ; then
    # the MIXED file (both generated regions, TYPEV included, + the old
    # header and ENC_ tail) equals the shipped one once whole-line comments are removed;
    # then closure and nativeboot run through tests/snap.sh on a copy of the
    # tree whose kernel/unisa_model.inc is the MIXED file
    I=$T/integ; mkdir -p "$I"
    mix "$T/base.cc" "$I/mixed.inc" 2> "$I/me"; rc=$?
    nc() { awk 'c { if (index($0, "*/")) c = 0; next } /^\/\*/ { if (!index($0, "*/")) c = 1; next } { print }' "$1"; }
    nc "$I/mixed.inc" > "$I/m.nc"; nc kernel/unisa_model.inc > "$I/s.nc"
    if [ $rc = 0 ] && [ -s "$I/m.nc" ] && cmp -s "$I/m.nc" "$I/s.nc" && grep -qxF "$(grep '^char \*TYPEV = ' kernel/unisa_model.inc)" "$I/mixed.inc"; then
        echo "integ mixed=shipped: MIXED ($(wc -c < "$I/mixed.inc" | tr -d ' ') B) = shipped ($(wc -c < kernel/unisa_model.inc | tr -d ' ') B) modulo whole-line comments ($(wc -c < "$I/m.nc" | tr -d ' ') B compared)"; P "integ mixed=shipped"
    else echo "integ mixed=shipped: FAILED (rc $rc): $(cat "$I/me")"; fi
    mkdir -p "$I/tree"
    ( tar cf - --exclude=.git --exclude=corpus --exclude=ujs --exclude=research . ) | ( cd "$I/tree" && tar xf - )
    cp "$I/mixed.inc" "$I/tree/kernel/unisa_model.inc"
    for s in ${INTEG:-closure nativeboot}; do
        t1=$(now)
        case $s in closure) arg="closure examples/*.c tests/c/*.c" ;; *) arg=$s ;; esac   # closure takes the all.sh probes
        ( cd "$I/tree" && B 60 bash tests/snap.sh "$arg" ) > "$I/$s" 2>&1; rc=$?
        echo "integ $s: rc $rc, $(perl -e "printf '%.1f', $(now) - $t1") s: $(tail -1 "$I/$s")"
        [ $rc = 0 ] && P "integ $s"
    done
fi

# ================================================ typekw slice: TYPEV
# tk <awk program> <name>: a modified copy of typekw.tsv (ENVIRON, no escapes)
tk() { LC_ALL=C awk -F'\t' -v OFS='\t' "$1" $K/typekw.tsv > "$TK/$2.tsv"; }
tkrun() { b=$1; o=$2; tf=$3; voc=$4; rm -f "$o"; VOC=$voc; TKW=$tf; gm $b "$o" > "$TK/o" 2>&1; r=$?; unset VOC TKW; return $r; }
if sel tneg; then
    TK=$T/tn; mkdir -p "$TK"
    tk '$2 == "long" { $2 = "char" } { print }' dup
    tk '$2 == "long" { $2 = "" } { print }' empty
    Q='a"b' tk '$2 == "long" { $2 = ENVIRON["Q"] } { print }' quote
    Q='a\b' tk '$2 == "long" { $2 = ENVIRON["Q"] } { print }' bslash
    Q="$(printf 'a\200b')" tk '$2 == "long" { $2 = ENVIRON["Q"] } { print }' hibyte
    Q='a??b' tk '$2 == "long" { $2 = ENVIRON["Q"] } { print }' trigraph
    tk '$2 == "long" { $2 = "0long" } { print }' oct0
    tk '$2 == "long" { $2 = "7long" } { print }' oct7
    awk -F'\t' '$1 != "typekw" { print }' $K/vocab.tsv > "$TK/norow.voc"
    awk -F'\t' '{ print } $1 == "typekw" { print }' $K/vocab.tsv > "$TK/duprow.voc"
    for b in cc ua san; do
        for c in "nofile|$TK/none.tsv: cannot open" \
                 "dup|line 7: duplicate value char" \
                 "empty|TYPEKW value 2 is outside this tool's input domain: an empty value" \
                 "quote|TYPEKW value 2 is outside this tool's input domain: a quote" \
                 "bslash|TYPEKW value 2 is outside this tool's input domain: a backslash" \
                 "hibyte|TYPEKW value 2 is outside this tool's input domain: a control or non-ASCII byte" \
                 "trigraph|TYPEKW value 2 is outside this tool's input domain: \"??\"" \
                 "oct0|TYPEKW value 2 is outside this tool's input domain: a non-first vocab value starting 0-7" \
                 "oct7|TYPEKW value 2 is outside this tool's input domain: a non-first vocab value starting 0-7" \
                 "norow|no typekw row" \
                 "duprow|typekw row given twice"; do
            n=${c%%|*}; want=${c#*|}
            case $n in
            nofile) tf="$TK/none.tsv"; voc=$K/vocab.tsv ;;
            norow|duprow) tf=$K/typekw.tsv; voc="$TK/$n.voc" ;;
            *) tf="$TK/$n.tsv"; voc=$K/vocab.tsv ;;
            esac
            tkrun $b "$TK/out" "$tf" "$voc"; rc=$?
            if [ $rc = 1 ] && grep -qF "$want" "$TK/o" && [ ! -e "$TK/out" ] && ! grep -q 'runtime error\|AddressSanitizer' "$TK/o"; then
                printf "%s\n" "tneg $n $b: rc 1, no output: $(head -1 "$TK/o" | LC_ALL=C tr -c '\n -~' '?')"; P "tneg $n $b"
            else printf "%s\n" "tneg $n $b: FAILED (rc $rc, want 1 and '$want'): $(head -1 "$TK/o" | LC_ALL=C tr -c '\n -~' '?')"; fi
        done
    done
fi

if sel tpos; then
    # a non-first value starting 8 or 9 is legal ("\08long" is \0 then '8');
    # cc compiles the emitted TYPEV line and reads value 2 back
    TK=$T/tp; mkdir -p "$TK"
    for n in oct8 oct9; do
        d=${n#oct}
        tk "\$2 == \"long\" { \$2 = \"${d}long\" } { print }" $n
        for b in cc ua san; do
            tkrun $b "$TK/out" "$TK/$n.tsv" $K/vocab.tsv; rc=$?
            { grep '^char \*TYPEV = ' "$TK/out"; echo 'int puts(const char *); int main(void) { char *p = TYPEV; int i; for (i = 0; i < 2; i = i + 1) while (*p++) ; puts(p); return 0; }'; } > "$TK/x.c" 2>/dev/null
            B 60 cc -w -o "$TK/x" "$TK/x.c" > /dev/null 2>&1 && got=$(B 10 "$TK/x") || got="(no build)"
            if [ $rc = 0 ] && grep -q "^char \*TYPEV = \".*\\\\0${d}long\\\\0" "$TK/out" && [ "$got" = "${d}long" ] && grep -qx '#define NTYPEV 18' "$TK/out"; then
                printf '%s\n' "tpos $n $b: rc 0, TYPEV holds '\\0${d}long\\0'; cc reads value 2 back as '$got'"; P "tpos $n $b"
            else echo "tpos $n $b: FAILED (rc $rc, value 2 '$got'): $(head -1 "$TK/o")"; fi
        done
    done
fi

if sel tfunc; then
    # a legal rename (short -> shrt) and a legal reorder (int <-> char) of a
    # temp typekw.tsv: the output differs from the base in the TYPEV line
    # only, that line is exactly the predicted one, and the compatibility
    # judge REPORTS the difference against lex.TYPEKW (rc 1)
    TK=$T/tf; mkdir -p "$TK"
    tk '$2 == "short" { $2 = "shrt" } { print }' rename
    tk '$2 == "int" { $2 = "char"; print; next } $2 == "char" { $2 = "int" } { print }' reorder
    base=$(grep '^char \*TYPEV = ' "$T/base.cc")
    for n in rename reorder; do
        case $n in
        rename) want=$(printf '%s\n' "$base" | sed 's/\\0short\\0/\\0shrt\\0/') ;;
        reorder) want=$(printf '%s\n' "$base" | sed 's/"int\\0char\\0/"char\\0int\\0/') ;;
        esac
        for b in cc ua san; do tkrun $b "$TK/out.$n.$b" "$TK/$n.tsv" $K/vocab.tsv || echo "tfunc $n $b: genmodel failed: $(head -1 "$TK/o")"; done
        got=$(grep '^char \*TYPEV = ' "$TK/out.$n.cc")
        nd=$(diff "$T/base.cc" "$TK/out.$n.cc" | grep -c '^[<>]')
        if [ "$want" != "$base" ] && [ "$got" = "$want" ] && [ "$nd" = 2 ] && diff "$T/base.cc" "$TK/out.$n.cc" | grep -q '^> char \*TYPEV = '; then
            printf "%s\n" "tfunc $n bytes: only the TYPEV line changed, to exactly the declared order: $(printf '%s' "$got" | cut -c1-60)..."; P "tfunc $n bytes"
        else printf "%s\n" "tfunc $n bytes: FAILED ($nd diff lines): $got"; fi
        B 30 python3 $K/typekw_check.py "$TK/$n.tsv" > "$TK/j.$n" 2>&1; rc=$?
        if [ $rc = 1 ] && grep -q '^typekw_check: DIFFERENCE' "$TK/j.$n"; then printf "%s\n" "tfunc $n judge: rc 1: $(tr '\n' ' ' < "$TK/j.$n" | sed "s|$TK/||")"; P "tfunc $n judge"
        else echo "tfunc $n judge: did NOT report a difference (rc $rc): $(cat "$TK/j.$n")"; fi
        for b in ua san; do cmp -s "$TK/out.$n.cc" "$TK/out.$n.$b" && { echo "tfunc $n $b: output identical to cc"; P "tfunc $n $b"; } || echo "tfunc $n $b: DIFFERS from cc"; done
    done
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
