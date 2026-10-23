#!/bin/bash
# build_candidate.sh DIR UA -- shared, six targets, pack-prep-1..3, pack-models, pack-driver; each step bounded 58 s; stops at
# the first failure.  UA must be a same-source reference (tests/build_ref.sh SRC OUT from this tree).
set -u; D=${1:?candidate dir}; UA=${2:?same-source reference}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "${ROOT:-$R}"
# All elapsed displays use one monotonic millisecond clock; wall-clock adjustments do not affect them.
monotonic_ms() { python3 -c 'import time; print(time.monotonic_ns() // 1000000)'; }
elapsed_text() { printf '%d.%03d' "$(( $1 / 1000 ))" "$(( $1 % 1000 ))"; }
STEP_TOTAL_MS=0
# step_report STEP RC START_MONOTONIC_MS STATUS_FILE: bound returns 142 when its watchdog killed the step (the bound
# status file keeps the raw wait status); any other non-zero rc is the step's own failure.  Both are named.
step_report() {
    local st=$1 rc=$2 s0=$3 sf=$4 el=$(( $(monotonic_ms) - $3 ))
    STEP_TOTAL_MS=$(( STEP_TOTAL_MS + el ))
    if [ "$rc" -eq 142 ]; then echo "step $st WATCHDOG: killed by bound after ${STEP_BOUND:-58} s (raw wait status $(cat "$sf" 2>/dev/null || echo unknown)) $(elapsed_text "$el")s"
    else echo "step $st rc=$rc $(elapsed_text "$el")s"; fi
}
# 0.0.40 (机房主任 22:45 A): the H1 table the exit table will use is settled before anything is written or built
python3 "$R/release/tools/h1source.py" . || exit 2
mkdir -p "$D"; D=$(cd "$D" && pwd)
# a detached worktree has no gitignored unisacc.com / seed (the shared step runs unisacc.com): name the fix instead of exit 127
# Only SEED_C=1 builds tbl/net with the installed product; the existing
# Python constructor route does not read that pair (first-host bootstrap).
if [ "${SEED_C:-1}" = 1 ]; then
for f in unisacc.com unisacc.com.build.json; do [ -f "$f" ] || { echo "build_candidate: $PWD/$f missing; copy the installed pair from the main checkout (cp -p unisacc.com unisacc.com.build.json unisacc-seed.com* WT/)" >&2; exit 2; }; done
fi
for step in shared osx/arm64 osx/x86_64 lnx/arm64 lnx/x86_64 win/arm64 win/x86_64; do
  log=$D/step-${step//\//-}.log; s0=$(monotonic_ms)
  tests/bound --status $D/status-${step//\//-} ${STEP_BOUND:-58} make model-com MODEL_DIR=$D MODEL_STEP=$step > $log 2>&1; rc=$?
  step_report "$step" "$rc" "$s0" "$D/status-${step//\//-}"; [ $rc -eq 0 ] || exit $rc
done
# fresh construction per candidate: the old cache moves into the .trash of its root through the one guard (tests/safedel.cx)
[ -n "$D" ] && sh "$R/tests/cxbin" "$R/tests/safedel.cx" -- "$D/model-cache" || { echo "build_candidate: $D/model-cache not cleared (refused)"; exit 3; }
for step in pack-prep-1 pack-prep-2 pack-prep-3 pack-models pack-driver; do
  s0=$(monotonic_ms)
  tests/bound --status $D/status-$step ${STEP_BOUND:-58} make model-com MODEL_DIR=$D MODEL_STEP=$step UA=$UA > $D/step-$step.log 2>&1; rc=$?
  step_report "$step" "$rc" "$s0" "$D/status-$step"; [ $rc -eq 0 ] || exit $rc
done
# Sum only the bounded model steps above, excluding setup, identity checks and seed build.
echo "model steps total $(elapsed_text "$STEP_TOTAL_MS")s"
shasum -a 256 $D/unisacc-next.com
# B5: the seed-built ident checks the record when the build made it; Python only as fallback
if [ -x $D/seedbin/ident ]; then $D/seedbin/ident . check $D/unisacc-next.com; else python3 exec/c/provenance.py check $D/unisacc-next.com; fi
# comboot seed pair (RELEASE-PIPELINE §9 step 3): $D/seed/unisacc-seed.com(.build.json); use SEED_DIR=$D/seed.
# 0.0.30 S1: the seed is built without Python (host cc + exported source + seed/ape.c), not copied from unisacc-next.com.
release/tools/build_seed.sh $D/seed || exit $?
