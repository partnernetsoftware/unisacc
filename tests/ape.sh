#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# unisacc.com: one file, every target. [A-38] [S-10]
#
# The file is a PE for Windows and a shell script for a Unix shell, with the
# lnx and osx images appended, gzipped.  Here it is built and then run ON THIS
# HOST, which exercises the header (the shell has to accept the first line),
# the offsets in the script (BSD tail reads a leading zero as octal, so they
# are plain decimal), the `gzip -dc` the script pipes the slice through, and
# the slice itself.
#
# The other hosts run the same file: tests/linux.sh through Lima, and
# tests/crossnative.sh's Windows machine loads the PE half.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
COM=$T/unisacc.com
if [ "${1:-}" = --prepare ]; then
    [ "$#" -eq 2 ] && [ -n "${APE_STATE:-}" ] || { echo 'ape --prepare TARGET needs APE_STATE' >&2; exit 2; }
    exec "$_BOUND" 52 python3 tests/ape_stage.py prepare "$APE_STATE" "$2" "$UA"
fi
[ "$#" -eq 0 ] || { echo 'unknown ape option' >&2; exit 2; }
if [ -n "${APE_STATE:-}" ]; then
    "$_BOUND" 25 python3 tests/ape_stage.py pack "$APE_STATE" "$COM" "$UA" >"$T/build.log" 2>&1
else
    "$_BOUND" 55 python3 -m unisa ape unisacc.c --via "$UA" -O 2 \
        -o "$COM" >"$T/build.log" 2>&1
fi
build_rc=$?
if [ "$build_rc" -ne 0 ]; then
    echo "  FAIL could not build the .com (rc=$build_rc)"
    cat "$T/build.log"
    exit 1
fi
chmod +x "$COM"
[ "$(head -c 2 "$COM")" = "MZ" ] || { echo "  FAIL not an MZ file"; exit 1; }
ok=0; bad=0
for f in examples/hello.c examples/fib.c tests/c/b_float.c; do
    b=$(basename "$f" .c)
    { echo '#include <stdio.h>'; cat "$f"; } > "$T/ref.c"
    "$_BOUND" 20 cc -w -o "$T/ref" "$T/ref.c" -lm >"$T/ref-build.log" 2>&1 || {
        bad=$((bad+1)); echo "  FAIL reference build $b"; cat "$T/ref-build.log"; continue;
    }
    "$_BOUND" 8 "$T/ref" > "$T/want" 2>"$T/ref-run.log"; ref_rc=$?
    "$_BOUND" 30 "$COM" -run "$f" > "$T/got" 2>"$T/com-run.log"; com_rc=$?
    if [ "$ref_rc" -eq 0 ] && [ "$com_rc" -eq 0 ] && cmp -s "$T/want" "$T/got"; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %s\n" "$b"; diff "$T/want" "$T/got" | head -3; fi
done
# a watchdog that kills the .com must kill the compiler it unpacked, too:
# the launcher ran it as a child, and a timed-out run once left one spinning
# at 97% CPU for nine hours [I4 era].  Stdin still reaches it.
printf 'int main(void){ for(;;){} return 0; }\n' > "$T/spin.c"
(cd "$T" && "$_BOUND" 3 sh "$COM" spin.c -run >/dev/null 2>&1)
sleep 1
if ps -eo command | grep -q "[u]nisacc\.[0-9]* spin.c"; then
    bad=$((bad+1)); echo "  FAIL a timed-out .com left its compiler running"
    ps -eo pid,command | grep "[u]nisacc\.[0-9]* spin.c" | awk '{print $1}' | xargs kill -9 2>/dev/null
else ok=$((ok+1)); fi
got=$(printf 'int main(void){ return 42; }\n' | (cd "$T" && "$_BOUND" 30 sh "$COM" - -run); echo $?)
if [ "$got" = 42 ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL stdin did not reach the compiler ($got)"; fi
printf "  the file: %d B, %s\n" "$(wc -c < "$COM")" "$(uname -s)/$(uname -m)"
echo
echo "ape  ran $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
