#!/bin/bash
# queuetimercheck (0.0.40, 机房主任 21:38): a REAL timer ends a queue window.  Real release/tools/queue.sh, the real
# tests/term.sh and tests/bound.py in the window worktree; only the window body (tests/release.sh) is a fixture that
# sleeps longer than TERM_SH_ALARM.  bound.py must kill it (rc 142 from the inner watchdog, not an outer 124), the queue
# must stop after exactly one window with that rc, never print a pass, and never record a start refusal.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-qtimer.XXXXXX"); trap '[ -n "${QUEUETIMER_KEEP:-}" ] && cp -pR "$T" "$QUEUETIMER_KEEP"; rm -rf "$T"' EXIT
fail() { echo "queuetimer: $*"; exit 1; }
ALARM=${QUEUETIMER_ALARM:-4}
_BOUND=$("$R/tests/bound" --helper) || { echo "queuetimer: need tests/bound"; exit 2; }
D=$T/cand; S=$T/seed; W=$T/wt; mkdir -p "$D" "$S" "$W/tests"
printf c > "$D/unisacc-next.com"; printf '{"artifact_sha256":"%s"}\n' "$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)" > "$D/unisacc-next.com.build.json"
printf s > "$S/unisacc-seed.com"; printf '#!/bin/sh\nexit 0\n' > "$T/ua"; chmod +x "$T/ua"
cp "$R/tests/term.sh" "$R/tests/bound.py" "$W/tests/"
# the window body: records its start and its parent's argv, sleeps past the alarm, and would record a finish it must
# never reach
cat > "$W/tests/release.sh" <<BODY
#!/bin/sh
date +%s > "$T/body-started"
ps -o pid=,ppid=,args= -p \$PPID > "$T/body-parent" 2>&1
echo fixture body started
sleep $((ALARM * 4))
date +%s > "$T/body-finished"
echo "gate  suites 1   failed 0"
exit 0
BODY
chmod +x "$W/tests/release.sh"
Q=$T/state; t0=$(date +%s)
env -u QUEUE_START -u QUEUE_SUITES STAGELOG_RUN="${QUEUETIMER_STAGELOG:-0}" TERM_SH=0 TERM_SH_ALARM=$ALARM QUEUE_WORKTREE="$W" QUEUE_STATE="$Q" QUEUE_BACKUP="$T/bak" \
  QUEUE_WINDOWS=1 UNISACC_FFI_X86_PROVIDER=/nonexistent "$_BOUND" 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$S" > "$T/out" 2>&1; rc=$?
took=$(( $(date +%s) - t0 ))
[ "$rc" = 142 ] || fail "queue rc=$rc, want 142 from the inner watchdog ($(cat "$T/out"))"
[ -f "$T/body-started" ] || fail "the window body never started"
[ ! -e "$T/body-finished" ] || fail "the window body ran past the alarm"
grep -qE "bound\.py $ALARM " "$T/body-parent" || fail "the window body was not run under bound.py $ALARM: $(cat "$T/body-parent")"
[ "$took" -ge "$ALARM" ] && [ "$took" -lt 45 ] || fail "took ${took}s: not the ${ALARM}s inner alarm"
[ "$(grep -c '^window ' "$Q/release-queue.log")" = 1 ] && grep -q '^window 1 rc=142 ' "$Q/release-queue.log" && grep -q '^final rc=142$' "$Q/release-queue.log" || fail "window lines: $(grep -E '^window|^final' "$Q/release-queue.log")"
grep -q '^mode=fresh$' "$Q/start-receipt.txt" && grep -q '^restored=no$' "$Q/start-receipt.txt" && grep -q '^source=none$' "$Q/start-receipt.txt" || fail "receipt: $(cat "$Q/start-receipt.txt")"
grep -qE 'failed 0|OBSERVATION ONLY|completed' "$Q/release-queue.log" && fail "a pass/completion line appeared"
[ ! -e "$T/bak/start-refusals.log" ] && ! grep -q REFUSED "$T/out" || fail "a timed-out admitted start was recorded as a refusal"
echo "queuetimer  real term.sh -> bound.py ${ALARM}s alarm killed a longer window body: queue rc 142 (inner, not 124) after one window in ${took}s, body never finished, no pass line, fresh/restored=no/source=none, no refusal"
