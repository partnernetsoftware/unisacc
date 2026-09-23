#!/bin/bash
# unisacc.com: one file, every target. [A-38] [S-10]
#
# The file is a PE for Windows and a shell script for a Unix shell, with the
# lnx and osx images appended.  Here it is built and then run ON THIS HOST,
# which exercises the header (the shell has to accept the first line), the
# offsets in the script (BSD tail reads a leading zero as octal, so they are
# plain decimal) and the slice itself.
#
# The other hosts run the same file: tests/linux.sh through Lima, and
# tests/crossnative.sh's Windows machine loads the PE half.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || ./tests/build_ref.sh >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
COM=$T/unisacc.com
perl -e 'alarm 900; exec @ARGV' python3 -m unisa ape unisacc.c --via "$UA" \
    -o "$COM" >/dev/null 2>&1 || { echo "  FAIL could not build the .com"; exit 1; }
chmod +x "$COM"
[ "$(head -c 2 "$COM")" = "MZ" ] || { echo "  FAIL not an MZ file"; exit 1; }
ok=0; bad=0
for f in examples/hello.c examples/fib.c tests/c/b_float.c; do
    b=$(basename "$f" .c)
    { echo '#include <stdio.h>'; cat "$f"; } > "$T/ref.c"
    cc -w -o "$T/ref" "$T/ref.c" -lm 2>/dev/null || { continue; }
    "$T/ref" > "$T/want" 2>/dev/null
    perl -e 'alarm 120; exec @ARGV' "$COM" -run "$f" > "$T/got" 2>/dev/null
    if cmp -s "$T/want" "$T/got"; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %s\n" "$b"; diff "$T/want" "$T/got" | head -3; fi
done
printf "  the file: %d B, %s\n" "$(wc -c < "$COM")" "$(uname -s)/$(uname -m)"
echo
echo "ape  ran $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
