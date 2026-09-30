#!/bin/sh
# Differential test against the system compiler.  [P-6]
#
# `unisa acc` proves net == gold.  It says NOTHING about whether gold == C99 --
# and gold has been wrong three times (E-2, E-15, E-16) while acc read 1.000.
# The only instrument that can see that is a reference compiler.
#
# Two routes, one suite.  With UA unset this drives the Python reference
# (`python3 -m unisa`), which is what the default gate has always run.  With
# UA set -- the convention the other com-* jobs use, e.g. `UA="$PRODUCT"
# UA_RUN="$PRODUCT" ./tests/difftest_o.sh` -- it drives the C product instead,
# through the same `unisa FILE -run` command line.  The two are not
# interchangeable and that is the point: R13-0's fb12-03/04/05 fail only on the
# product (the reference exits 0/22/8 where 0.0.12 refuses to compile), so a
# suite that can only run the reference cannot see them at all.
set -u
CC=${CC:-cc}
DRIVE=${DRIVE:-built}
pass=0; fail=0; unsup=0
T=$(mktemp -d); R=$(pwd)
# `python3 -m unisa` resolves against the cwd, and we are about to leave it
PYTHONPATH="$R${PYTHONPATH:+:$PYTHONPATH}"; export PYTHONPATH
. "$R/tests/par.sh"
if [ -n "${UA:-}" ]; then
    # product route.  The two CLIs are not the same and this is where that
    # bites: the reference takes a `run` subcommand and `--drive`, the product
    # is tinycc-shaped and takes the file with `-run` after it -- the form
    # difftest_o.sh:68 already uses.  Driving the product with `run FILE
    # --drive` makes every probe fail to open the file, which reads as "the
    # product supports nothing" (measured: match 0, unsupported 36-37 per
    # shard) rather than as a wrong command line.
    U_RUN=${UA_RUN:-$UA}
    U_KIND=product
    [ -x "$U_RUN" ] || { echo "difftest: UA=$U_RUN is not executable" >&2; exit 2; }
    # Which list describes this driver: the shipped .com has its own (the two
    # routes disagree, see the header of difftest.com.knownfail); a UA that is
    # the reference build (/tmp/ua_ref or a private tests/build_ref.sh output)
    # is measured against the reference list, or every probe the reference
    # already handles reads as "revived" under the product's list.
    case "$U_RUN" in *.com) KNOWN=$R/tests/difftest.com.knownfail;; *) KNOWN=$R/tests/difftest.knownfail;; esac
    # `-run` BEFORE the file: both drivers accept that form, whereas the
    # reference (src/main.c) treats everything after the file as the program's
    # argv, so `FILE -run` gave the probe an argument -- fb12-03 (asserts
    # argc == 1) and fb12-06 (fopen(argv[1])) failed on the UA route for that
    # reason alone, not because of the compiler.  [R13-0 #03/#06, 2026-09-30]
    seen() { (cd "$T" && "$U_RUN" -O2 -run "$R/$1" 2>"$2") > "$3"; }
else
    U_KIND=reference
    KNOWN=$R/tests/difftest.knownfail
    seen() { (cd "$T" && python3 -m unisa run "$R/$1" --drive "$DRIVE" 2>"$2") > "$3"; }
fi
isknown() { grep -qs "^$1[[:space:]]" "$KNOWN"; }
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
      # #line: the shim above shifts every physical line, and __LINE__ /
      # __FILE__ (N17) must see the probe as unisacc does -- its own
      # numbering and the path it was handed (the same "$f" both sides get)
      printf '#line 1 "%s"\n' "$f"
      cat "$f"; } > "$T/$b.ref.c"
    if ! $CC -w -std=c99 -o "$D/$b" "$T/$b.ref.c" -lm 2>"$T/$b.cc"; then
        exit 0
    fi
    (cd "$D" && ./"$b" 2>/dev/null) > "$T/$b.want"; echo $? > "$T/$b.wcode"
    seen "$f" "$T/$b.err" "$T/$b.got"
    echo $? > "$T/$b.gcode"
    ) &
done
wait
# B: the verdicts, in order.
#
# knownfail, the same rule as tests/c99.knownfail: a probe listed there may be
# FAIL or UNS without failing the suite, and a listed probe that starts
# PASSING is a failure -- so closing a defect means deleting its line and the
# list cannot rot.  Without this, a red example could only be landed by
# breaking the gate, which is why the six R13-0 probes waited for it.
known=0; revived=0
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
        if isknown "$b"; then
            revived=$((revived+1))
            printf "       %-14s is listed in difftest.knownfail but passes: delete its line\n" "$b"
        fi
    elif isknown "$b"; then
        known=$((known+1)); printf "  known %-12s %s\n" "$b" "$(grep -m1 "^$b[[:space:]]" "$KNOWN" | cut -c1-58)"
    elif [ -s "$T/$b.err" ]; then
        unsup=$((unsup+1)); printf "  UNS  %-14s %s\n" "$b" "$(head -1 "$T/$b.err" | cut -c1-58)"
    else
        fail=$((fail+1)); printf "  FAIL %-14s got %-18s want %s\n" "$b" "'$got'($gcode)" "'$want'($wcode)"
    fi
done
rm -rf "$T"
echo
echo "match $pass   wrong $fail   unsupported $unsup   knownwrong $known   revived $revived"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ] && [ "$revived" -eq 0 ]
