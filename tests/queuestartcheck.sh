#!/bin/bash
# queuestartcheck (0.0.40): default fresh leaves a same-artifact backup untouched and starts empty;
# explicit resume binds backup build.json artifact + complete state, wrong/incomplete/mismatched
# sources and a live Q with results fail closed; unknown mode refused.  Fixture only -- no real queue.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-queuestart.XXXXXX"); trap 'rm -rf "$T"' EXIT
fail() { echo "queuestart: $*"; exit 1; }
QS="$R/release/tools/queuestart.sh"
[ -x "$QS" ] || fail "queuestart.sh missing or not executable"

# candidate + matching / mismatched backups
mkdir -p "$T/cand" "$T/bak/state" "$T/bak-wrong/state" "$T/bak-incomplete/state"
printf 'PRODUCT-BYTES' > "$T/cand/unisacc-next.com"
art=$(shasum -a 256 "$T/cand/unisacc-next.com" | cut -d' ' -f1)
printf '{"artifact_sha256":"%s","commit":"c1"}\n' "$art" > "$T/cand/unisacc-next.com.build.json"
printf '{"artifact_sha256":"%s","commit":"c1"}\n' "$art" > "$T/bak/unisacc-next.com.build.json"
printf '{"jobs":{},"results":{"probe":{"rc":0}}}\n' > "$T/bak/state/results.json"
echo MARKER > "$T/bak/state/probe.log"
printf '{"artifact_sha256":"%s","commit":"other"}\n' "$(printf OTHER | shasum -a 256 | cut -d' ' -f1)" > "$T/bak-wrong/unisacc-next.com.build.json"
printf '{"jobs":{},"results":{"probe":{"rc":0}}}\n' > "$T/bak-wrong/state/results.json"
printf '{"artifact_sha256":"%s","commit":"c1"}\n' "$art" > "$T/bak-incomplete/unisacc-next.com.build.json"
# incomplete: state dir exists but no results.json
echo orphan > "$T/bak-incomplete/state/orphan.log"

# 1) fresh + backup present + Q absent: empty start, backup untouched, no restore import
Q1=$T/q-fresh; rm -rf "$Q1"
out=$("$QS" "$Q1" "$T/bak" "$T/cand" fresh 2>&1) || fail "fresh rc $? : $out"
echo "$out" | grep -q 'left untouched' || fail "fresh did not report untouched backup: $out"
echo "$out" | grep -q 'restored state from\|resumed state from' && fail "fresh restored/resumed: $out"
[ -f "$Q1/results.json" ] && fail "fresh imported results.json"
[ -f "$Q1/probe.log" ] && fail "fresh imported probe.log"
[ -f "$T/bak/state/results.json" ] && [ -f "$T/bak/state/probe.log" ] || fail "fresh deleted or moved backup"
grep -q '^mode=fresh$' "$Q1/start-receipt.txt" && grep -q '^restored=no$' "$Q1/start-receipt.txt" \
  && grep -q "^artifact_sha256=$art\$" "$Q1/start-receipt.txt" || fail "fresh receipt: $(cat "$Q1/start-receipt.txt")"

# 2) fresh again with existing empty Q: keeps Q, still no backup read
out=$("$QS" "$Q1" "$T/bak" "$T/cand" fresh 2>&1) || fail "fresh-existing rc $? : $out"
echo "$out" | grep -q 'backup not read\|using existing state' || fail "fresh-existing message: $out"
[ -f "$Q1/results.json" ] && fail "fresh-existing imported results"

# 3) resume matching complete backup: imports, receipt bound, backup preserved
Q3=$T/q-resume; rm -rf "$Q3"
out=$("$QS" "$Q3" "$T/bak" "$T/cand" resume 2>&1) || fail "resume rc $? : $out"
echo "$out" | grep -q "resumed state from $T/bak/state" || fail "resume message: $out"
[ -f "$Q3/results.json" ] && grep -q MARKER "$Q3/probe.log" || fail "resume did not import state"
[ -f "$T/bak/state/results.json" ] || fail "resume removed backup"
grep -q '^mode=resume$' "$Q3/start-receipt.txt" && grep -q '^restored=yes$' "$Q3/start-receipt.txt" \
  && grep -q "^source=$T/bak/state\$" "$Q3/start-receipt.txt" || fail "resume receipt: $(cat "$Q3/start-receipt.txt")"

