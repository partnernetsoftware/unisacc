#!/bin/bash
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
ok=0; bad=0
fail() { bad=$((bad+1)); echo "  FAIL $*"; }

for f in examples/*.c tests/c/*.c tests/c99/*.c corpus/c-testsuite/tests/single-exec/*.c; do
    [ -f "$f" ] || continue
    d="$R/$(dirname "$f")"
    for o in -O0 -O2; do
        (cd "$T" && perl -e 'alarm 5; exec @ARGV' "$UA" $o -I "$d" "$R/$f" -run > "$T/out$o" 2>&1 </dev/null
         echo "rc=$?" >> "$T/out$o")
    done
    if cmp -s "$T/out-O0" "$T/out-O2"; then ok=$((ok+1)); else fail "$f: -O2 runs differently"; fi
done

for f in examples/*.c tests/c/*.c; do
    for t in lnx/x86_64 osx/arm64 win/arm64; do
        "$UA" -O2 "$f" -t "$t" > "$T/p.tape" 2>/dev/null || continue
        python3 -m unisa compile "$T/p.tape" --from-tape -o "$T/p.py" --target "$t" --drive built >/dev/null 2>&1
        "$UA" -O2 "$f" -b "$t" -o "$T/p.ua" 2>/dev/null
        if cmp -s "$T/p.py" "$T/p.ua"; then ok=$((ok+1)); else fail "$f $t: -O2 closure"; fi
    done
done

host=$(host_target)
if [ -n "$host" ]; then
    "$UA" unisacc.c -b "$host" -o "$T/ref"
    "$UA" -O2 unisacc.c -b "$host" -o "$T/o2"
    cp "$T/o2" "$T/o2x"; chmod +x "$T/o2x"
    command -v codesign >/dev/null && codesign -f -s - "$T/o2x" >/dev/null 2>&1
    bound 20 "$T/o2x" unisacc.c -b "$host" -o "$T/o2ref"
    if cmp -s "$T/o2ref" "$T/ref"; then ok=$((ok+1)); else fail "the -O2 compiler writes a different compiler"; fi
    bound 20 "$T/o2x" -O2 unisacc.c -b "$host" -o "$T/o2o2"
    if cmp -s "$T/o2o2" "$T/o2"; then ok=$((ok+1)); else fail "-O2 is not a fixed point"; fi
    printf "  compiler image  -O0 %s B   -O2 %s B\n" "$(wc -c < "$T/ref" | tr -d ' ')" "$(wc -c < "$T/o2" | tr -d ' ')"
fi

echo
echo "opt  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
