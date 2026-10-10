#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# exec/c/chain.sh FILE... -- source to tape through ONE generic executor
# (exec/c/run.c) and three threshold networks: E2 (preprocess), E1 (lex, typed), E3
# (parse).  Each stage runs as its own process; the reference's tape is only
# compared with, never fed forward. Python constructs the models.
# NETWORK=0 uses reference lookup tables instead of network inference.
#
# Per file, the stages run in order and the first that does not accept stops
# the chain; its stage and first stderr line are kept.  Verdicts:
#   equal        all three accept and the tape equals `ua_ref -S`
#   not-covered  a stage rejects with `not covered: ...`
#   rejected     a stage rejects for another reason (a language error, a
#                stray char) -- listed, not counted as covered or as bad
#   bad          the tape differs; or a stage exits other than 0/1 (bad
#                table, out of steps, killed); or the reference times out or
#                dies on a signal -- a tool failure is never hidden
# With CHAINKEEP=LIST every listed file must be `equal` (and be an input);
# names are compared as strings, so pass them as the list spells them.
# Every step is bounded (AGENTS.md: 60 s).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
UA=${UA:-/tmp/ua_ref}; . "$R/tests/lib.sh"; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
# Gate passes the historical keep set plus the complete C probe glob.  Count
# each path once, while keeping the fixed keep list as a non-regression floor.
: > "$T/inputs"
for f in "$@"; do
    grep -Fxq "$f" "$T/inputs" || printf '%s\n' "$f" >> "$T/inputs"
done
# CHAINSHARD=k/n keeps every n-th distinct input (the gate runs the probe set as
# shards so each job stays under the 60 s ceiling); the keep floor is checked
# only for the files inside this shard.
if [ -n "${CHAINSHARD:-}" ]; then
    awk -v s="${CHAINSHARD%/*}" -v n="${CHAINSHARD#*/}" '(NR-1)%n==s-1' "$T/inputs" > "$T/inputs.k"
    mv "$T/inputs.k" "$T/inputs"
fi
set -- $(cat "$T/inputs")
CHAINKNOWN=${CHAINKNOWN:-exec/c/chain.knownfail}
awk 'NF < 4 || $2 != "R14-8" || $3 != "P1" { exit 1 }' "$CHAINKNOWN" || { echo "chain: malformed knownfail"; exit 1; }
python3 "$R/tests/knownfail.py" keys "$CHAINKNOWN" > "$T/known" || { echo "chain: malformed knownfail"; exit 1; }
for name in $(cat "$T/known"); do
    [ -n "${CHAINSHARD:-}" ] || { grep -Fxq "tests/c/$name" "$T/inputs" || grep -Fxq "examples/$name" "$T/inputs"; } || { echo "chain: missing known probe $name"; exit 1; }
done
b() { "$_BOUND" "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "chain: cc failed"; exit 1; }
# 0.0.40 K5-1d (机房主任 02:35): pp δ through exec/pp/gen-delta.sh (seed-gen first, no Python fallback), with a
# private seed-gen cache inside this chain scratch (overrides an inherited SEED_GEN_DIR; cleared with $T)
b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/pp/gen-delta.sh" "$T/e2.json" >"$T/e2-gen.out" 2>"$T/e2-gen.err" || { echo "chain: E2 gen failed"; exit 1; }
b 60 python3 exec/build/gen.py lex "$T/e1.json" --typed >/dev/null 2>&1 || { echo "chain: E1 gen failed"; exit 1; }
# 0.0.40 K5-1i (机房主任 07:46): parse2 δ through exec/parse2gen/gen-delta.sh, sharing the private seed-gen cache the pp
# step above built cold (same keyed binary; stdout/stderr to $T/e3-gen.{out,err})
b 60 env SEED_GEN_DIR="$T/.seed-gen-cache" sh "$R/exec/parse2gen/gen-delta.sh" "$T/e3.json" >"$T/e3-gen.out" 2>"$T/e3-gen.err" || { echo "chain: E3 gen failed"; exit 1; }
MODEL=tbl
case ${NETWORK:-1} in 0) ;; 1) MODEL=net;; *) echo "NETWORK must be 0 or 1"; exit 2;; esac
for s in e2 e1 e3; do
    b 60 python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl" || { echo "chain: tbl $s failed"; exit 1; }
    if [ "$MODEL" = net ]; then
        b 60 python3 exec/c/net.py "$T/$s.tbl" "$T/$s.net" || exit 1
        b 60 "$T/run" --check-net "$T/$s.tbl" "$T/$s.net" || exit 1
    fi
