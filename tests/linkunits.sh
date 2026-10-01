#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Separate compilation and the unisacc linker (0.0.17 R17-2, docs/toolchain.md §7).
#
# Each translation unit of the tests/multi/ fixtures (plus the fb12 multi-unit
# fixtures that are plain .c pairs) is compiled ON ITS OWN with
#   unisacc UNIT.c -c -b HOST -funit -o UNIT.o
# and the objects are joined by `unisacc a.o b.o -o prog`.  The program must
# print exactly what the one-step `unisacc a.c b.c` program prints and what
# `cc a.c b.c` prints, in both unit orders.  The fixtures were built so that
# accidental sharing is visible (both units define `hidden`/`helper`), so a
# renaming mistake in the linker shows as a wrong number, not a crash.
# Also: a mixed-target link and a foreign-target run are refused by name, and
# the objects are real objects (the system tools read them; elfobj links
# whole-program objects with them).
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(scratch)
case "$(uname -s)/$(uname -m)" in
    Darwin/arm64) HOST=osx/arm64; OTHER=lnx/arm64;; Darwin/x86_64) HOST=osx/x86_64; OTHER=lnx/x86_64;;
    Linux/x86_64) HOST=lnx/x86_64; OTHER=osx/x86_64;; Linux/aarch64) HOST=lnx/arm64; OTHER=osx/arm64;;
    *) echo "linkunits: no native target"; exit 1;;
esac
ok=0; bad=0
say() { if [ "$2" = "$3" ]; then ok=$((ok+1)); else bad=$((bad+1)); printf "  FAIL %-28s want [%s] got [%s]\n" "$1" "$2" "$3"; fi; }
pair() {   # pair NAME A.c B.c [nocc]  -- nocc: the fixture uses unisacc's on-demand headers (cc refuses it)
    local n=$1 a=$2 b=$3 want one got
    one=$("$_BOUND" 30 "$UA" -I tests/multi "$a" "$b" 2>&1; echo "rc=$?")
    if [ "${4:-}" = nocc ]; then want=$one
    else want=$("$_BOUND" 30 cc -std=c99 -w -I tests/multi -o "$T/$n.ref" "$a" "$b" 2>&1 && "$_BOUND" 10 "$T/$n.ref" 2>&1; echo "rc=$?"); fi
    "$_BOUND" 30 "$UA" -I tests/multi "$a" -c -b "$HOST" -funit -o "$T/$n.a.o" 2>"$T/err" &&
    "$_BOUND" 30 "$UA" -I tests/multi "$b" -c -b "$HOST" -funit -o "$T/$n.b.o" 2>>"$T/err" ||
        { bad=$((bad+1)); printf "  FAIL %-28s unit compile: %s\n" "$n" "$(head -1 "$T/err")"; return; }
    got=$("$_BOUND" 30 "$UA" "$T/$n.a.o" "$T/$n.b.o" 2>&1; echo "rc=$?")
    say "$n linked = cc" "$want" "$got"
    say "$n linked = one-step" "$one" "$got"
    got=$("$_BOUND" 30 "$UA" "$T/$n.a.o" "$T/$n.b.o" -o "$T/$n.prog" 2>&1 && "$_BOUND" 10 "$T/$n.prog" 2>&1; echo "rc=$?")
    say "$n linked -o = cc" "$want" "$got"
}
pair m       tests/multi/m1.c tests/multi/m2.c
pair m-rev   tests/multi/m2.c tests/multi/m1.c
pair n       tests/multi/n1.c tests/multi/n2.c nocc
pair static  tests/multi/static1.c tests/multi/static2.c
pair fwd     tests/multi/fwd1.c tests/multi/fwd2.c
pair fwd-rev tests/multi/fwd2.c tests/multi/fwd1.c
# refusals, by name
"$_BOUND" 30 "$UA" tests/multi/m1.c -c -b "$OTHER" -funit -o "$T/x1.o" 2>/dev/null
"$_BOUND" 30 "$UA" tests/multi/m2.c -c -b "$HOST" -funit -o "$T/x2.o" 2>/dev/null
say "mixed targets refused" "different targets" "$("$_BOUND" 30 "$UA" "$T/x1.o" "$T/x2.o" -o "$T/x" 2>&1 | grep -o 'different targets')"
say "foreign run refused" "another target" "$("$_BOUND" 30 "$UA" "$T/x1.o" 2>&1 | grep -o 'another target')"
printf 'int main(void){return 0;}\n' > "$T/plain.c"; "$_BOUND" 30 "$UA" "$T/plain.c" -c -b "$HOST" -o "$T/plain.o" 2>/dev/null
say "whole-program object refused" "no unit tape" "$("$_BOUND" 30 "$UA" "$T/plain.o" 2>&1 | grep -o 'no unit tape')"
echo
echo "linkunits  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
