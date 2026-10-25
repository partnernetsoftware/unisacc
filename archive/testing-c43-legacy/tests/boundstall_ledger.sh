#!/bin/sh
# Ledger helper (r7g static draft).  Usage: boundstall_ledger.sh TAG COMMAND [ARG...]
# Records "TAG pid starttime" in $BOUNDSTALL_LEDGER while this process is alive, then execs COMMAND (same pid, same
# starttime).  Used for every spawn the cases make, foreground and background alike.  This helper writes only that
# record.  The "intent TAG" line is written by the spawning parent (harness spawn()), not by this helper; the harness
# checks that every intent has its record.
tag=$1; shift
read -r stat_line < /proc/$$/stat || exit 97
rest=${stat_line##*) }
start=$(echo "$rest" | awk '{print $20}')
case "$start" in ''|*[!0-9]*) exit 96 ;; esac
printf '%s %s %s\n' "$tag" "$$" "$start" >> "$BOUNDSTALL_LEDGER" || exit 95
exec "$@"
