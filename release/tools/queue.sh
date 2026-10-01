#!/bin/bash
# queue.sh CAND_DIR UA SEED_DIR -- drive tests/release.sh windows until the queue completes (R18-11 ④).
# Generated per release from arguments (0.0.16/0.0.17 copied the previous script and once
# pointed at a wrong seed directory).  Never launches a window while a gatequeue is alive;
# a stray one older than 120 s is killed by PID.  Log: CAND_DIR/release-queue.log.
set -u
D=${1:?candidate dir}; UA=${2:?same-source reference}; SEED=${3:?seed dir}
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
# R19-0: the queue runs in its own detached worktree at the commit it started
# on, so commits to main (plans, prd, other agents' work) never reach it.
W=${QUEUE_WORKTREE:-/tmp/unisacc-queue-$(git rev-parse --short HEAD)}
if [ ! -d "$W" ]; then git worktree add -q --detach "$W" HEAD || { echo "queue: cannot create worktree $W"; exit 2; }; fi
cd "$W"
[ -s "$SEED/unisacc-seed.com" ] || { echo "no $SEED/unisacc-seed.com"; exit 2; }
[ -x "$UA" ] && [ -s "$D/unisacc-next.com" ] || { echo "missing UA or candidate"; exit 2; }
: "${UNISACC_FFI_X86_PROVIDER:?set UNISACC_FFI_X86_PROVIDER}"
LOG=$D/release-queue.log; : > "$LOG"
for i in $(seq 1 300); do
  for k in $(seq 1 90); do
    ps -eo pid,command | grep "[g]atequeue.py" > "$D/.gq" || break
    [ "$k" -ge 60 ] && awk '{print $1}' "$D/.gq" | xargs kill 2>/dev/null
    sleep 2
  done
  env TERM_SH_NOFALLBACK=1 ./tests/term.sh env REALPROG_CACHE="$R/corpus" UNISACC_FFI_X86_PROVIDER="$UNISACC_FFI_X86_PROVIDER" MODEL_COM="$D/unisacc-next.com" UA="$UA" SEED_DIR="$SEED" GATE_STATE="$D/queue" ./tests/release.sh --com >> "$LOG" 2>&1; rc=$?
  echo "window $i rc=$rc $(date +%H:%M:%S)" >> "$LOG"
  case $rc in 0) break;; 75|142) ;; *) tail -25 "$LOG" | grep -q BlockingIOError && continue; break;; esac
  [ $((i % 40)) -eq 0 ] && osascript -e 'tell application "Terminal" to close (every window whose busy is false)' >/dev/null 2>&1
done
echo "final rc=$rc" >> "$LOG"
grep -E "^queue:|final rc|UNVERIFIED" "$LOG" | tail -4
