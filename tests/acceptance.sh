#!/bin/sh
# UNISA SH acceptance. [A]  Run from the repo root.
set -u
U="python3 -m unisa"
pass=0; fail=0
chk() { # chk <name> <expected> <actual>
    if [ "$2" = "$3" ]; then
        pass=$((pass+1)); printf "  ok   %-42s %s\n" "$1" "$3"
    else
        fail=$((fail+1)); printf "  FAIL %-42s got %s want %s\n" "$1" "$3" "$2"
    fi
}

sec1() {
echo "== [A-12] acc = 1.000 on FULL gold =="
n=$($U acc | grep -c ' 1\.0000')
# every stage the gold defines, not a number that goes stale when one is added
want=$(python3 -c "from unisa.gold import STAGES; print(len(STAGES))")
chk "stages at 1.000" "$want" "$n"
}

sec2() {
echo "== [A-5][A-6] every example folds 6/6 =="
for e in hello fact ptr fib switch struct do; do
    chk "fold $e" "6/6 match" "$($U run examples/$e.c --fold | tail -1)"
done
}

sec3() {
echo "== stdout values =="
chk "hello" "hello from C99" "$($U run examples/hello.c)"
chk "fact"  "120"            "$($U run examples/fact.c)"
chk "switch" "6"             "$($U run examples/switch.c)"
chk "do"    "3"              "$($U run examples/do.c)"
chk "fib"   "55"             "$($U run examples/fib.c)"
chk "ptr"   "7"              "$($U run examples/ptr.c)"
chk "struct" "9"             "$($U run examples/struct.c)"
}

sec4() {
echo "== [A-7] host.c differs by OS, never by arch =="
for os in lnx osx win; do
    a=$($U run examples/host.c --target $os/x86_64)
    b=$($U run examples/host.c --target $os/arm64)
    chk "host $os arch-invariant" "$a" "$b"
done
}

sec5() {
echo "== [A-8] negative control: the fold must be breakable =="
for f in osx_class_bit win_argregs arm_gate; do
    chk "fault $f" "4/6 match" "$($U run examples/hello.c --fold --fault $f | tail -1)"
done
}

sec6() {
echo "== [K-5] constructed integer weights: same compiler, no float =="
chk "build-weights exact" "all 11 exact by construction, verified over FULL gold" \
    "$($U build-weights | grep '^all 11')"
for e in hello fact ptr fib switch struct do; do
    chk "fold $e (built)" "6/6 match" \
        "$($U run examples/$e.c --fold --drive built | tail -1)"
done
chk "fault under built" "4/6 match" \
    "$($U run examples/hello.c --fold --drive built --fault osx_class_bit | tail -1)"
$U compile examples/fib.c -o /tmp/u_tr --target osx/arm64 --drive spec  >/dev/null
$U compile examples/fib.c -o /tmp/u_bu --target osx/arm64 --drive built >/dev/null
if cmp -s /tmp/u_tr /tmp/u_bu; then r=identical; else r=DIFFER; fi
chk "trained vs constructed image" "identical" "$r"
}

