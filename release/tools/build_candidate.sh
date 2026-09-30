#!/bin/bash
# build_candidate.sh DIR UA -- shared, six targets, pack-models, pack-driver; each step bounded 58 s; stops at
# the first failure.  UA must be a same-source reference (tests/build_ref.sh SRC OUT from this tree).
set -u; D=${1:?candidate dir}; UA=${2:?same-source reference}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "${ROOT:-$R}"
for step in shared osx/arm64 osx/x86_64 lnx/arm64 lnx/x86_64 win/arm64 win/x86_64; do
  log=$D/step-${step//\//-}.log
  python3 tests/bound.py 58 make model-com MODEL_DIR=$D MODEL_STEP=$step > $log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S)"; [ $rc -eq 0 ] || exit $rc
done
for step in pack-models pack-driver; do
  python3 tests/bound.py 58 make model-com MODEL_DIR=$D MODEL_STEP=$step UA=$UA > $D/step-$step.log 2>&1; rc=$?
  echo "step $step rc=$rc $(date +%H:%M:%S)"; [ $rc -eq 0 ] || exit $rc
done
shasum -a 256 $D/unisacc-next.com; python3 exec/c/provenance.py check $D/unisacc-next.com
