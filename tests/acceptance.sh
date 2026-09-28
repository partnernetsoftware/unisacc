#!/bin/sh
# UNISA SH acceptance. [A]  Run from the repo root.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
_BOUND=$("$R/tests/bound" --helper) || exit 2
U="$_BOUND 55 python3 -m unisa"
ACCEPT_CASE=${ACCEPT_CASE:-all}
case "$ACCEPT_CASE" in all|6-build|6-fold|6-fault|6-image|10-Btape|10-Bimage|10-Ctape|10-Uimage|10-compare) ;; *) echo 'invalid ACCEPT_CASE' >&2; exit 2;; esac
pass=0; fail=0
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
SELF="$T/unisacc.flat.c"
"$_BOUND" 10 python3 "$R/tests/sourceflat.py" "$SELF" || exit 1
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
# win_argregs perturbs the x86 WinAPI argument path; ARM now reads its own ABI.
for pair in osx_class_bit:4 win_argregs:5 arm_gate:4; do
    f=${pair%:*}; matches=${pair#*:}
    chk "fault $f" "$matches/6 match" "$($U run examples/hello.c --fold --fault $f | tail -1)"
done
}

sec6() {
echo "== [K-5] constructed integer weights: case $ACCEPT_CASE =="
if [ "$ACCEPT_CASE" = all ] || [ "$ACCEPT_CASE" = 6-build ]; then
    $U build-weights --out "$T/built.json" --pack "$T/built.uns2" >"$T/build.log" 2>&1 || { cat "$T/build.log"; fail=$((fail+1)); return; }
    # cmd_build's historical prose says '11'; inspect the actual stage set.
    "$_BOUND" 8 python3 - "$T/built.json" <<'CHECK' || { fail=$((fail+1)); return; }
import json,sys
from unisa.gold import ALL
with open(sys.argv[1]) as f: stages=json.load(f)
assert set(stages)==set(ALL), (set(stages),set(ALL))
print('constructed stage set: %d/%d' % (len(stages),len(ALL)))
CHECK
    if cmp -s "$T/built.json" weights/built.json && cmp -s "$T/built.uns2" weights/built.uns2; then r=identical; else r=DIFFER; fi
    chk "constructed weights and pack" identical "$r"
fi
if [ "$ACCEPT_CASE" = all ] || [ "$ACCEPT_CASE" = 6-fold ]; then
    SHARD=${SHARD:-1/1}; SK=${SHARD%/*}; SN=${SHARD#*/}
    case "$SHARD" in *[!0-9/]*|*/*/*|'') echo invalid_SHARD >&2; exit 2;; esac
    [ "$SK" -ge 1 ] && [ "$SN" -ge "$SK" ] || exit 2
    idx=0
    for e in hello fact ptr fib switch struct do; do
        idx=$((idx+1)); [ $(((idx-1)%SN)) -eq $((SK-1)) ] || continue
        if $U run examples/$e.c --fold --drive built >"$T/fold.log" 2>&1; then
            chk "fold $e (built)" "6/6 match" "$(tail -1 "$T/fold.log")"
        else cat "$T/fold.log"; fail=$((fail+1)); fi
    done
fi
if [ "$ACCEPT_CASE" = all ] || [ "$ACCEPT_CASE" = 6-fault ]; then
    $U run examples/hello.c --fold --drive built --fault osx_class_bit >"$T/fault.log" 2>&1
    fault_rc=$?
    if [ "$fault_rc" -eq 2 ]; then
        chk "fault under built" "4/6 match" "$(tail -1 "$T/fault.log")"
    else echo "unexpected fault-control exit $fault_rc"; cat "$T/fault.log"; fail=$((fail+1)); fi
fi
if [ "$ACCEPT_CASE" = all ] || [ "$ACCEPT_CASE" = 6-image ]; then
    $U compile examples/fib.c -o "$T/u_tr" --target osx/arm64 --drive spec >"$T/spec.log" 2>&1 &&
    $U compile examples/fib.c -o "$T/u_bu" --target osx/arm64 --drive built >"$T/built.log" 2>&1 &&
    [ -s "$T/u_tr" ] && [ -s "$T/u_bu" ] || { cat "$T/spec.log" "$T/built.log"; fail=$((fail+1)); return; }
    if cmp -s "$T/u_tr" "$T/u_bu"; then r=identical; else r=DIFFER; fi
    chk "spec vs constructed image" identical "$r"
fi
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
echo "== [A-20] self-hosting ladder: full exported source built two ways =="
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
echo "== [A-23] bootstrap fixed point: case $ACCEPT_CASE =="
if [ "$(uname -s)/$(uname -m)" != Darwin/arm64 ]; then
    echo 'bootstrap requires Darwin/arm64 (not a pass)'; return
fi
if [ "$ACCEPT_CASE" = all ]; then
    "$_BOUND" 55 ./tests/bootstrap.sh >"$T/bootstrap.log" 2>&1 || { cat "$T/bootstrap.log"; fail=$((fail+1)); return; }
    chk "B = C = U" "bootstrap fixed point reached" "$(tail -1 "$T/bootstrap.log")"
    return
fi
[ -n "${ACCEPT_STATE:-}" ] || { echo 'ACCEPT_STATE required for bootstrap pieces' >&2; exit 2; }
mkdir -p "$ACCEPT_STATE"; S=$ACCEPT_STATE
ACCEPT_REF=${UA:-/tmp/ua_ref}
[ -x "$ACCEPT_REF" ] || { echo 'bootstrap reference missing' >&2; exit 1; }
identity=$(cat unisacc.c "$SELF" "$SELF.source.json" tests/export_ref.sh src/*.c src/*.h kernel/*.c kernel/*.inc weights/built.json "$ACCEPT_REF" tests/acceptance.sh \
    unisa/*.py unisa/front/*.py unisa/image/*.py | cksum)
if [ "$ACCEPT_CASE" = 10-Btape ]; then
    . "$R/tests/lib.sh"; ua_ready
    rm -f "$S/B" "$S/C.tape" "$S/U" "$S/U.tape"
    printf '%s\n' "$identity" >"$S/source.stamp"
    "$_BOUND" 30 "$UA" "$SELF" -c >"$S/B.tape" || { fail=$((fail+1)); return; }
    [ -s "$S/B.tape" ] || { fail=$((fail+1)); return; }
else
    [ "$(cat "$S/source.stamp" 2>/dev/null)" = "$identity" ] || { echo 'bootstrap state missing/stale' >&2; exit 1; }
fi
case "$ACCEPT_CASE" in
    10-Bimage)
        [ -s "$S/B.tape" ] || exit 1
        rm -f "$S/B"
        "$_BOUND" 45 python3 -m unisa compile "$S/B.tape" --from-tape -o "$S/B" --target osx/arm64 --drive built || { fail=$((fail+1)); return; }
        [ -s "$S/B" ] || exit 1
        chmod +x "$S/B"; "$_BOUND" 8 codesign -f -s - "$S/B" || exit 1;;
    10-Ctape)
        [ -x "$S/B" ] || exit 1
        "$_BOUND" 30 "$S/B" "$SELF" -c >"$S/C.tape" || { fail=$((fail+1)); return; }
        [ -s "$S/C.tape" ] || exit 1;;
    10-Uimage)
        rm -f "$S/U"
        "$_BOUND" 45 python3 -m unisa compile "$SELF" -o "$S/U" --target osx/arm64 --drive built || { fail=$((fail+1)); return; }
        [ -s "$S/U" ] || exit 1
        chmod +x "$S/U"; "$_BOUND" 8 codesign -f -s - "$S/U" || exit 1;;
    10-compare)
        [ -x "$S/U" ] && [ -s "$S/B.tape" ] && [ -s "$S/C.tape" ] || exit 1
        "$_BOUND" 30 "$S/U" "$SELF" -c >"$S/U.tape" || { fail=$((fail+1)); return; }
        [ -s "$S/U.tape" ] || exit 1
        if cmp -s "$S/B.tape" "$S/C.tape" && cmp -s "$S/B.tape" "$S/U.tape"; then r=same; else r=DIFFER; fi
        chk "B = C = U" same "$r"; return;;
esac
chk "$ACCEPT_CASE completed" yes yes
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
for t in lnx:7f454c46 osx:cffaedfe win:4d5a0000; do
    os=${t%%:*}; want=${t#*:}
    for arch in x86_64 arm64; do
        $U compile examples/hello.c -o "$T"/h.$os.$arch --target $os/$arch >/dev/null
        chk "$os/$arch magic" "$want" \
            "$(od -An -tx1 -N4 "$T"/h.$os.$arch | tr -d ' \n')"
    done
done
}

sec13() {
echo "== [A-10] compiling twice gives identical bytes =="
$U compile examples/hello.c -o "$T"/r1 --target lnx/x86_64 >/dev/null
$U compile examples/hello.c -o "$T"/r2 --target lnx/x86_64 >/dev/null
if cmp -s "$T"/r1 "$T"/r2; then r=same; else r=differs; fi
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
# stays under the 60 s ceiling (AGENTS.md); all.sh runs individual applicable sections as k/15.  With
# no PART, every section runs, in order.
PART=${PART:-1/1}; PK=${PART%/*}; PN=${PART#*/}
case "$ACCEPT_CASE" in
    6-*) sec6;;
    10-*) sec10;;
    all)
i=1
while [ $i -le 15 ]; do
    [ $(( (i - 1) % PN )) -eq $(( PK - 1 )) ] && sec$i
    i=$((i + 1))
done
;;
esac

echo
echo "passed $pass, failed $fail"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$fail" -eq 0 ] && [ "$pass" -gt 0 ]
