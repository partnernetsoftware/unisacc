#!/bin/bash
# checkrun.sh LOG -- CMD... (0.0.39, after 110ab2e4): run a check, keep its full output in LOG, print the last
# lines, and exit with the CHECK's own status -- never a pipe's last stage (110ab2e4 was committed red because
# `check | tail` returned tail's 0).  Use it as the gate before commit/push:  checkrun.sh L -- cmd && git commit ...
set -u
LOG=${1:?log file}; shift; [ "${1:-}" = -- ] && shift
[ $# -gt 0 ] || { echo "checkrun: no command" >&2; exit 2; }
"$@" > "$LOG" 2>&1; rc=$?
tail -n "${CHECKRUN_TAIL:-3}" "$LOG"
echo "checkrun: rc=$rc ($*)" | tee -a "$LOG" >&2   # the log carries its own verdict
exit $rc
