#!/bin/bash
# queue.sh CAND_DIR UA SEED_DIR -- drive tests/release.sh windows until the queue completes (R18-11 ④).
# Generated per release from arguments (0.0.16/0.0.17 copied the previous script and once
# pointed at a wrong seed directory).  Never launches a window while a gatequeue is alive;
# a stray one older than 120 s is killed by PID.  State and log live BESIDE the candidate dir
# (CAND_DIR.queue/, CAND_DIR.queue/release-queue.log): rebuilding a candidate empties CAND_DIR, and
# 0.0.22 lost its whole queue state that way (0.0.23 F6).  QUEUE_STATE overrides the directory.
set -u
D=${1:?candidate dir}; UA=${2:?same-source reference}; SEED=${3:?seed dir}
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
# R19-0: the queue runs in its own detached worktree at the commit it started
# on, so commits to main (plans, prd, other agents' work) never reach it.
W=${QUEUE_WORKTREE:-/tmp/unisacc-queue-$(git rev-parse --short HEAD)}
if [ ! -d "$W" ]; then
  git worktree add -q --detach "$W" HEAD || { echo "queue: cannot create worktree $W"; exit 2; }
  # untracked inputs the gates read: the installed product (comboot writes it;
  # absent at the start, its appearance invalidated the 0.0.19 queue at 408/415)
