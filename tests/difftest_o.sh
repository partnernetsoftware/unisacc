#!/bin/bash
# H3: every -O level against cc -O2.  Each probe is built by the system
# compiler at -O2 (with the same shim difftest.sh uses for our __mmap
# family) and run; unisacc runs it at -O0, -O1 and -O2.  All four must print
# the same bytes and exit the same way.
#
# cc's answer is cached by hash of source + cc version (as fuzz.sh does):
# each fresh reference binary costs a 0.5-0.9 s XProtect scan on first run,
# and a run may not take more than 60 s (AGENTS.md).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
CC=${CC:-cc}
T=$(scratch)
CACHE=${DIFFO_CACHE:-${TMPDIR:-/tmp}/unisacc-diffo-want}; mkdir -p "$CACHE"
CCV=$($CC --version 2>&1 | head -1)
PAR_WAIT=4
. "$R/tests/par.sh"
for f in tests/c/*.c examples/*.c; do
    b=$(basename "$f" .c)
    [ "$b" = "host" ] && continue
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    { echo '#include <stdio.h>'; echo '#include <string.h>'
      echo '#include <stdlib.h>'; echo '#include <sys/mman.h>'
      echo 'static long unisa_mmap_(long a,long n,long p,long f,long d,long o){return (long)mmap((void *)a,(size_t)n,(int)p,(int)f,(int)d,(off_t)o);}'
      echo '#define __mmap(...) unisa_mmap_(__VA_ARGS__)'
      echo '#define __mprotect(a,n,p) mprotect((void *)(long)(a),(n),(p))'
      echo '#define __munmap(a,n) munmap((void *)(long)(a),(n))'
      cat "$f"; } > "$D/ref.c"
    key="$CACHE/$( (cat "$D/ref.c"; echo "$CCV -O2") | shasum | cut -c1-40)"
    if [ -f "$key" ]; then cp "$key" "$D/want"
    else
        if ! $CC -w -std=c99 -O2 -o "$D/ref" "$D/ref.c" -lm 2>/dev/null; then
            echo skip > "$D/v"; exit 0; fi
        (cd "$D" && bound 10 ./ref 2>/dev/null; echo "rc=$?") > "$D/want"
        cp "$D/want" "$key.$$" && mv "$key.$$" "$key"
    fi
    for o in -O0 -O1 -O2; do
        (cd "$D" && bound 10 "$UA" $o "$R/$f" -run 2>/dev/null </dev/null; echo "rc=$?") > "$D/got$o"
    done
    echo done > "$D/v"
    ) &
done
wait
ok=0; bad=0; skip=0
for f in tests/c/*.c examples/*.c; do
    b=$(basename "$f" .c); D="$T/$b.d"
    [ "$b" = "host" ] && continue
    case "$(cat "$D/v" 2>/dev/null)" in
    skip) skip=$((skip+1)); continue;;
    done) ;;
    *) bad=$((bad+1)); echo "  FAIL $b: no verdict"; continue;;
    esac
    for o in -O0 -O1 -O2; do
        if cmp -s "$D/want" "$D/got$o"; then ok=$((ok+1))
        else bad=$((bad+1)); printf "  WRONG %-14s %s: got [%s] cc -O2 says [%s]\n" "$b" "$o" \
            "$(tr '\n' ' ' < "$D/got$o" | cut -c1-40)" "$(tr '\n' ' ' < "$D/want" | cut -c1-40)"; fi
    done
done
echo
echo "difftest_o  agree $ok   wrong $bad   skipped $skip   (-O0 -O1 -O2 against cc -O2)"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
