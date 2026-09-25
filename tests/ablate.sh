#!/bin/bash
# Is each stage's answer USED? [A-36]
#
# stages.sh proves a stage is ASKED.  That is weaker than it looks: an answer
# can be asked for and thrown away, and every answer-checking suite stays
# green.  Here each stage -- each head of a multi-head stage -- is ablated in
# turn (UNISA_ABLATE rotates its answer to the next class, unisa/oracle.py),
# and the six images of every probe are rebuilt.  If no byte of any image
# changes and nothing is refused, the answer did not matter: that stage is
# reported UNUSED, and the suite fails.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
PROBES=${*:-examples/hello.c examples/fib.c examples/switch.c examples/struct.c tests/c/b_float.c tests/c/b_file.c tests/c/b_argv.c}
TARGETS="lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64"
ABL=${ABL:-"pp lex parse type scope irsel enc reloc regmap abi.sysno abi.arg0 abi.arg1 abi.arg2 abi.ret abi.gate abi.nrreg"}
# SHARD=k/n keeps every n-th ablation starting at the k-th: each ablation is
# 42 compiles, and all sixteen in one run were over the 60 s ceiling
# (AGENTS.md).  all.sh runs 1/4 .. 4/4.
if [ -n "${SHARD:-}" ]; then
    ABL=$(echo $ABL | tr ' ' '\n' | awk -v k="${SHARD%/*}" -v n="${SHARD#*/}" '(NR-1)%n==k-1' | tr '\n' ' ')
fi
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

images() {   # images <tag> -> one line per probe/target: sha or REFUSED
    for f in $PROBES; do
        for t in $TARGETS; do
            # a rotated answer can send the compiler into a loop: every
            # compile has a watchdog, and a timeout counts as REFUSED
            if perl -e 'alarm 60; exec @ARGV' python3 -m unisa compile "$f" \
                   -o "$T/img" --target "$t" --drive built >/dev/null 2>&1; then
                echo "$f $t $(shasum < "$T/img" | cut -c1-16)"
            else
                echo "$f $t REFUSED"
            fi
        done
    done > "$T/$1"
}

# The harness first proves itself: for each item, the oracle must hand back a
# DIFFERENT answer, without raising, on a real key.  Only then can a refused
# compile be blamed on the compiler -- an earlier version crashed in its own
# hook, and every "refusal" it counted was its own.
ABL="$ABL" python3 - <<'PY' || { echo "ablate: the harness itself is broken"; exit 1; }
import os, unisa.oracle as O
from unisa.__main__ import _built
from unisa.gold import STAGES
o = O.Oracle(_built(), drive="built")
for item in os.environ["ABL"].split():
    st, _, hd = item.partition(".")
    key = next(iter(STAGES[st].keys()))
    O._ABLATE.clear(); base = o.ask(st, key)
    O._ABLATE[st] = hd; abl = o.ask(st, key)
    O._ABLATE.clear()
    pick = (lambda a: a if not hd else a[hd])
    assert pick(base) != pick(abl), item
PY
images base
used=0; unused=0
for a in $ABL; do
    UNISA_ABLATE=$a images "abl"
    n=$(diff "$T/base" "$T/abl" | grep '^>' | grep -vc REFUSED)
    r=$(grep -c REFUSED "$T/abl")
    # changed bytes, or a compile the wrong answer made the compiler refuse
    # (the harness proved itself above, so a refusal is the compiler's)
    if [ "$n" -gt 0 ] || [ "$r" -gt 0 ]; then
        used=$((used+1)); printf "  used    %-12s %3d of %d images changed, %d refused\n" "$a" "$n" "$(wc -l < "$T/base")" "$r"
    else
        unused=$((unused+1)); printf "  UNUSED  %-12s no image changed (%d refused)\n" "$a" "$r"
    fi
done
echo
echo "ablate  used $used   unused $unused"
[ "$unused" -eq 0 ]
