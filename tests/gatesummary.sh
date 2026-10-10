#!/bin/bash
# gatesummary.sh RESULT_DIR T0 -- the gate summary (0.0.40, 机房主任 20:54): counts only the leading
# "suite rc=N" field of each result line ('%-14s rc=%-3s %3ss :: %s'), never a number later in the line.
# A line without that leading field is a failure.  Prints the summary; exits 1 if any failed, 4 if any
# unverified (77) and none failed, else 0.
set -u
O=${1:?result dir}; T0=${2:-$(date +%s)}
n=$(ls "$O" 2>/dev/null | wc -l | tr -d ' ')
[ "$n" -gt 0 ] || { echo "gate  suites 0   no result lines: failed (nothing was checked)"; exit 1; }
all=$(cat "$O"/*) || { echo "gate: cannot read result lines in $O"; exit 1; }
bad=$(printf '%s\n' "$all" | grep -vcE '^[^ ]+ +rc=(0|77) ')
unverified=$(printf '%s\n' "$all" | grep -cE '^[^ ]+ +rc=77 ')
echo "gate  suites $(ls "$O" | wc -l | tr -d ' ')   failed $bad   unverified $unverified   $(( $(date +%s)-T0 ))s wall"
[ "$bad" -eq 0 ] || exit 1
[ "$unverified" -eq 0 ] || exit 4
exit 0
