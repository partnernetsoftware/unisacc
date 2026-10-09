#!/bin/bash
# build_candidate.sh DIR UA -- shared, six targets, pack-prep-1..3, pack-models, pack-driver; each step bounded 58 s; stops at
# the first failure.  UA must be a same-source reference (tests/build_ref.sh SRC OUT from this tree).
set -u; D=${1:?candidate dir}; UA=${2:?same-source reference}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "${ROOT:-$R}"; mkdir -p "$D"; D=$(cd "$D" && pwd)
# a detached worktree has no gitignored unisacc.com / seed (the shared step runs unisacc.com): name the fix instead of exit 127
# Only SEED_C=1 builds tbl/net with the installed product; the existing
# Python constructor route does not read that pair (first-host bootstrap).
if [ "${SEED_C:-1}" = 1 ]; then
for f in unisacc.com unisacc.com.build.json; do [ -f "$f" ] || { echo "build_candidate: $PWD/$f missing; copy the installed pair from the main checkout (cp -p unisacc.com unisacc.com.build.json unisacc-seed.com* WT/)" >&2; exit 2; }; done
fi
for step in shared osx/arm64 osx/x86_64 lnx/arm64 lnx/x86_64 win/arm64 win/x86_64; do
  log=$D/step-${step//\//-}.log; s0=$(date +%s)
  tests/bound 58 make model-com MODEL_DIR=$D MODEL_STEP=$step > $log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S) $(( $(date +%s)-s0 ))s"; [ $rc -eq 0 ] || exit $rc
done
rm -rf $D/model-cache   # fresh construction per candidate
for step in pack-prep-1 pack-prep-2 pack-prep-3 pack-models pack-driver; do
  s0=$(date +%s)
  tests/bound 58 make model-com MODEL_DIR=$D MODEL_STEP=$step UA=$UA > $D/step-$step.log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S) $(( $(date +%s)-s0 ))s"; [ $rc -eq 0 ] || exit $rc
done
shasum -a 256 $D/unisacc-next.com
# B5: the seed-built ident checks the record when the build made it; Python only as fallback
if [ -x $D/seedbin/ident ]; then $D/seedbin/ident . check $D/unisacc-next.com; else python3 exec/c/provenance.py check $D/unisacc-next.com; fi
# comboot seed pair (RELEASE-PIPELINE §9 step 3): $D/seed/unisacc-seed.com(.build.json); use SEED_DIR=$D/seed.
# 0.0.30 S1: the seed is built without Python (host cc + exported source + seed/ape.c), not copied from unisacc-next.com.
release/tools/build_seed.sh $D/seed || exit $?