done
if [ -n "${CHAINKEEP:-}" ]; then
    [ -s "$CHAINKEEP" ] || { echo "chain: CHAINKEEP names an empty list"; exit 1; }
fi
eq=0; nc=0; rj=0; bad=0; known=0; revived=0; : > "$T/equal"
for f in "$@"; do
    b 10 "$UA" "$f" -S -o - > "$T/ref" 2>/dev/null; rr=$?
    stage=""; rc=0; why=""
    in=$f
    # 0.0.28 F1: E1's constructor/destructor records reach E3 through this file (exec/c/run.c)
    export UNISA_ATTRIBUTES="$T/attributes"; rm -f "$UNISA_ATTRIBUTES"
    for s in e2 e1 e3; do
        case $s in
        e2) b 10 "$T/run" "$T/e2.$MODEL" "$in" "$f" "$R/include" > "$T/$s.out" 2> "$T/$s.err"; rc=$? ;;
        e1) b 10 "$T/run" "$T/e1.$MODEL" "$in" "$f" > "$T/$s.out" 2> "$T/$s.err"; rc=$? ;;
        e3) b 10 "$T/run" "$T/e3.$MODEL" "$in" "$f" > "$T/$s.out" 2> "$T/$s.err"; rc=$? ;;
        esac
        [ $rc -eq 0 ] || { stage=$s; why=$(head -1 "$T/$s.err"); break; }
        in=$T/$s.out
    done
    if [ $rr -ge 128 ] || [ $rr -lt 0 ]; then v=bad; why="reference exited $rr"
    elif [ -z "$stage" ]; then
        if [ $rr -eq 0 ] && cmp -s "$T/e3.out" "$T/ref"; then v=equal; else v=bad; why="tape differs (reference rc=$rr)"; fi
    elif [ $rc -ne 1 ]; then v=bad; why="$stage exited $rc: $why"
    else case "$why" in "reject: not covered"*) v=not-covered ;; *) v=rejected ;; esac
    fi
    if grep -Fxq "${f##*/}" "$T/known"; then
        if [ "$v" = bad ] && [ "$why" = "tape differs (reference rc=0)" ]; then
            v=known
        elif [ "$v" = equal ]; then
            v=revived
        else
            v=bad; why="known probe changed verdict: $why"
        fi
    fi
    case $v in
    equal) eq=$((eq+1)); echo "$f" >> "$T/equal" ;;
    known) known=$((known+1)); [ -n "${CHAINV:-}" ] && echo "  known $f  tape differs" ;;
    revived) revived=$((revived+1)); echo "  REVIVED $f  remove it from chain.knownfail" ;;
    not-covered) nc=$((nc+1)) ;;
    rejected) rj=$((rj+1)); [ -n "${CHAINV:-}" ] && echo "  rejected $f  at $stage: $why" ;;
    bad) bad=$((bad+1)); echo "  BAD $f  ${stage:+at $stage: }$why" ;;
    esac
    [ -n "${CHAINV:-}" ] && [ "$v" = not-covered ] && echo "  not-covered $f  at $stage: ${why#reject: }"
done
lost=0
if [ -n "${CHAINKEEP:-}" ]; then
    for k in $(cat "$CHAINKEEP"); do
        [ -n "${CHAINSHARD:-}" ] && ! grep -Fxq "$k" "$T/inputs" && continue
        grep -qx "$k" "$T/equal" || { echo "  LOST $k"; lost=$((lost+1)); }
    done
fi
echo "chain $MODEL  files $#   equal $eq   known $known   revived $revived   not-covered $nc   rejected $rj   bad $bad${CHAINKEEP:+   lost $lost}"
[ $bad -eq 0 ] && [ $revived -eq 0 ] && [ $lost -eq 0 ] && [ $eq -gt 0 ]
