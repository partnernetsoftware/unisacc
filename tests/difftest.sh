#!/bin/sh
# Differential test against the system compiler.  [P-6]
#
# `unisa acc` proves net == gold.  It says NOTHING about whether gold == C99 --
# and gold has been wrong three times (E-2, E-15, E-16) while acc read 1.000.
# The only instrument that can see that is a reference compiler.
set -u
CC=${CC:-cc}
U="python3 -m unisa"
DRIVE=${DRIVE:-built}
pass=0; fail=0; unsup=0
T=$(mktemp -d); R=$(pwd)
# `python3 -m unisa` resolves against the cwd, and we are about to leave it
PYTHONPATH="$R${PYTHONPATH:+:$PYTHONPATH}"; export PYTHONPATH
for f in tests/c/*.c examples/*.c; do
    b=$(basename "$f" .c)
    [ "$b" = "host" ] && continue
    # the probes call printf and the string functions bare, because we link
    # our own; give the REFERENCE compiler the declarations it insists on --
    # clang makes an implicit declaration an ERROR -- and nothing else
    { echo '#include <stdio.h>'; echo '#include <string.h>'
      echo '#include <stdlib.h>'; cat "$f"; } > "$T/$b.ref.c"
    if ! $CC -w -std=c99 -o "$T/$b" "$T/$b.ref.c" 2>"$T/$b.cc"; then
        printf "  skip %-14s (reference rejects: %s)\n" "$b" \
            "$(grep -m1 error "$T/$b.cc" | cut -c1-40)"; continue
    fi
    # in $T, not the repo: some probes write files
    want=$(cd "$T" && ./"$b" 2>/dev/null); wcode=$?
    got=$(cd "$T" && $U run "$R/$f" --drive "$DRIVE" 2>"$T/$b.err"); gcode=$?
    if [ ! -s "$T/$b.err" ] && [ "$got" = "$want" ] && [ "$gcode" = "$wcode" ]; then
        pass=$((pass+1)); printf "  ok   %-14s %s\n" "$b" "$(echo "$want" | head -1)"
    elif [ -s "$T/$b.err" ]; then
        unsup=$((unsup+1)); printf "  UNS  %-14s %s\n" "$b" "$(head -1 "$T/$b.err" | cut -c1-58)"
    else
        fail=$((fail+1)); printf "  FAIL %-14s got %-18s want %s\n" "$b" "'$got'($gcode)" "'$want'($wcode)"
    fi
done
rm -rf "$T"
echo
echo "match $pass   wrong $fail   unsupported $unsup"
[ "$fail" -eq 0 ]