fi
# 0.0.33: also for a QUEUE_WORKTREE made beforehand (build_candidate flow): it got no corpus links
# (diag saw 0 damaged-corpus programs) and kept the root .com instead of the candidate pair
# 0.0.31: the root holds the SIGNED public .com beside the unsigned build.json (not a pair);
# the queue tests the candidate, so install the stage-2 pair from CAND_DIR
cp -p "$D/unisacc-next.com" "$W/unisacc.com" && cp -p "$D/unisacc-next.com.build.json" "$W/unisacc.com.build.json"
# 0.0.21: the corpus suites read corpus/c-testsuite (ignored by git, so absent in a worktree; a
# network clone failed there); CORPUS is not the knob -- warn and diag read it with another meaning
# 0.0.27: launched from an rc worktree, $R has no ignored corpus either -- take it from the main checkout
M=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
for c in "$M"/corpus/*; do [ -e "$W/corpus/${c##*/}" ] || { mkdir -p "$W/corpus"; ln -s "$c" "$W/corpus/${c##*/}"; }; done
# 0.0.21 R21-14: a stale build.json beside a new unisacc.com is rewritten by comboot mid-queue
# and invalidates every job that declares it (0.0.20: six exec-driver results)
if [ -f "$W/unisacc.com" ]; then
  want=$(python3 -c "import json;print(json.load(open('$W/unisacc.com.build.json'))['artifact_sha256'])" 2>/dev/null)
  [ "$want" = "$(shasum -a 256 "$W/unisacc.com" | cut -d' ' -f1)" ] || { echo "queue: unisacc.com and its build.json are not a pair; install both from stage 2"; exit 2; }
fi
cd "$W"
[ -s "$SEED/unisacc-seed.com" ] || { echo "no $SEED/unisacc-seed.com"; exit 2; }
[ -x "$UA" ] && [ -s "$D/unisacc-next.com" ] || { echo "missing UA or candidate"; exit 2; }
# 0.0.37: the provider feeds only the Darwin/arm64 Rosetta suites (gate.sh); elsewhere it is not read
[ "$(uname -s)/$(uname -m)" != Darwin/arm64 ] || : "${UNISACC_FFI_X86_PROVIDER:?set UNISACC_FFI_X86_PROVIDER}"
UNISACC_FFI_X86_PROVIDER=${UNISACC_FFI_X86_PROVIDER:-}
# 0.0.37 speedup ①: one driver only -- a second queue.sh on this host would race this one's
# stray-gatequeue kill; refuse instead of competing (0.0.36: hand-written resume loops).
# match only processes RUNNING the script (it is argv0 or argv1), never a shell whose -c text names it;
# skip this script, its children and its parent
others=$(ps -eo pid=,ppid=,args= | awk -v me=$$ -v pp=$PPID '$1!=me && $2!=me && $1!=pp && ($3 ~ /queue\.sh$/ || $4 ~ /release\/tools\/queue\.sh$/) {print $1}')
[ -z "$others" ] || { echo "queue: another queue.sh is running (pid $others); resume with that one, do not start a second"; exit 2; }
# 0.0.37 speedup ①: 0.0.36 lost 539 and 713 results to a sleeping display (apps-real and term.sh
# need it).  Hold display and idle sleep for the life of this script; macOS only, elsewhere a no-op.
command -v caffeinate >/dev/null 2>&1 && { caffeinate -dims -w $$ & }
Q=${QUEUE_STATE:-$D.queue}
# 0.0.37 speedup ③: a persistent copy outside the candidate dir and /tmp, written only between
# windows (no gatequeue alive, nothing half-written).  A restart that finds no state restores it;
# gatequeue still re-checks every stamp, so a restored result is reused only for identical inputs.
B=${QUEUE_BACKUP:-$HOME/.unisacc/queue-backup/$(basename "$D")-$(shasum -a 256 "$D/unisacc-next.com" | cut -c1-12)}
# the swap below leaves a moment with state.old and no state: restore from whichever full generation exists
for g in state state.old; do
  if [ ! -d "$Q" ] && [ -d "$B/$g" ]; then mkdir -p "$Q" && cp -pR "$B/$g/." "$Q/" && echo "queue: restored state from $B/$g (stamps re-checked by gatequeue)"; fi
done
mkdir -p "$Q"
backup() {   # swap only after a complete copy: a failed copy keeps the previous backup
  mkdir -p "$B" && rm -rf "$B/state.new" && cp -pR "$Q" "$B/state.new" && cp -p "$D/unisacc-next.com.build.json" "$B/" \
    && rm -rf "$B/state.old" && { [ ! -d "$B/state" ] || mv "$B/state" "$B/state.old"; } && mv "$B/state.new" "$B/state" && rm -rf "$B/state.old" \
    || echo "queue: backup to $B failed (previous copy kept)" >> "$LOG"; }
LOG=$Q/release-queue.log; [ -f "$LOG" ] && echo "--- restart $(date +%H:%M:%S)" >> "$LOG" || : > "$LOG"
# 0.0.38 P6: per-window wall clock into the private stage log (outside the repo); a logging failure
# never stops the queue.  STAGELOG_RUN=0 turns it off.
SL=${STAGELOG_RUN-q-$(shasum -a 256 "$D/unisacc-next.com" | cut -c1-12)}; [ "$SL" = 0 ] && SL=
qid=; [ -z "$SL" ] || qid=$(python3 "$R/release/tools/stagelog.py" begin --run "$SL" --phase queue --source-commit "$(git rev-parse --short HEAD)" 2>/dev/null) || :
for i in $(seq 1 300); do
  for k in $(seq 1 90); do
    ps -eo pid,command | grep "[g]atequeue.py" > "$D/.gq" || break
    [ "$k" -ge 60 ] && awk '{print $1}' "$D/.gq" | xargs kill 2>/dev/null
    sleep 2
  done
  wid=; [ -z "$SL" ] || wid=$(python3 "$R/release/tools/stagelog.py" begin --run "$SL" --phase queue --subphase "window-$i" --parent-id "$qid" 2>/dev/null) || :
  env TERM_SH_NOFALLBACK=1 ./tests/term.sh env REALPROG_CACHE="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)/corpus" UNISACC_FFI_X86_PROVIDER="$UNISACC_FFI_X86_PROVIDER" MODEL_COM="$D/unisacc-next.com" UA="$UA" SEED_DIR="$SEED" GATE_STATE="$Q" ./tests/release.sh --com >> "$LOG" 2>&1; rc=$?
  echo "window $i rc=$rc $(date +%H:%M:%S)" >> "$LOG"; backup
  [ -z "$SL" ] || python3 "$R/release/tools/stagelog.py" end --id "$wid" --rc "$rc" --execution-status "window-rc-$rc" >/dev/null 2>&1 || :
  case $rc in 0) break;; 75|142) ;; *) tail -25 "$LOG" | grep -q BlockingIOError && continue; break;; esac
  # 0.0.28 R1 (owner rule): the 5-minute load at or above QUEUE_LOAD_MAX (6) for ten minutes pauses
  # the queue at a window boundary until it falls below QUEUE_LOAD_RESUME (4); results are kept.
  # Built in because the 0.0.27 external watcher missed once and killed the wrong PID once.
  l5=$(sysctl -n vm.loadavg 2>/dev/null | awk '{print int($3)}'); [ -n "$l5" ] || l5=$(awk '{print int($2)}' /proc/loadavg 2>/dev/null || echo 0)
  if [ "$l5" -ge "${QUEUE_LOAD_MAX:-6}" ]; then hot=${hot:-$(date +%s)}; else hot=; fi
  if [ -n "$hot" ] && [ $(( $(date +%s) - hot )) -ge "${QUEUE_LOAD_SECS:-600}" ]; then
    echo "paused: 5-min load >= ${QUEUE_LOAD_MAX:-6} for ${QUEUE_LOAD_SECS:-600} s $(date +%H:%M:%S)" >> "$LOG"
    while [ "$(sysctl -n vm.loadavg 2>/dev/null | awk '{print int($3)}')" -ge "${QUEUE_LOAD_RESUME:-4}" ]; do sleep 60; done
    echo "resumed $(date +%H:%M:%S)" >> "$LOG"; hot=
  fi
  [ $((i % 40)) -eq 0 ] && osascript -e 'tell application "Terminal" to close (every window whose busy is false)' >/dev/null 2>&1
done
echo "final rc=$rc" >> "$LOG"
[ -z "$qid" ] || python3 "$R/release/tools/stagelog.py" end --id "$qid" --rc "$rc" >/dev/null 2>&1 || :
grep -E "^queue:|final rc|UNVERIFIED" "$LOG" | tail -4
exit "$rc"
