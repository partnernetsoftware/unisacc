#!/bin/sh
# warmupcheck: release/tools/warmup.sh marks a built cache only (0.0.38).  Stub steps in a private
# checkout: not-applicable steps cost no window, success writes the marker, one failure writes none
# and retries, a second failure settles COLD (named), and a settled state needs no more windows.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-warmup.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "warmup: $*"; exit 1; }
mkdir -p "$T/root/tests" "$T/root/exec/c/asm" "$T/root/exec/pipeline" "$T/root/release/tools" "$T/s1" "$T/s2" "$T/s3"
cp "$R/tests/bound.py" "$T/root/tests/"; cp "$R/release/tools/warmup.sh" "$T/root/release/tools/"
step() { printf '#!/bin/sh\necho ran >> "%s/ran"\nexit %s\n' "$T" "$1" > "$T/root/exec/c/warningcheck.sh"
         printf '#!/bin/sh\necho ran >> "%s/ran"\nexit %s\n' "$T" "$1" > "$T/root/exec/c/asm/bindprep.sh"
         printf '#!/bin/sh\necho ran >> "%s/ran"\nexit %s\n' "$T" "$1" > "$T/root/exec/pipeline/elf.sh"
         chmod +x "$T/root/exec/c/warningcheck.sh" "$T/root/exec/c/asm/bindprep.sh" "$T/root/exec/pipeline/elf.sh"; }
W="$T/root/release/tools/warmup.sh"
k=$(printf '%s\n%s' "$T/root" "" | cksum | cut -d' ' -f1)
# Linux: steps 1-2 not applicable, step 3 succeeds -> one window, marker, then settled
step 0
out=$(WARMUP_HOST=Linux "$W" "$T/s1" "$T/root"); [ $? -eq 75 ] || fail "success window not 75"
echo "$out" | grep -q "1/3 not applicable" && echo "$out" | grep -q "3/3 done" || fail "na/done not reported: $out"
[ -f "$T/s1/warm.1.$k.na" ] && [ -f "$T/s1/warm.3.$k" ] || fail "markers missing"
WARMUP_HOST=Linux "$W" "$T/s1" "$T/root" >/dev/null; [ $? -eq 0 ] || fail "settled state asked for another window"
# failure: no marker, counted; second failure -> COLD; then settled
step 3
WARMUP_HOST=Linux "$W" "$T/s2" "$T/root" >/dev/null; [ $? -eq 75 ] || fail "failed step not 75"
[ ! -f "$T/s2/warm.3.$k" ] && [ ! -f "$T/s2/warm.3.$k.cold" ] && [ "$(cat "$T/s2/warm.3.$k.fail")" = 1 ] || fail "first failure wrote a marker"
out=$(WARMUP_HOST=Linux "$W" "$T/s2" "$T/root"); [ $? -eq 75 ] || fail "second failure not 75"
echo "$out" | grep -q "FAILED twice" && [ -f "$T/s2/warm.3.$k.cold" ] && [ ! -f "$T/s2/warm.3.$k" ] || fail "COLD not named: $out"
WARMUP_HOST=Linux "$W" "$T/s2" "$T/root" >/dev/null; [ $? -eq 0 ] || fail "COLD state asked for another window"
# Darwin runs steps 1 and 2 (one per window); another checkout warms again
step 0; : > "$T/ran"
WARMUP_HOST=Darwin "$W" "$T/s3" "$T/root" >/dev/null; WARMUP_HOST=Darwin "$W" "$T/s3" "$T/root" >/dev/null
[ "$(wc -l < "$T/ran")" -eq 2 ] && [ -f "$T/s3/warm.1.$k" ] && [ -f "$T/s3/warm.2.$k" ] || fail "Darwin steps not run one per window"
cp -R "$T/root" "$T/root2"; WARMUP_HOST=Linux "$T/root2/release/tools/warmup.sh" "$T/s1" "$T/root2" >/dev/null
[ $? -eq 75 ] || fail "a new checkout reused another checkout's markers"
# a changed model input in the same checkout warms again (marker bound to the inputs)
( cd "$T/root" && git init -q && mkdir -p exec weights && echo 1 > exec/x && git add -A && git -c user.name=t -c user.email=t@t commit -qm a ) || fail "git fixture"
step 0; WARMUP_HOST=Linux "$W" "$T/s1" "$T/root" >/dev/null; [ $? -eq 75 ] || fail "git-tracked checkout reused the untracked marker"
WARMUP_HOST=Linux "$W" "$T/s1" "$T/root" >/dev/null; [ $? -eq 0 ] || fail "same inputs did not settle"
echo 2 > "$T/root/exec/x"; WARMUP_HOST=Linux "$W" "$T/s1" "$T/root" >/dev/null; [ $? -eq 75 ] || fail "changed exec/ input kept the old warm marker"
echo "warmup  na costs no window, success marks, failure counts without a marker, second failure COLD, settled states 0, per-checkout markers"
