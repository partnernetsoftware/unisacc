#!/bin/bash
# queuetimeoutcheck (0.0.40, 机房主任 20:44): window timeouts at the REAL release/tools/queue.sh never turn green.
# A fake window launcher (QUEUE_WORKTREE/tests/term.sh) returns a scripted rc per window and records each launch;
# it writes one result into the state the first time, so state conservation across timed-out windows is visible.
#   142 (window watchdog) every window, capped by QUEUE_WINDOWS -> queue rc 142, all windows launched, state kept
#   124 (outer kill)                                            -> queue rc 124 after one window, state kept
#   142 then 0                                                  -> rc 0 only because a later window completed
# No real gate runs and nothing waits on a real timeout (each fake window returns at once).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-qtimeout.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "queuetimeout: $*"; exit 1; }
# macOS has no GNU timeout(1); in-repo tests/bound is the watchdog (1..60s), as in queuestartlaunchcheck (e2e961d1)
_BOUND=$("$R/tests/bound" --helper) || { echo "queuetimeout: need tests/bound (native watchdog)"; exit 2; }
D=$T/cand; S=$T/seed; W=$T/wt; mkdir -p "$D" "$S" "$W/tests"
cp "$R/unisacc.com" "$D/unisacc-next.com"   # real product bytes: queue.sh pre-flights the static-image contract on the candidate
 art=$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)
printf '{"artifact_sha256":"%s"}\n' "$art" > "$D/unisacc-next.com.build.json"
printf 'seed' > "$S/unisacc-seed.com"; printf '#!/bin/sh\nexit 0\n' > "$T/ua"; chmod +x "$T/ua"
# the fake launcher: rc sequence from $T/rcs (one per line, last repeats); first window writes a result into GATE_STATE
cat > "$W/tests/term.sh" <<'SH'
#!/bin/sh
T=__T__; n=$(($(cat "$T/launches" 2>/dev/null | wc -l) + 1)); echo launched >> "$T/launches"
for a in "$@"; do case $a in GATE_STATE=*) GS=${a#GATE_STATE=};; esac; done
[ -f "$GS/results.json" ] || printf '{"stamp":{"a":"s"},"results":{"a":{"rc":0}}}\n' > "$GS/results.json"
rc=$(sed -n "${n}p" "$T/rcs"); [ -n "$rc" ] || rc=$(tail -1 "$T/rcs"); exit "$rc"
SH
sed -i.bak "s#__T__#$T#" "$W/tests/term.sh" && rm -f "$W/tests/term.sh.bak"; chmod +x "$W/tests/term.sh"
run() {   # run RCS WINDOWS NAME -> prints "rc launches"; state in $T/q-NAME
  rm -f "$T/launches"; printf '%s\n' $1 > "$T/rcs"; Q=$T/q-$3
  env -u QUEUE_START -u QUEUE_STATE -u QUEUE_BACKUP QUEUE_SELFTEST=1 STAGELOG_RUN=0 QUEUE_WORKTREE="$W" QUEUE_BACKUP="$T/bak-$3" QUEUE_STATE="$Q" \
    QUEUE_WINDOWS="$2" UNISACC_FFI_X86_PROVIDER=/nonexistent "$_BOUND" 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$S" > "$T/out" 2>&1; qrc=$?
  cp "$T/out" "$T/out-$3" || { echo "copy-failed 0"; return; }
  echo "$qrc $(wc -l < "$T/launches" 2>/dev/null | tr -d ' ' || echo 0)"
}
# go RCS WINDOWS NAME: sets $1 (queue rc) and $2 (launches); a helper that printed fewer than two words is a clear failure
# an unrelated live gatequeue.py (outside the fixture worktree) must neither be waited on nor killed by a selftest queue.sh
mkdir -p "$T/stub/tests"; printf 'import time\ntime.sleep(60)\n' > "$T/stub/tests/gatequeue.py"
python3 "$T/stub/tests/gatequeue.py" & stub=$!; trap 'kill $stub 2>/dev/null; rm -rf "$T"' EXIT
go() { local o; o=$(run "$@"); set -- $o; [ $# -ge 2 ] || fail "helper run $* gave no 'rc launches' result ($o): $(cat "$T/out" 2>/dev/null)"; RC=$1; NL=$2; }
kept() { python3 -c "import json,sys; d=json.load(open(sys.argv[1])); sys.exit(0 if d['results'].get('a')=={'rc':0} else 1)" "$T/q-$1/results.json"; }
go "142" 1 one;   [ "$RC" = 142 ] && [ "$NL" = 1 ] || fail "one window, 142: rc=$RC launches=$NL (want 142/1, no second window)"
go "142" 3 cap;   [ "$RC" = 142 ] && [ "$NL" = 3 ] || fail "142 every window: rc=$RC launches=$NL (want 142/3) $(cat "$T/out")"
kept cap || fail "state lost across watchdog-timed-out windows"; grep -q '^final rc=142$' "$T/q-cap/release-queue.log" || fail "final rc 142 not logged"
go "124" 3 kill;       [ "$RC" = 124 ] && [ "$NL" = 1 ] || fail "outer kill 124: rc=$RC launches=$NL (want 124/1)"
kept kill || fail "state lost after an outer kill"
# an admitted start that later times out is not a refusal: no REFUSED line, no refusal log, receipt says fresh
for n in cap kill; do
  [ ! -e "$T/bak-$n/start-refusals.log" ] || fail "timeout ($n) recorded as a start refusal"
  grep -q '^mode=fresh$' "$T/q-$n/start-receipt.txt" || fail "timeout ($n) lost its fresh start receipt"
done
for n in cap kill; do grep -q REFUSED "$T/out-$n" && fail "timeout ($n) output claims a refusal"; done
go "142 0" 5 late;     [ "$RC" = 0 ] && [ "$NL" = 2 ] || fail "142 then 0: rc=$RC launches=$NL (want 0/2)"
go "75 142 1" 5 red;  [ "$RC" = 1 ] && [ "$NL" = 3 ] || fail "75,142 then a red window: rc=$RC launches=$NL (want 1/3)"
# the singleton guard itself still bites without the selftest override (a live queue.sh on the host)
mkdir -p "$T/release/tools"; printf '#!/bin/sh\nsleep 40\n' > "$T/release/tools/queue.sh"; chmod +x "$T/release/tools/queue.sh"
"$T/release/tools/queue.sh" & dummy=$!; trap 'kill $dummy $stub 2>/dev/null; rm -rf "$T"' EXIT; sleep 1
rm -f "$T/launches"; printf '0\n' > "$T/rcs"
env -u QUEUE_START -u QUEUE_SELFTEST QUEUE_WORKTREE="$W" QUEUE_BACKUP="$T/bak-g" QUEUE_STATE="$T/q-g" UNISACC_FFI_X86_PROVIDER=/nonexistent "$_BOUND" 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$S" > "$T/out-g" 2>&1; grc=$?
[ "$grc" = 2 ] && grep -q 'another queue.sh is running' "$T/out-g" || fail "singleton guard did not refuse beside a live queue.sh: rc=$grc $(cat "$T/out-g")"
kill -0 $stub 2>/dev/null || fail "an unrelated gatequeue.py was killed by a selftest queue.sh"
echo "queuetimeout  real queue.sh: one window ending 142 exits 142 with no second window; watchdog 142 to the window cap exits 142 (logged, state kept); an outer kill 124 stops at once with 124 (state kept); 142 then a completed window is 0; 75/142 then a red window is that red rc; a timed-out admitted start is never recorded as a refusal"
