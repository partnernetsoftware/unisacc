#!/bin/bash
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
perl -e 'alarm 55; exec @ARGV' python3 -m unisa ape unisacc.c --via "$UA" -O 2 \
    -o "$COM" >/dev/null 2>&1 || { echo "  FAIL could not build the .com"; exit 1; }
chmod +x "$COM"
[ "$(head -c 2 "$COM")" = "MZ" ] || { echo "  FAIL not an MZ file"; exit 1; }
ok=0; bad=0
for f in examples/hello.c examples/fib.c tests/c/b_float.c; do
    b=$(basename "$f" .c)
    { echo '#include <stdio.h>'; cat "$f"; } > "$T/ref.c"
    cc -w -o "$T/ref" "$T/ref.c" -lm 2>/dev/null || { continue; }
    "$T/ref" > "$T/want" 2>/dev/null
    perl -e 'alarm 30; exec @ARGV' "$COM" -run "$f" > "$T/got" 2>/dev/null
    if cmp -s "$T/want" "$T/got"; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %s\n" "$b"; diff "$T/want" "$T/got" | head -3; fi
done
# a watchdog that kills the .com must kill the compiler it unpacked, too:
# the launcher ran it as a child, and a timed-out run once left one spinning
# at 97% CPU for nine hours [I4 era].  Stdin still reaches it.
printf 'int main(void){ for(;;){} return 0; }\n' > "$T/spin.c"
(cd "$T" && perl -e 'alarm 3; exec @ARGV' sh "$COM" spin.c -run >/dev/null 2>&1)
sleep 1
if ps -eo command | grep -q "[u]nisacc\.[0-9]* spin.c"; then
    bad=$((bad+1)); echo "  FAIL a timed-out .com left its compiler running"
    ps -eo pid,command | grep "[u]nisacc\.[0-9]* spin.c" | awk '{print $1}' | xargs kill -9 2>/dev/null
else ok=$((ok+1)); fi
got=$(printf 'int main(void){ return 42; }\n' | (cd "$T" && perl -e 'alarm 30; exec @ARGV' sh "$COM" - -run); echo $?)
if [ "$got" = 42 ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL stdin did not reach the compiler ($got)"; fi
printf "  the file: %d B, %s\n" "$(wc -c < "$COM")" "$(uname -s)/$(uname -m)"
echo
echo "ape  ran $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