# 4) resume wrong artifact → refuse, no import
Q4=$T/q-wrong; rm -rf "$Q4"
"$QS" "$Q4" "$T/bak-wrong" "$T/cand" resume >"$T/err-wrong" 2>&1; rc=$?
[ "$rc" -eq 2 ] || fail "wrong artifact rc=$rc"
grep -q 'resume refused: backup artifact' "$T/err-wrong" || fail "wrong artifact msg: $(cat "$T/err-wrong")"
[ -f "$Q4/results.json" ] && fail "wrong artifact imported results"

# 5) resume incomplete (no results.json) → refuse
Q5=$T/q-inc; rm -rf "$Q5"
"$QS" "$Q5" "$T/bak-incomplete" "$T/cand" resume >"$T/err-inc" 2>&1; rc=$?
[ "$rc" -eq 2 ] || fail "incomplete rc=$rc"
grep -q 'no complete state' "$T/err-inc" || fail "incomplete msg: $(cat "$T/err-inc")"

# 6) resume with live Q results → refuse mixing
Q6=$T/q-live; rm -rf "$Q6"; mkdir -p "$Q6"; printf '{"results":{"x":{"rc":1}}}\n' > "$Q6/results.json"
"$QS" "$Q6" "$T/bak" "$T/cand" resume >"$T/err-live" 2>&1; rc=$?
[ "$rc" -eq 2 ] || fail "live mix rc=$rc"
grep -q 'live state already' "$T/err-live" || fail "live mix msg: $(cat "$T/err-live")"
grep -q '"x"' "$Q6/results.json" || fail "live results were overwritten"

# 7) resume missing build.json → refuse
Q7=$T/q-nobuild; rm -rf "$Q7"; mkdir -p "$T/bak-nobuild/state"
printf '{"results":{}}\n' > "$T/bak-nobuild/state/results.json"
"$QS" "$Q7" "$T/bak-nobuild" "$T/cand" resume >"$T/err-nb" 2>&1; rc=$?
[ "$rc" -eq 2 ] || fail "no build.json rc=$rc"
grep -q 'no build.json' "$T/err-nb" || fail "no build.json msg: $(cat "$T/err-nb")"

# 8) unknown mode → refuse
Q8=$T/q-bad; rm -rf "$Q8"
"$QS" "$Q8" "$T/bak" "$T/cand" auto >"$T/err-bad" 2>&1; rc=$?
[ "$rc" -eq 2 ] || fail "bad mode rc=$rc"
grep -q 'must be fresh or resume' "$T/err-bad" || fail "bad mode msg: $(cat "$T/err-bad")"

# 9) default MODE arg (=fresh) with backup: same as (1)
Q9=$T/q-default; rm -rf "$Q9"
out=$("$QS" "$Q9" "$T/bak" "$T/cand" 2>&1) || fail "default rc $? : $out"
echo "$out" | grep -q 'left untouched' || fail "default not fresh: $out"
[ -f "$Q9/results.json" ] && fail "default imported results"

# 10) queue.sh wiring: QUEUE_START=fresh must not emit the old auto-restore line when only setup runs.
#     Exercise via queuestart (queue.sh's sole restore path); old phrase must be absent from queue.sh.
grep -q 'queuestart.sh' "$R/release/tools/queue.sh" || fail "queue.sh does not call queuestart.sh"
grep -n 'restored state from' "$R/release/tools/queue.sh" && fail "queue.sh still auto-restores"
grep -q 'QUEUE_START' "$R/release/tools/queue.sh" || fail "queue.sh missing QUEUE_START"

echo "queuestart  fresh leaves same-artifact backup untouched (empty Q, receipt restored=no); resume binds artifact+complete state with receipt; wrong/incomplete/missing-build/live-mix/unknown-mode refuse rc2; queue.sh defaults to fresh (no auto-restore)"
