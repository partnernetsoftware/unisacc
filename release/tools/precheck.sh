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
    # 0.0.23 F1: a cold cache (new sources) cost 55 s in 0.0.21 and 0.0.22; the first pass builds it,
    # the second is the check -- a timeout only fails when the warm pass times out too
    if [ "$rc" -eq 142 ]; then echo "warm-up: $w cold pass timed out (${t}s); checking warm"; t0=$(date +%s); python3 tests/bound.py 55 env $w >/dev/null 2>&1; rc=$?; t=$(( $(date +%s) - t0 )); fi
    echo "warm-up: $w rc=$rc ${t}s"; [ "$rc" -eq 0 ] || bad=1
done
# 0.0.25 P8: every row of the version plan says how it ends before the version commit.
# precheck runs after the version commit (freezecheck needs it), so ledgercheck's default
# (version.h + 1) already names the next plan: check the version being released.
V=$(sed -n 's/.*UNISACC_VERSION "\(.*\)".*/\1/p' src/version.h)
python3 tests/bound.py 20 python3 tests/ledgercheck.py --final "plans/v$V.md" > /tmp/precheck-lg.$$ 2>&1 || { cat /tmp/precheck-lg.$$; echo "  FAIL: settle the deferral ledger in the plan first"; bad=1; }
rm -f /tmp/precheck-lg.$$
# 0.0.25 P1: after the version commit only `fix:` commits may touch the product closure.
python3 tests/bound.py 20 python3 tests/freezecheck.py > /tmp/precheck-fz.$$ 2>&1 || { cat /tmp/precheck-fz.$$; echo "  FAIL: new work inside the freeze window (move it to the next version, or mark a red fix with fix:)"; bad=1; }
rm -f /tmp/precheck-fz.$$
# 0.0.24: a header change (kill, ttyname) left exec/facts/pp-autoinc-gen.tsv stale; the candidate
# had to be rebuilt.  Exported fact tables must match their generators before freezing.
python3 tests/bound.py 30 python3 exec/facts/export.py --check > /tmp/precheck-ex.$$ 2>&1 || { tail -3 /tmp/precheck-ex.$$; echo "  FAIL: exported facts stale (run exec/facts/export.py, re-record graphhash)"; bad=1; }
rm -f /tmp/precheck-ex.$$
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
# 0.0.27 C2: the lnx/x86_64 ccinterop cells run on release-check ccinterop-x86, not a local Lima VM
# 0.0.34 rc4: each contract window runs inside Terminal.app (term.sh), as the release queue does: from a bare
# shell tools took 51 s against 44 s (first-exec scans of every new binary)
# 0.0.28 R2: the contract layer (ledger, freeze, gate layers, fresh-order, ...) before sealing --
# 0.0.27 moved its rc twice for tool reds that only the queue found after the seal.
# Measured 2026-10-05: 46 suites, 6 windows of 55 s at jobs 2, 192 s; the cap of 10 leaves room
cq=$(mktemp -d "${TMPDIR:-/tmp}/precheck-contract.XXXXXX"); crc=75; w=0
while [ "$crc" -eq 75 ] && [ "$w" -lt 10 ]; do w=$((w+1)); ./tests/term.sh env ${UA:+"UA=$UA"} ${MODEL_COM:+"MODEL_COM=$MODEL_COM"} ${SEED_DIR:+"SEED_DIR=$SEED_DIR"} ${UNISACC_FFI_X86_PROVIDER:+"UNISACC_FFI_X86_PROVIDER=$UNISACC_FFI_X86_PROVIDER"} python3 tests/gatequeue.py --layer contract --state "$cq" --jobs 2 > "$cq/w$w.log" 2>&1; crc=$?; done
if [ "$crc" -ne 0 ]; then grep '^DONE' "$cq"/w*.log | grep -v ' rc=0 ' | cut -c1-200; tail -1 "$cq/w$w.log"; echo "  FAIL: contract layer (rc=$crc after $w windows; logs $cq)"; bad=1
else echo "contract layer: passed in $w window(s)"; rm -rf "$cq"; fi
# 0.0.28 E17: a known-fail line written during this version is debt the version must close before sealing
V=$(sed -n 's/.*UNISACC_VERSION "\(.*\)".*/\1/p' src/version.h)
kf=$(grep -n "0\.0\.${V##*.} \|${V} " tests/*.knownfail tests/*.knownwrong exec/c/*.knownfail 2>/dev/null | grep -v '^[^:]*:[0-9]*:#' | grep -v '^tests/pyfront.knownfail:')   # the Python control group is not shipped
if [ -n "$kf" ]; then printf '%s\n' "$kf" | cut -c1-200; echo "  FAIL: known-fail lines tagged $V remain (close them or move them to the next version with a reason)"; bad=1; fi
echo "precheck $([ $bad -eq 0 ] && echo passed || echo FAILED: fix before freezing)"
exit $bad
