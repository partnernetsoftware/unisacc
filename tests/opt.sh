#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# -O1/-O2: the optimised tape means what the walker's tape means. [H1]
#
#   1. every probe and c-testsuite program: -run at -O2 prints and exits
#      exactly as at -O0
#   2. closure at -O2: the C front end's optimised tape, lowered by the
#      Python back end, is byte-identical to the C back end's image
#   3. the compiler built at -O2 writes the same compiler as the reference,
#      and rebuilds itself at -O2 (a fixed point)
#
# Under 60 s (AGENTS.md): -run only, nothing written to disk is executed
# except the one self-built compiler.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
SELF="$T/unisacc.flat.c"
bound 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
OPT_PART=${OPT_PART:-all}; SHARD=${SHARD:-1/1}
case "$OPT_PART" in all|run|closure|self) ;; *) echo 'invalid OPT_PART' >&2; exit 2;; esac
[[ "$SHARD" =~ ^[1-9][0-9]{0,5}/[1-9][0-9]{0,5}$ ]] || exit 2
SK=${SHARD%/*}; SN=${SHARD#*/}; [ "$SK" -le "$SN" ] || exit 2
pick() { index=$((index+1)); [ $(((index-1)%SN)) -eq $((SK-1)) ]; }
ok=0; bad=0
fail() { bad=$((bad+1)); echo "  FAIL $*"; }

if [ "$OPT_PART" = all ] || [ "$OPT_PART" = run ]; then
index=0
for f in examples/*.c tests/c/*.c tests/c99/*.c corpus/c-testsuite/tests/single-exec/*.c; do
    [ -f "$f" ] || continue
    pick || continue
    d="$R/$(dirname "$f")"; valid=1
    for o in -O0 -O2; do
        if ! bound 10 "$UA" "$o" -I "$d" "$R/$f" -c >"$T/tape$o" 2>"$T/compile$o.log" || [ ! -s "$T/tape$o" ]; then
            # Known non-C99 corpus inputs remain explicit exclusions, never passes.
            if [[ "$f" == corpus/* ]] && grep -qs "^$(basename "$f" .c)[[:space:]]" tests/corpus.knownfail; then
                echo "  KNOWN $f $o: non-C99 input refused"
            else fail "$f $o: tape build failed or empty"; cat "$T/compile$o.log"; fi
            valid=0; continue
        fi
        rm -f "$T/status$o"
        (cd "$T" && "$_BOUND" --status "$T/status$o" 5 "$UA" $o -I "$d" "$R/$f" -run > "$T/out$o" 2>&1 </dev/null
         echo "rc=$?" >> "$T/out$o")
    done
    [ "$valid" -eq 1 ] || continue
    normal=1
    for o in -O0 -O2; do
        status=$(cat "$T/status$o" 2>/dev/null)
        if [ -z "$status" ] || [ $((status & 127)) -ne 0 ]; then normal=0; fi
    done
    if [ "$normal" -ne 1 ]; then fail "$f: execution timeout/signal"; continue; fi
    if cmp -s "$T/out-O0" "$T/out-O2"; then ok=$((ok+1)); else fail "$f: -O2 runs differently"; fi
done
fi

if [ "$OPT_PART" = all ] || [ "$OPT_PART" = closure ]; then
index=0
for f in examples/*.c tests/c/*.c; do
    pick || continue
    for t in lnx/x86_64 osx/arm64 win/arm64; do
        bound 10 "$UA" -O2 "$f" -t "$t" > "$T/p.tape" 2>"$T/tape.log" && [ -s "$T/p.tape" ] || { fail "$f $t: tape build"; cat "$T/tape.log"; continue; }
        bound 20 python3 -m unisa compile "$T/p.tape" --from-tape -o "$T/p.py" --target "$t" --drive built >"$T/python.log" 2>&1 || { fail "$f $t: Python image build"; cat "$T/python.log"; continue; }
        bound 10 "$UA" -O2 "$f" -b "$t" -o "$T/p.ua" >"$T/ua.log" 2>&1 || { fail "$f $t: C image build"; cat "$T/ua.log"; continue; }
        [ -s "$T/p.py" ] && [ -s "$T/p.ua" ] || { fail "$f $t: empty image"; continue; }
        if cmp -s "$T/p.py" "$T/p.ua"; then ok=$((ok+1)); else fail "$f $t: -O2 closure"; fi
    done
done
fi

host=$(host_target)
if { [ "$OPT_PART" = all ] || [ "$OPT_PART" = self ]; } && [ -n "$host" ]; then
    bound 15 "$UA" "$SELF" -b "$host" -o "$T/ref" || exit 1
    bound 15 "$UA" -O2 "$SELF" -b "$host" -o "$T/o2" || exit 1
    [ -s "$T/ref" ] && [ -s "$T/o2" ] || exit 1
    cp "$T/o2" "$T/o2x"; chmod +x "$T/o2x"
    if command -v codesign >/dev/null; then bound 8 codesign -f -s - "$T/o2x" >/dev/null 2>&1 || exit 1; fi
    bound 15 "$T/o2x" "$SELF" -b "$host" -o "$T/o2ref" || exit 1
    if cmp -s "$T/o2ref" "$T/ref"; then ok=$((ok+1)); else fail "the -O2 compiler writes a different compiler"; fi
    bound 15 "$T/o2x" -O2 "$SELF" -b "$host" -o "$T/o2o2" || exit 1
    if cmp -s "$T/o2o2" "$T/o2"; then ok=$((ok+1)); else fail "-O2 is not a fixed point"; fi
    printf "  compiler image  -O0 %s B   -O2 %s B\n" "$(wc -c < "$T/ref" | tr -d ' ')" "$(wc -c < "$T/o2" | tr -d ' ')"
fi

echo
echo "opt part=$OPT_PART SHARD=$SHARD  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
