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
# 0.0.36 (owner, 10-09): main checkout only -- no worktrees, no branches.  The queue runs in the
# main checkout under a freeze of main (product inputs untouched until the queue is done).  The
# root unisacc.com pair is replaced by the candidate's; the previous pair is kept as *.prequeue.
W=$R
[ -f "$W/unisacc.com" ] && [ ! -f "$W/unisacc.com.prequeue" ] && cp -p "$W/unisacc.com" "$W/unisacc.com.prequeue" && cp -p "$W/unisacc.com.build.json" "$W/unisacc.com.build.json.prequeue"
cmp -s "$D/unisacc-next.com" "$W/unisacc.com" || { cp -p "$D/unisacc-next.com" "$W/unisacc.com" && cp -p "$D/unisacc-next.com.build.json" "$W/unisacc.com.build.json"; }
# 0.0.21: the corpus suites read corpus/c-testsuite (ignored by git, so absent in a worktree; a
# network clone failed there); CORPUS is not the knob -- warn and diag read it with another meaning
# 0.0.27: launched from an rc worktree, $R has no ignored corpus either -- take it from the main checkout
# 0.0.21 R21-14: a stale build.json beside a new unisacc.com is rewritten by comboot mid-queue
# and invalidates every job that declares it (0.0.20: six exec-driver results)
if [ -f "$W/unisacc.com" ]; then
  want=$(python3 -c "import json;print(json.load(open('$W/unisacc.com.build.json'))['artifact_sha256'])" 2>/dev/null)
  [ "$want" = "$(shasum -a 256 "$W/unisacc.com" | cut -d' ' -f1)" ] || { echo "queue: unisacc.com and its build.json are not a pair; install both from stage 2"; exit 2; }
fi
cd "$W"
[ -s "$SEED/unisacc-seed.com" ] || { echo "no $SEED/unisacc-seed.com"; exit 2; }
[ -x "$UA" ] && [ -s "$D/unisacc-next.com" ] || { echo "missing UA or candidate"; exit 2; }
: "${UNISACC_FFI_X86_PROVIDER:?set UNISACC_FFI_X86_PROVIDER}"
Q=${QUEUE_STATE:-$D.queue}; mkdir -p "$Q"
LOG=$Q/release-queue.log; : > "$LOG"
for i in $(seq 1 300); do
  for k in $(seq 1 90); do
    ps -eo pid,command | grep "[g]atequeue.py" > "$D/.gq" || break
    [ "$k" -ge 60 ] && awk '{print $1}' "$D/.gq" | xargs kill 2>/dev/null
    sleep 2
  done
  env TERM_SH_NOFALLBACK=1 ./tests/term.sh env REALPROG_CACHE="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)/corpus" UNISACC_FFI_X86_PROVIDER="$UNISACC_FFI_X86_PROVIDER" MODEL_COM="$D/unisacc-next.com" UA="$UA" SEED_DIR="$SEED" GATE_STATE="$Q" ./tests/release.sh --com >> "$LOG" 2>&1; rc=$?
  echo "window $i rc=$rc $(date +%H:%M:%S)" >> "$LOG"
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
grep -E "^queue:|final rc|UNVERIFIED" "$LOG" | tail -4
exit "$rc"
