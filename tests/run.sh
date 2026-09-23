#!/bin/bash
# `unisacc -run`: compile and run, with nothing written to disk. [A-37] [S-9]
#
# For every probe, `unisacc -run FILE.c` must print what the same file prints
# when the system compiler builds it and the binary runs -- same stdout, same
# exit status.  The program is compiled AT the addresses it will run at, in
# memory this process mapped, so there is no image and no code signature.
#
# UA_RUN=<path> checks a different build -- for instance unisacc's own
# output, which is what makes the check worth running twice.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
# `unisacc -run FILE.c` -- the compiler itself, no separate tool
UA_RUN=${UA_RUN:-/tmp/ua_ref}
[ -x "$UA_RUN" ] || ./tests/build_ref.sh >/dev/null || exit 1
PROBES=${*:-examples/hello.c examples/fib.c examples/fact.c examples/switch.c
            examples/struct.c examples/do.c examples/ptr.c
            tests/c/b_float.c tests/c/b_argv.c tests/c/b_mmap.c
            tests/c/b_printf.c tests/c/b_static.c}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0; skip=0
for f in $PROBES; do
    b=$(basename "$f" .c)
    { echo '#include <stdio.h>'; cat "$f"; } > "$T/ref.c"
    cc -w -o "$T/ref" "$T/ref.c" -lm 2>/dev/null || cc -w -o "$T/ref" "$f" -lm 2>/dev/null || {
        skip=$((skip+1)); continue; }
    (cd "$T" && perl -e 'alarm 30; exec @ARGV' ./ref one two > ref.out 2>/dev/null)
    rc1=$?
    # from the repo root: unisacc looks for <stdio.h> under ./include
    perl -e 'alarm 60; exec @ARGV' "$UA_RUN" -run "$f" one two > "$T/run.out" 2>/dev/null
    rc2=$?
    # argv[0] differs by construction (the source path vs the binary), so a
    # probe that prints it is compared on the rest of its output
    if cmp -s "$T/ref.out" "$T/run.out" && [ "$rc1" = "$rc2" ]; then
        ok=$((ok+1))
    else
        bad=$((bad+1)); printf "  FAIL %-12s exit %s/%s\n" "$b" "$rc1" "$rc2"
        diff "$T/ref.out" "$T/run.out" | head -4
    fi
done
# Windows, when the UTM machine is up: `-run` there maps with VirtualAlloc,
# re-protects to make the code executable and flushes the instruction cache
# (arm64 will not run code that is still only in the data cache).
UTM=/Applications/UTM.app/Contents/MacOS/utmctl; VM=${WINVM:-minicon-win-arm-64}
if [ -x "$UTM" ] && "$UTM" status "$VM" 2>/dev/null | grep -q started \
   && perl -e 'alarm 20; exec @ARGV' "$UTM" exec "$VM" --cmd cmd.exe -- /c echo up >/dev/null 2>&1; then
    n=$(date +%s)$RANDOM
    for t in win/arm64 win/x86_64; do
        tt=$(echo "$t" | tr / _)
        "$UA_RUN" unisacc.c -b "$t" > "$T/uw.$tt" 2>/dev/null
        "$UTM" file push "$VM" 'C:\u\r'"$n$tt"'.exe' < "$T/uw.$tt" 2>/dev/null
    done
    { echo '#include <stdio.h>'; cat examples/fib.c; } > "$T/wf.c"
    "$UTM" file push "$VM" 'C:\u\r'"$n"'.c' < "$T/wf.c" 2>/dev/null
    printf '@echo off\r\ncd /d C:\\u\r\nr%swin_x86_64.exe -run r%s.c > r%s.txt 2>&1\r\nr%swin_arm64.exe -run r%s.c >> r%s.txt 2>&1\r\necho done > r%sd.txt\r\n' \
        "$n" "$n" "$n" "$n" "$n" "$n" "$n" | "$UTM" file push "$VM" 'C:\u\r'"$n"'.bat' 2>/dev/null
    "$UTM" exec "$VM" --hide --cmd cmd.exe -- /c 'C:\u\r'"$n"'.bat' >/dev/null 2>&1
    i=0; while [ $i -lt 60 ]; do
        case "$("$UTM" file pull "$VM" 'C:\u\r'"$n"'d.txt' 2>&1)" in *done*) break;; esac
        i=$((i+1)); sleep 3; done
    got=$("$UTM" file pull "$VM" 'C:\u\r'"$n"'.txt' 2>/dev/null | tr -d '\r' | tr '\n' ' ')
    case "$got" in
        "55 55 "*) ok=$((ok+2)); echo "  ok   windows -run, both ISAs";;
        *) bad=$((bad+1)); echo "  FAIL windows -run gave [$got]";;
    esac
else
    echo "  skip windows (-run needs the UTM machine started)"
fi

echo
echo "run  compiled-and-ran $ok   wrong $bad   (skipped $skip)"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
