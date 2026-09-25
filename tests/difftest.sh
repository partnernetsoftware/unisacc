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
. "$R/tests/par.sh"
# SHARD=k/n runs every n-th probe from the k-th, so each shard stays under
# the 60 s ceiling (AGENTS.md); the default 1/1 is the whole list.
FILES=""; i=0
SHARD=${SHARD:-1/1}; SH_K=${SHARD%/*}; SH_N=${SHARD#*/}
for f in tests/c/*.c examples/*.c; do
    [ $((i % SH_N)) -eq $((SH_K - 1)) ] && FILES="$FILES $f"
    i=$((i + 1))
done
# A: every probe's reference build and both runs, PAR at a time.  Each probe
# gets a directory of its own -- some probes write files.
for f in $FILES; do
    b=$(basename "$f" .c)
    [ "$b" = "host" ] && continue
    throttle
    (
    D="$T/$b.d"; mkdir -p "$D"
    # the probes call printf and the string functions bare, because we link
    # our own; give the REFERENCE compiler the declarations it insists on --
    # clang makes an implicit declaration an ERROR -- and nothing else
    # ...including the three syscalls the probes spell as builtins.  glibc
    # EXPORTS a symbol named __mmap, so an undeclared call linked to it with
    # an implicit int return and the 64-bit pointer was cut to 32 bits: the
    # REFERENCE crashed, on Linux only, and the suite reported it as ours.
    { echo '#include <stdio.h>'; echo '#include <string.h>'
      echo '#include <stdlib.h>'; echo '#include <sys/mman.h>'
      echo 'static long unisa_mmap_(long a,long n,long p,long f,long d,long o){return (long)mmap((void *)a,(size_t)n,(int)p,(int)f,(int)d,(off_t)o);}'
      # VARIADIC: the probe writes __mmap(M_ARGS), and a six-parameter
      # function-like macro counts ONE argument there -- arguments are
      # identified before M_ARGS expands.  The build failed on glibc and
      # the fallback below quietly built the bare file instead.
      echo '#define __mmap(...) unisa_mmap_(__VA_ARGS__)'
      echo '#define __mprotect(a,n,p) mprotect((void *)(long)(a),(n),(p))'
      echo '#define __munmap(a,n) munmap((void *)(long)(a),(n))'
      cat "$f"; } > "$T/$b.ref.c"
    if ! $CC -w -std=c99 -o "$D/$b" "$T/$b.ref.c" -lm 2>"$T/$b.cc"; then
        exit 0
    fi
    (cd "$D" && ./"$b" 2>/dev/null) > "$T/$b.want"; echo $? > "$T/$b.wcode"
    (cd "$D" && $U run "$R/$f" --drive "$DRIVE" 2>"$T/$b.err") > "$T/$b.got"
    echo $? > "$T/$b.gcode"
    ) &
done
wait
# B: the verdicts, in order.
for f in $FILES; do
    b=$(basename "$f" .c)
    [ "$b" = "host" ] && continue
    if [ ! -f "$T/$b.wcode" ]; then
        printf "  skip %-14s (reference rejects: %s)\n" "$b" \
            "$(grep -m1 error "$T/$b.cc" | cut -c1-40)"; continue
    fi
    want=$(cat "$T/$b.want"); wcode=$(cat "$T/$b.wcode")
    got=$(cat "$T/$b.got"); gcode=$(cat "$T/$b.gcode")
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
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]
