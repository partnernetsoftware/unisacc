#!/bin/bash
# precheck.sh UA -- R19-0 (3): run BEFORE freezing and building a candidate.
# Each check is one the 0.0.18 release found only after a candidate existed,
# costing a rebuild each time.  Budgets keep 30% margin under the 55 s bounds.
set -u; UA=${1:?same-source reference}; R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
bad=0
t0=$(date +%s); python3 tests/bound.py 55 python3 -m unisa build-weights >/dev/null 2>&1 || bad=1; t=$(( $(date +%s) - t0 ))
echo "build-weights ${t}s (local budget 19 s: release-check runs it under 55 s on a runner about twice as slow)"; [ "$t" -le 19 ] || { echo "  FAIL: hosted runners take about twice as long"; bad=1; }
[ -z "$(git status --porcelain -- weights)" ] || { echo "  FAIL: build-weights changed weights/ (commit the construction first)"; bad=1; }
for w in "CORE_ASM_ARCH=arm64 exec/c/asm/bindprep.sh" "CORE_ASM_ARCH=x86_64 exec/c/asm/bindprep.sh" "UA=$UA exec/c/warningcheck.sh ua Wall"; do
    t0=$(date +%s); python3 tests/bound.py 55 env $w >/dev/null 2>&1; rc=$?; t=$(( $(date +%s) - t0 ))
    echo "warm-up: $w rc=$rc ${t}s"; [ "$rc" -eq 0 ] || bad=1
done
echo "precheck $([ $bad -eq 0 ] && echo passed || echo FAILED: fix before freezing)"
exit $bad
