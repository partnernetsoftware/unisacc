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
S=$T/s-newline; rc=$(rel "$S" RELEASE_SUITES="checkrun
../bad"); [ "$rc" = 66 ] && noscheduler "$S" || fail "a second line of names was ignored: rc=$rc $(cat "$T/out")"
S=$T/s-cr; rc=$(rel "$S" RELEASE_SUITES="$(printf 'checkrun\r')"); [ "$rc" = 66 ] || fail "a CR in RELEASE_SUITES accepted: rc=$rc"
for c in "unknown:checkrun no-such-suite" "pipeline:checkrun difftest_o-1" "repeat:checkrun checkrun" "blank:   " "glob:check*" "glob2:docedit ?heckrun" "path:../checkrun" ; do
  name=${c%%:*}; suites=${c#*:}; S=$T/s-$name
  rc=$(rel "$S" RELEASE_SUITES="$suites"); [ "$rc" = 66 ] && noscheduler "$S" || fail "$name scope not refused before scheduling: rc=$rc $(cat "$T/out")"
done
# the scheduler's own selection check must count its exit status: a correct list with rc 1 is still a refusal
mkdir -p "$T/shim"; real=$(command -v python3)
printf '#!/bin/sh\nfor a in "$@"; do [ "$a" = --list-selection ] && { echo checkrun; exit 1; }; done\nexec "%s" "$@"\n' "$real" > "$T/shim/python3"; chmod +x "$T/shim/python3"
S=$T/s-listrc; rc=$(rel "$S" RELEASE_SUITES="checkrun" PATH="$T/shim:$PATH"); [ "$rc" = 66 ] && noscheduler "$S" || fail "a failing scheduler selection check was accepted: rc=$rc $(cat "$T/out")"
S=$T/s-out; rc=$(rel "$S" RELEASE_SUITES="checkrun" RELEASE_OUT="$T/out-dir"); [ "$rc" = 66 ] && noscheduler "$S" || fail "RELEASE_OUT accepted for an observation: rc=$rc"
S=$T/s-formal; mkdir -p "$S"; printf '{"observation": true}\n' > "$S/observation.json"
rc=$(rel "$S"); [ "$rc" = 66 ] && noscheduler "$S" || fail "a formal run continued an observation state: rc=$rc $(cat "$T/out")"
grep -q 'never continues it' "$T/out" || fail "formal refusal not explained"
# the other direction: an observation never continues a formal state, and never re-scopes an observation state
S=$T/s-formal2; mkdir -p "$S"; printf '{"stamp":{},"jobs":{"x":["y"]},"results":{"x":{"rc":0}}}\n' > "$S/results.json"; before=$(cd "$S" && shasum -a 256 * | sort)
rc=$(rel "$S" RELEASE_SUITES="checkrun"); [ "$rc" = 66 ] && [ "$(cd "$S" && shasum -a 256 * | sort)" = "$before" ] && [ ! -e "$S/observation.json" ] || fail "an observation continued a formal state: rc=$rc $(cat "$T/out")"
S=$T/s-rescope; mkdir -p "$S"; printf '{"observation": true, "suites": ["checkrun"], "acceptance": false}\n' > "$S/observation.json"; before=$(cd "$S" && shasum -a 256 * | sort)
rc=$(rel "$S" RELEASE_SUITES="checkrun docedit"); [ "$rc" = 66 ] && [ "$(cd "$S" && shasum -a 256 * | sort)" = "$before" ] || fail "an observation state was re-scoped: rc=$rc $(cat "$T/out")"
S=$T/s-badobs; mkdir -p "$S"; printf 'not json' > "$S/observation.json"; rc=$(rel "$S" RELEASE_SUITES="checkrun"); [ "$rc" = 66 ] || fail "an unreadable observation.json was continued: rc=$rc"
# queue.sh: 65 (completed observation) and 66 (refused observation) end the queue after one window -- never the
# 75/142 continue path, never read as a pass by the window loop (fake window launcher, real queue.sh)
_BOUND=$("$R/tests/bound" --helper) || { echo "observation: need tests/bound"; exit 2; }
D=$T/cand; W=$T/wt; mkdir -p "$D" "$W/tests"; printf c > "$D/unisacc-next.com"
printf '{"artifact_sha256":"%s"}\n' "$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)" > "$D/unisacc-next.com.build.json"
# a process whose command TEXT mentions gatequeue.py (not running it) must not make queue.sh wait
bash -c 'sleep 40 # tests/gatequeue.py --com' & mention=$!
for want in 65 66; do
  printf '#!/bin/sh\necho x >> "%s/launches-%s"\nexit %s\n' "$T" "$want" "$want" > "$W/tests/term.sh"; chmod +x "$W/tests/term.sh"
  env -u QUEUE_START STAGELOG_RUN=0 QUEUE_WORKTREE="$W" QUEUE_STATE="$T/q$want" QUEUE_BACKUP="$T/b$want" QUEUE_SUITES=checkrun QUEUE_WINDOWS=5 \
    UNISACC_FFI_X86_PROVIDER=/nonexistent "$_BOUND" 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$T/seed" > "$T/out" 2>&1; rc=$?
  [ "$rc" = "$want" ] && [ "$(wc -l < "$T/launches-$want" | tr -d ' ')" = 1 ] || fail "queue.sh on window rc $want: exit $rc, launches $(wc -l < "$T/launches-$want")"
done
kill "$mention" 2>/dev/null; wait "$mention" 2>/dev/null
echo "observation  unknown/non-contract/repeated/blank/glob/path-like/multi-line RELEASE_SUITES, RELEASE_OUT with an observation, a scheduler selection check that exits non-zero, a formal run on an observation state, an observation on a formal state, a re-scoped or unreadable observation state all exit 66 before any scheduling, states untouched; queue.sh ends after one window on 65/66 with that rc, and does not wait on a process that merely mentions gatequeue.py"
