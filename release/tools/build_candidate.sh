#!/bin/bash
# build_candidate.sh DIR UA -- shared, six targets, pack-prep-1..3, pack-models, pack-driver; each step bounded 58 s; stops at
# the first failure.  UA must be a same-source reference (tests/build_ref.sh SRC OUT from this tree).
set -u; D=${1:?candidate dir}; UA=${2:?same-source reference}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "${ROOT:-$R}"; mkdir -p "$D"; D=$(cd "$D" && pwd)
for step in shared osx/arm64 osx/x86_64 lnx/arm64 lnx/x86_64 win/arm64 win/x86_64; do
  log=$D/step-${step//\//-}.log; s0=$(date +%s)
  python3 tests/bound.py 58 make model-com MODEL_DIR=$D MODEL_STEP=$step > $log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S) $(( $(date +%s)-s0 ))s"; [ $rc -eq 0 ] || exit $rc
done
rm -rf $D/model-cache   # fresh construction per candidate
for step in pack-prep-1 pack-prep-2 pack-prep-3 pack-models pack-driver; do
  s0=$(date +%s)
  python3 tests/bound.py 58 make model-com MODEL_DIR=$D MODEL_STEP=$step UA=$UA > $D/step-$step.log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S) $(( $(date +%s)-s0 ))s"; [ $rc -eq 0 ] || exit $rc
done
shasum -a 256 $D/unisacc-next.com; python3 exec/c/provenance.py check $D/unisacc-next.com
# comboot seed pair (RELEASE-PIPELINE §9 step 3): $D/seed/unisacc-seed.com(.build.json); use SEED_DIR=$D/seed
mkdir -p $D/seed && cp $D/unisacc-next.com $D/seed/unisacc-seed.com && cp $D/unisacc-next.com.build.json $D/seed/unisacc-seed.com.build.json && echo "seed: $D/seed"