sec7() {
echo "== [A-19] the compiler compiles its own decision layer =="
$U emit-kernel --out kernel >/dev/null
# the self test includes the model and the cases from beside itself, so the
# copy cc builds is told where that is
{ echo '#include <stdio.h>'; cat kernel/unisa_self.c; } > /tmp/u_ref.c
cc -w -std=c99 -I kernel -o /tmp/u_ref /tmp/u_ref.c 2>/dev/null
# Every decision of every stage's FULL gold, counted FROM the tables -- a
# literal count went stale the first time a table grew (the floating axis).
NDEC=$(python3 -c "
import sys; sys.path.insert(0, '.')
from unisa.gold import ALL, STAGES
print(sum(len(STAGES[s].keys()) * len(STAGES[s].heads) for s in ALL))")
chk "C kernel under cc" "$NDEC decisions, 0 wrong" "$(/tmp/u_ref)"
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    $U compile kernel/unisa_self.c -o /tmp/u_self --target osx/arm64 \
        --drive built >/dev/null
    chmod +x /tmp/u_self; codesign -f -s - /tmp/u_self >/dev/null 2>&1
    chk "C kernel built by unisa" "$NDEC decisions, 0 wrong" "$(/tmp/u_self)"
fi
}

sec8() {
echo "== [A-20] self-hosting ladder: unisacc.c built two ways =="
r=$(./tests/selfhost.sh examples/*.c tests/c/a_*.c 2>/dev/null | grep -c "differ 0")
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    chk "cc-built and unisa-built agree" "2" "$r"
else
    chk "cc-built unisacc agrees" "1" "$r"
fi
}

sec9() {
echo "== [A-21] unisacc compiles C and the tape runs =="
r=$(NOBASE=1 ./tests/ccrun.sh examples/fact.c examples/fib.c examples/hello.c 2>/dev/null | tail -1)
chk "unisacc-compiled programs" "unisacc-compiled 3   wrong 0   knownwrong 0   refused 0" "$r"
}

sec10() {
echo "== [A-23] bootstrap fixed point =="
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    chk "B = C = U" "bootstrap fixed point reached" \
        "$(./tests/bootstrap.sh 2>/dev/null | tail -1)"
else
    printf "  skip %-42s (needs the host toolchain)\n" "bootstrap"
fi
}

sec11() {
echo "== native execution on this host =="
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    n=$(./tests/native.sh examples/*.c 2>/dev/null | tail -1)
    chk "examples run natively" "native 8   mismatch 0" "$n"
else
    printf "  skip %-42s (no matching host toolchain)\n" "native execution"
fi
}

sec12() {
echo "== [A-9] image magics =="
mkdir -p /tmp/uimg
for t in lnx:7f454c46 osx:cffaedfe win:4d5a0000; do
    os=${t%%:*}; want=${t#*:}
    for arch in x86_64 arm64; do
        $U compile examples/hello.c -o /tmp/uimg/h.$os.$arch --target $os/$arch >/dev/null
        chk "$os/$arch magic" "$want" \
            "$(od -An -tx1 -N4 /tmp/uimg/h.$os.$arch | tr -d ' \n')"
    done
done
}

sec13() {
echo "== [A-10] compiling twice gives identical bytes =="
$U compile examples/hello.c -o /tmp/uimg/r1 --target lnx/x86_64 >/dev/null
$U compile examples/hello.c -o /tmp/uimg/r2 --target lnx/x86_64 >/dev/null
if cmp -s /tmp/uimg/r1 /tmp/uimg/r2; then r=same; else r=differs; fi
chk "two compiles" "same" "$r"
}

sec14() {
echo "== [A-11] UNS1 magic =="
$U dump-weights --dtype i8 --out weights >/dev/null
chk "parse.i8.unisa magic" "554e5331" \
    "$(od -An -tx1 -N4 weights/parse.i8.unisa | tr -d ' \n')"
}

sec15() {
echo "== [A-14] the shipped weights are byte-reproducible =="
# Reconstruction is seconds and is what we ship.  Training is NOT run here:
# it is minutes of full-core work, it is not on the shipping path, and it once
# turned this suite into a two-hour job.  `tests/baseline.sh` runs it on
# purpose, when someone asks for the comparison numbers.
cp weights/built.uns2 /tmp/uw_built1
$U build-weights >/dev/null
if cmp -s /tmp/uw_built1 weights/built.uns2; then r=same; else r=differs; fi
chk "two constructions" "same" "$r"
}

# PART=k/n runs every n-th section starting at the k-th, so that each run
# stays under the 60 s ceiling (AGENTS.md); all.sh runs 1/4 .. 4/4.  With
# no PART, every section runs, in order.
PART=${PART:-1/1}; PK=${PART%/*}; PN=${PART#*/}
i=1
while [ $i -le 15 ]; do
    [ $(( (i - 1) % PN )) -eq $(( PK - 1 )) ] && sec$i
    i=$((i + 1))
done

echo
echo "passed $pass, failed $fail"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]
