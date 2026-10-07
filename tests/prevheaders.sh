#!/bin/sh
# prevheaders.sh -- the installed previous release (./unisacc.com) must still compile the tools a candidate
# build makes with it (seed/tbl.c, seed/net.c) against this tree's include/, and a trimmed stdio program
# must run.  0.0.33 D3′: new stdio helpers gated only on their own names were unreachable for the
# 0.0.32 -ftrim-libc reach, so build_candidate's shared step failed with "undefined function".
cd "$(dirname "$0")/.." || exit 2
[ -f unisacc.com ] || { echo "prevheaders: no installed unisacc.com" >&2; exit 1; }
T=$(mktemp -d "${TMPDIR:-/tmp}/prevheaders.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
bad=0; n=0
for f in seed/tbl.c seed/net.c; do
  n=$((n+1)); tests/bound 30 sh unisacc.com "$f" -o "$T/x" > "$T/log" 2>&1 || { bad=$((bad+1)); echo "FAIL $f"; tail -2 "$T/log"; }
done
n=$((n+1))
if tests/bound 30 sh unisacc.com -ftrim-libc tests/c/stream_wbuf.c -o "$T/sw" > "$T/log" 2>&1 && tests/bound 10 "$T/sw" > "$T/out" 2>&1 && grep -q END "$T/out"; then :; else bad=$((bad+1)); echo "FAIL stream_wbuf -ftrim-libc"; tail -2 "$T/log"; fi
echo "prevheaders  checked $n   failed $bad"
[ $n -gt 0 ] && [ $bad -eq 0 ]
