#!/bin/bash
# observationcheck (0.0.40, 机房主任 20:55): RELEASE_SUITES is an observation of named contract suites, refused
# before any warm-up or scheduling when its scope is wrong.  Every refusal exits 66 and leaves no scheduler state;
# a formal run refuses a state that an observation wrote.  Fixture candidate/UA/seed; no gate suite runs here.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-observe.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "observation: $*"; exit 1; }
printf '#!/bin/sh\nexit 0\n' > "$T/model"; cp "$T/model" "$T/ua"; chmod +x "$T/model" "$T/ua"; mkdir -p "$T/seed"; printf s > "$T/seed/unisacc-seed.com"
rel() {   # rel STATE [ENV...] -> rc
  local S=$1; shift
  env -u RELEASE_SUITES -u RELEASE_OUT MODEL_COM="$T/model" UA="$T/ua" SEED_DIR="$T/seed" GATE_STATE="$S" "$@" \
    "$R/tests/release.sh" --com > "$T/out" 2>&1; echo $?
}
noscheduler() { [ ! -e "$1/results.json" ]; }
for c in "unknown:checkrun no-such-suite" "pipeline:checkrun difftest_o-1" "repeat:checkrun checkrun" ; do
  name=${c%%:*}; suites=${c#*:}; S=$T/s-$name
  rc=$(rel "$S" RELEASE_SUITES="$suites"); [ "$rc" = 66 ] && noscheduler "$S" || fail "$name scope not refused before scheduling: rc=$rc $(cat "$T/out")"
done
S=$T/s-out; rc=$(rel "$S" RELEASE_SUITES="checkrun" RELEASE_OUT="$T/out-dir"); [ "$rc" = 66 ] && noscheduler "$S" || fail "RELEASE_OUT accepted for an observation: rc=$rc"
S=$T/s-formal; mkdir -p "$S"; printf '{"observation": true}\n' > "$S/observation.json"
rc=$(rel "$S"); [ "$rc" = 66 ] && noscheduler "$S" || fail "a formal run continued an observation state: rc=$rc $(cat "$T/out")"
grep -q 'never continues it' "$T/out" || fail "formal refusal not explained"
echo "observation  unknown/non-contract/repeated RELEASE_SUITES, RELEASE_OUT with an observation and a formal run on an observation state all exit 66 before any scheduling"
