#!/bin/bash
# unisaccrun: compile and run, with nothing written to disk. [A-37] [S-9]
#
# For every probe, `unisaccrun FILE.c` must print what the same file prints
# when the system compiler builds it and the binary runs -- same stdout, same
# exit status.  The program is compiled AT the addresses it will run at, in
# memory this process mapped, so there is no image and no code signature.
#
# UA_RUN=<path> checks a different build: the point of the second one is that
# it is unisacc's OWN output (tests/closure.sh builds it that way).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
UA_RUN=${UA_RUN:-/tmp/ua_run}
[ -x "$UA_RUN" ] || ./tests/build_run.sh >/dev/null || exit 1
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
    perl -e 'alarm 60; exec @ARGV' "$UA_RUN" "$f" one two > "$T/run.out" 2>/dev/null
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
echo
echo "run  compiled-and-ran $ok   wrong $bad   (skipped $skip)"
[ "$bad" -eq 0 ]
