#!/bin/sh
# ps stand-in (r7g static draft).  bound execs this file directly, so the body records ITSELF first (same pid, the
# helper is not used for the body: that would recurse), then loops.  Each 10 ms sleep is a foreground spawn through
# the helper, which records it before exec.
stat_line=$(cat /proc/$$/stat) || exit 97
rest=${stat_line##*) }
start=$(echo "$rest" | awk '{print $20}')
case "$start" in ''|*[!0-9]*) exit 96 ;; esac
printf 'ps-flood %s %s\n' "$$" "$start" >> "$BOUNDSTALL_LEDGER" || exit 95
root=$(cat "$BOUNDSTALL_ROOT") || exit 1
i=2000000000
while :; do i=$((i+1)); echo "$i $root"; sh "$(dirname "$0")/boundstall_ledger.sh" ps-sleep sleep 0.01; done
