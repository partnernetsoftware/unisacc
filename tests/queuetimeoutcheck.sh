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
D=$T/cand; S=$T/seed; W=$T/wt; mkdir -p "$D" "$S" "$W/tests"
printf 'candidate' > "$D/unisacc-next.com"; art=$(shasum -a 256 "$D/unisacc-next.com" | cut -d' ' -f1)
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
  env -u QUEUE_START -u QUEUE_STATE -u QUEUE_BACKUP STAGELOG_RUN=0 QUEUE_WORKTREE="$W" QUEUE_BACKUP="$T/bak-$3" QUEUE_STATE="$Q" \
    QUEUE_WINDOWS="$2" UNISACC_FFI_X86_PROVIDER=/nonexistent timeout 50 "$R/release/tools/queue.sh" "$D" "$T/ua" "$S" > "$T/out" 2>&1
  echo "$? $(wc -l < "$T/launches" | tr -d ' ')"
}
kept() { python3 -c "import json,sys; d=json.load(open(sys.argv[1])); sys.exit(0 if d['results'].get('a')=={'rc':0} else 1)" "$T/q-$1/results.json"; }
set -- $(run "142" 1 one);   [ "$1" = 142 ] && [ "$2" = 1 ] || fail "one window, 142: rc=$1 launches=$2 (want 142/1, no second window)"
set -- $(run "142" 3 cap);   [ "$1" = 142 ] && [ "$2" = 3 ] || fail "142 every window: rc=$1 launches=$2 (want 142/3) $(cat "$T/out")"
kept cap || fail "state lost across watchdog-timed-out windows"; grep -q '^final rc=142$' "$T/q-cap/release-queue.log" || fail "final rc 142 not logged"
set -- $(run "124" 3 kill);       [ "$1" = 124 ] && [ "$2" = 1 ] || fail "outer kill 124: rc=$1 launches=$2 (want 124/1)"
kept kill || fail "state lost after an outer kill"
set -- $(run "142 0" 5 late);     [ "$1" = 0 ] && [ "$2" = 2 ] || fail "142 then 0: rc=$1 launches=$2 (want 0/2)"
set -- $(run "75 142 1" 5 red);  [ "$1" = 1 ] && [ "$2" = 3 ] || fail "75,142 then a red window: rc=$1 launches=$2 (want 1/3)"
echo "queuetimeout  real queue.sh: one window ending 142 exits 142 with no second window; watchdog 142 to the window cap exits 142 (logged, state kept); an outer kill 124 stops at once with 124 (state kept); 142 then a completed window is 0; 75/142 then a red window is that red rc"
