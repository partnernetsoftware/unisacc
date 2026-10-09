#!/bin/sh
# warmupcheck: release/tools/warmup.sh marks a built cache only (0.0.38).  Stub steps and a stub
# model key in a private checkout: not-applicable steps cost no window; step 3 is settled only while
# the cache under its real key is valid (a valid cache costs no window, a deleted/damaged one or a
# new key warms again, a run that leaves the cache invalid is a failure); a failure writes no marker
# and retries; a second failure settles COLD by name; Darwin steps 1-2 run one per window.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-warmup.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "warmup: $*"; exit 1; }
mkdir -p "$T/root/tests" "$T/root/exec/c/asm" "$T/root/exec/pipeline" "$T/root/release/tools" "$T/s1" "$T/s2" "$T/s3" "$T/s4"
cp "$R/tests/bound.py" "$T/root/tests/"; cp "$R/release/tools/warmup.sh" "$T/root/release/tools/"; cp "$R/release/tools/stagelog.py" "$T/root/release/tools/"
# model key stub: prints "KEY STATE" from $T/key and $T/state; a successful build makes the cache valid
printf '#!/bin/sh\necho "$(cat %s/key) $(cat %s/state)"\n' "$T" "$T" > "$T/modelkey"; chmod +x "$T/modelkey"
step() { for f in exec/c/warningcheck.sh exec/c/asm/bindprep.sh exec/pipeline/elf.sh; do
           printf '#!/bin/sh\necho ran >> "%s/ran"\n[ %s = 0 ] && [ "%s" = build ] && echo valid > "%s/state"\nexit %s\n' "$T" "$1" "${2:-build}" "$T" "$1" > "$T/root/$f"
           chmod +x "$T/root/$f"; done; }
W="$T/root/release/tools/warmup.sh"
run() { WARMUP_HOST=${H:-Linux} WARMUP_TARGET=Linux/x86_64 WARMUP_MODELKEY="$T/modelkey" "$W" "$@"; }
k=$(printf '%s' "$T/root" | cksum | cut -d' ' -f1)
echo K1 > "$T/key"; echo invalid > "$T/state"
# Linux: 1-2 not applicable; 3 builds -> cache valid -> marker under the key; then settled
step 0; out=$(run "$T/s1" "$T/root"); [ $? -eq 75 ] || fail "build window not 75"
echo "$out" | grep -q "1/3 not applicable" && echo "$out" | grep -q "3/3 done" || fail "na/done not reported: $out"
[ -f "$T/s1/warm.1.$k.na" ] && [ -f "$T/s1/warm.3.K1" ] || fail "markers missing"
run "$T/s1" "$T/root" >/dev/null; [ $? -eq 0 ] || fail "valid cache asked for another window"
# a valid cache costs no window even with no marker yet
: > "$T/ran"; run "$T/s4" "$T/root" >/dev/null; [ $? -eq 0 ] && [ ! -s "$T/ran" ] || fail "already-valid cache was rebuilt"
# cache deleted or damaged (state invalid) despite the marker: warm again
echo invalid > "$T/state"; run "$T/s1" "$T/root" >/dev/null; [ $? -eq 75 ] || fail "damaged cache kept the old marker"
# changed inputs = new key: warm again under that key
echo K2 > "$T/key"; echo invalid > "$T/state"; run "$T/s1" "$T/root" >/dev/null; [ $? -eq 75 ] && [ -f "$T/s1/warm.3.K2" ] || fail "new key reused the old marker"
# a step that exits 0 but leaves the cache invalid is a failure, not a marker
echo K3 > "$T/key"; echo invalid > "$T/state"; step 0 nobuild
mkdir -p "$T/home"; HOME="$T/home" STAGELOG_RUN=wt run "$T/s2" "$T/root" >/dev/null; [ $? -eq 75 ] && [ ! -f "$T/s2/warm.3.K3" ] && [ "$(cat "$T/s2/warm.3.K3.fail")" = 1 ] || fail "rc 0 without a valid cache was marked"
grep '"event":"end"' "$T/home/.unisacc/stagelog/events.jsonl" | grep -q '"rc":3' || fail "stage event kept rc 0 for an invalid cache"
# second failure: COLD by name, then settled
step 3; out=$(run "$T/s2" "$T/root"); [ $? -eq 75 ] || fail "second failure not 75"
echo "$out" | grep -q "FAILED twice" && [ -f "$T/s2/warm.3.K3.cold" ] && [ ! -f "$T/s2/warm.3.K3" ] || fail "COLD not named: $out"
run "$T/s2" "$T/root" >/dev/null; [ $? -eq 0 ] || fail "COLD state asked for another window"
# identity query fails: UNKNOWN, nothing written, nothing built, re-judged next time
: > "$T/key"; : > "$T/state"; : > "$T/ran"; out=$(run "$T/s4" "$T/root")
echo "$out" | grep -q "3/3 UNKNOWN" && [ ! -s "$T/ran" ] && ! ls "$T/s4" | grep -q '^warm\.3\.\.\|cold' || fail "identity failure was not UNKNOWN: $out"
# Darwin runs steps 1 and 2, one per window
echo K4 > "$T/key"; echo valid > "$T/state"; step 0; : > "$T/ran"
H=Darwin run "$T/s3" "$T/root" >/dev/null; H=Darwin run "$T/s3" "$T/root" >/dev/null
[ "$(wc -l < "$T/ran")" -eq 2 ] && [ -f "$T/s3/warm.1.$k" ] && [ -f "$T/s3/warm.2.$k" ] || fail "Darwin steps not run one per window"
echo "warmup  na costs no window, step 3 settled only by a valid cache under its real key (deleted/new key re-warm, rc0 without cache fails and its event says rc 3), identity failure UNKNOWN, failure retries, second failure COLD, Darwin one step per window"
