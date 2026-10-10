#!/bin/bash
# queuestartlaunchcheck (0.0.40, 机房主任 20:22/20:37): the REAL release/tools/queue.sh entry refuses a wrong start
# before any window: a fake window launcher (QUEUE_WORKTREE/tests/term.sh) records every launch.  Refusals must
# exit 2 with zero launches and the state dir untouched; a clean fresh start must launch.  No real gate runs.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-qlaunch.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "queuestartlaunch: $*"; exit 1; }
D=$T/cand; S=$T/seed; W=$T/wt; B=$T/bak; mkdir -p "$D" "$S" "$W/tests" "$B/state"
printf 'candidate' > "$D/unisacc-next.com"; art=$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)
printf '{"artifact_sha256":"%s"}\n' "$art" > "$D/unisacc-next.com.build.json"
printf 'seed' > "$S/unisacc-seed.com"; printf '#!/bin/sh\nexit 0\n' > "$T/ua"; chmod +x "$T/ua"
# the fake launcher: one line per window, then a finished, green window
printf '#!/bin/sh\necho launched >> "%s/launches"\nexit 0\n' "$T" > "$W/tests/term.sh"; chmod +x "$W/tests/term.sh"
printf '{"artifact_sha256":"%s"}\n' "$art" > "$B/unisacc-next.com.build.json"
printf '{"stamp":{},"results":{"old":{"rc":0}}}\n' > "$B/state/results.json"
q() {   # q STATE_DIR [ENV...] -- one real queue.sh start; prints rc
  local Q=$1; shift
  env STAGELOG_RUN=0 QUEUE_WORKTREE="$W" QUEUE_BACKUP="$B" QUEUE_STATE="$Q" UNISACC_FFI_X86_PROVIDER=/nonexistent "$@" \
    timeout 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$S" > "$T/out" 2>&1; echo $?
}
launches() { [ -f "$T/launches" ] && wc -l < "$T/launches" | tr -d ' ' || echo 0; }
snap() { [ -d "$1" ] && (cd "$1" && find . -type f -exec shasum -a 256 {} + | sort) || echo absent; }
# 1. fresh into a live state: refused at the entry, no window, state untouched
Q=$T/q-live; mkdir -p "$Q"; printf '{"results":{"x":{"rc":0}}}\n' > "$Q/results.json"; before=$(snap "$Q")
rc=$(q "$Q"); [ "$rc" = 2 ] && [ "$(launches)" = 0 ] && [ "$(snap "$Q")" = "$before" ] || fail "fresh into a live state: rc=$rc launches=$(launches) $(cat "$T/out")"
grep -q 'REFUSED (fresh start)' "$T/out" || fail "fresh refusal not reported: $(cat "$T/out")"
# 2. resume from a backup for another artifact: refused, no window, Q not created, refusal logged
printf '{"artifact_sha256":"%064d"}\n' 0 > "$B/unisacc-next.com.build.json"
Q=$T/q-foreign; rc=$(q "$Q" QUEUE_START=resume)
[ "$rc" = 2 ] && [ "$(launches)" = 0 ] && [ ! -e "$Q" ] || fail "foreign backup resume: rc=$rc launches=$(launches) $(cat "$T/out")"
grep -q 'REFUSED resume' "$B/start-refusals.log" || fail "foreign backup refusal not logged"
printf '{"artifact_sha256":"%s"}\n' "$art" > "$B/unisacc-next.com.build.json"
# 3. an unknown start mode: refused, no window
rc=$(q "$T/q-mode" QUEUE_START=auto); [ "$rc" = 2 ] && [ "$(launches)" = 0 ] || fail "unknown mode: rc=$rc launches=$(launches)"
# 4. a clean default start launches windows and never imports the backup (positive)
Q=$T/q-fresh; rc=$(q "$Q")
[ "$(launches)" -ge 1 ] || fail "clean fresh start launched no window: rc=$rc $(cat "$T/out")"
grep -q '^restored=no$' "$Q/start-receipt.txt" && [ ! -e "$Q/results.json" ] || fail "fresh start imported the backup"
echo "queuestartlaunch  real queue.sh entry: fresh-into-live, foreign-backup resume and unknown mode exit 2 with zero window launches, state untouched/not created, refusal logged; a clean fresh start launches without importing the backup"
