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
# 0.0.21 R21-14: the two reworks of the 0.0.20 release, caught before freezing
python3 tests/bound.py 30 python3 tests/subtractsafety.py > /tmp/precheck-ss.$$ 2>&1 || { tail -3 /tmp/precheck-ss.$$; echo "  FAIL: subtract-safety (a live file names an archived one)"; bad=1; }
rm -f /tmp/precheck-ss.$$
if [ -f unisacc.com ] && [ -f unisacc.com.build.json ]; then
    want=$(python3 -c "import json;print(json.load(open('unisacc.com.build.json'))['artifact_sha256'])" 2>/dev/null)
    have=$(shasum -a 256 unisacc.com | cut -d' ' -f1)
    [ "$want" = "$have" ] || { echo "  FAIL: unisacc.com and unisacc.com.build.json are not a pair (install both from stage 2)"; bad=1; }
fi
# 0.0.22 P1-B 7: the two external preparations the 0.0.21 queue lost 14 jobs to
if [ ! -s "${UNISACC_FFI_X86_PROVIDER:-/nonexistent}/manifest.json" ]; then
    echo "  FAIL: UNISACC_FFI_X86_PROVIDER has no manifest.json (rebuild: release/RELEASE-PIPELINE.md section 9 step 5b)"; bad=1
fi
if [ "$(limactl list minicon-lnx-x86_64 --format '{{.Status}}' 2>/dev/null)" != Running ]; then
    echo "  FAIL: Lima minicon-lnx-x86_64 is not running (ccinterop skips its lnx/x86_64 cells, and a skip is a failure): limactl start minicon-lnx-x86_64"; bad=1
fi
echo "precheck $([ $bad -eq 0 ] && echo passed || echo FAILED: fix before freezing)"
exit $bad
