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

echo "== [A-12] acc = 1.000 on FULL gold =="
n=$($U acc | grep -c ' 1\.0000')
chk "stages at 1.000" 11 "$n"

echo "== [A-5][A-6] every example folds 6/6 =="
for e in hello fact ptr fib switch struct do; do
    chk "fold $e" "6/6 match" "$($U run examples/$e.c --fold | tail -1)"
done

echo "== stdout values =="
chk "hello" "hello from C99" "$($U run examples/hello.c)"
chk "fact"  "120"            "$($U run examples/fact.c)"
chk "switch" "6"             "$($U run examples/switch.c)"
chk "do"    "3"              "$($U run examples/do.c)"
chk "fib"   "55"             "$($U run examples/fib.c)"
chk "ptr"   "7"              "$($U run examples/ptr.c)"
chk "struct" "9"             "$($U run examples/struct.c)"

echo "== [A-7] host.c differs by OS, never by arch =="
for os in lnx osx win; do
    a=$($U run examples/host.c --target $os/x86_64)
    b=$($U run examples/host.c --target $os/arm64)
    chk "host $os arch-invariant" "$a" "$b"
done

echo "== [A-8] negative control: the fold must be breakable =="
for f in osx_class_bit win_argregs arm_gate; do
    chk "fault $f" "4/6 match" "$($U run examples/hello.c --fold --fault $f | tail -1)"
done

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

echo "== [A-19] the compiler compiles its own decision layer =="
$U emit-kernel --out kernel >/dev/null
{ echo '#include <stdio.h>'; cat kernel/unisa_self.c; } > /tmp/u_ref.c
cc -w -std=c99 -o /tmp/u_ref /tmp/u_ref.c 2>/dev/null
chk "C kernel under cc" "8792 decisions, 0 wrong" "$(/tmp/u_ref)"
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    $U compile kernel/unisa_self.c -o /tmp/u_self --target osx/arm64 \
        --drive built >/dev/null
    chmod +x /tmp/u_self; codesign -f -s - /tmp/u_self >/dev/null 2>&1
    chk "C kernel built by unisa" "8792 decisions, 0 wrong" "$(/tmp/u_self)"
fi

echo "== [A-20] self-hosting ladder: unisacc.c built two ways =="
r=$(./tests/selfhost.sh examples/*.c tests/c/a_*.c 2>/dev/null | grep -c "differ 0")
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    chk "cc-built and unisa-built agree" "2" "$r"
else
    chk "cc-built unisacc agrees" "1" "$r"
fi

echo "== [A-21] unisacc compiles C and the tape runs =="
r=$(./tests/ccrun.sh examples/fact.c examples/fib.c examples/hello.c 2>/dev/null | tail -1)
chk "unisacc-compiled programs" "unisacc-compiled 3   wrong 0" "$r"

echo "== [A-23] bootstrap fixed point =="
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    chk "B = C = U" "bootstrap fixed point reached" \
        "$(./tests/bootstrap.sh 2>/dev/null | tail -1)"
else
    printf "  skip %-42s (needs the host toolchain)\n" "bootstrap"
fi

echo "== native execution on this host =="
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then
    n=$(./tests/native.sh examples/*.c 2>/dev/null | tail -1)
    chk "examples run natively" "native 8   mismatch 0" "$n"
else
    printf "  skip %-42s (no matching host toolchain)\n" "native execution"
fi

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

echo "== [A-10] compiling twice gives identical bytes =="
$U compile examples/hello.c -o /tmp/uimg/r1 --target lnx/x86_64 >/dev/null
$U compile examples/hello.c -o /tmp/uimg/r2 --target lnx/x86_64 >/dev/null
if cmp -s /tmp/uimg/r1 /tmp/uimg/r2; then r=same; else r=differs; fi
chk "two compiles" "same" "$r"

echo "== [A-11] UNS1 magic =="
$U dump-weights --dtype i8 --out weights >/dev/null
chk "parse.i8.unisa magic" "554e5331" \
    "$(od -An -tx1 -N4 weights/parse.i8.unisa | tr -d ' \n')"

echo "== [A-14] training is byte-reproducible =="
rm -rf /tmp/uw1 /tmp/uw2
$U train --quiet --out /tmp/uw1 >/dev/null
$U train --quiet --out /tmp/uw2 >/dev/null
if diff -r /tmp/uw1 /tmp/uw2 >/dev/null 2>&1; then r=same; else r=differs; fi
chk "two trainings" "same" "$r"

echo
echo "passed $pass, failed $fail"
[ "$fail" -eq 0 ]
