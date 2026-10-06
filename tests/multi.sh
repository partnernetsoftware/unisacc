#!/bin/bash
# Several translation units, one program.  [A-30]
#
# There is no linker and no object format: `unisa compile a.c b.c` walks both
# units with one walker, so a call in the first reaches a definition in the
# last the same way it reaches one further down its own file.  The fixture is
# built so that accidental sharing would be VISIBLE -- both units define
# `hidden` and `helper` at file scope, with different values -- and so that
# the units really do have to meet: unit two writes unit one's global.
#
# `fwd.h`/`fwd1.c`/`fwd2.c` are a second fixture for one specific defect
# (R13-0b, the `multi` gate): a header whose static functions call one another
# FORWARD, included by ONE unit, run in BOTH orders.  A file-scope `static`
# used to be registered when its DEFINITION was reached, so a forward reference
# in the header's first function was spelled without its unit suffix while the
# label carried it -- `fwd1 fwd2` passed and `fwd2 fwd1` reported
# `undefined function`.  libc's stdio.h has that shape (printf -> _u_vfmt ->
# _u_digits), which is why the failure needed a unit that includes it and is
# not the first.  The fix registers the unit's statics BEFORE it is walked.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
D=$R/tests/multi
. "$R/tests/lib.sh"
T=$(scratch)
U="python3 -m unisa"
rc=0

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *)             HOST="";;
esac

# Compare output only after the actual bounded command has exited successfully.
checked() { # banner, expected, all|last, command...
    local banner=$1 expected=$2 mode=$3 status got
    shift 3
    bound 20 "$@" >"$T/got" 2>"$T/err"
    status=$?
    if [ "$mode" = last ]; then got=$(cat "$T/got" "$T/err" | tail -1)
    else got=$(cat "$T/got"); fi
    if [ "$status" -ne 0 ]; then
        printf '  FAIL %-24s exit %s: %s\n' "$banner" "$status" "$(tail -1 "$T/err")"
        rc=1
    elif [ "$got" = "$expected" ]; then
        printf '  ok   %-24s %s\n' "$banner" "$got"
    else
        printf "  FAIL %-24s got '%s' want '%s'\n" "$banner" "$got" "$expected"
        rc=1
    fi
}

# Host reference: two translation units and a linker, with separate bounds.
if ! bound 20 cc -std=c99 -w -o "$T/ref" "$D/m1.c" "$D/m2.c" 2>"$T/cc.err"; then
    echo "  FAIL system reference build"; cat "$T/cc.err"; exit 1
fi
want=$(bound 20 "$T/ref")
status=$?
if [ "$status" -ne 0 ] || [ -z "$want" ]; then
    echo "  FAIL system reference execution ($status)"; exit 1
fi
printf "  reference (cc m1.c m2.c)   %s\n" "$want"
# the forward-static pair: two() == 2, both() == 7 + 8, vfmt() + digits() == 8 + 7
fwdwant=$(bound 20 cc -std=c99 -w -I "$D" -o "$T/fwdref" "$D/fwd1.c" "$D/fwd2.c" 2>/dev/null && "$T/fwdref")
[ -n "$fwdwant" ] || fwdwant='(cc failed)'
printf "  reference (cc fwd 1 2)     %s\n" "$fwdwant"
checked "unisa run m1 m2" "$want" all $U run "$D/m1.c" "$D/m2.c"
checked "unisa run m2 m1" "$want" all $U run "$D/m2.c" "$D/m1.c"
# Implicit strlen must resolve in the second unit; clang has no such reference.
checked "libc on demand" 5 last $U run "$D/n1.c" "$D/n2.c"
checked "--fold" '6/6 match' last $U run "$D/m1.c" "$D/m2.c" --fold

if [ -n "$HOST" ]; then
    if bound 20 $U compile "$D/m1.c" "$D/m2.c" -o "$T/m" --target "$HOST" >/dev/null; then
        chmod +x "$T/m"
        if command -v codesign >/dev/null; then
            bound 10 codesign -f -s - "$T/m" >/dev/null 2>&1 || exit 1
        fi
        checked "native $HOST" "$want" all "$T/m"
    else
        echo "  FAIL native $HOST: compile failed"; rc=1
    fi
fi

# Same input through the native/reference or shipped compiler selected by UA.
ua_ready
checked "unisacc -run m1 m2" "$want" all "$UA" -run "$D/m1.c" "$D/m2.c"
checked "unisacc m2 m1 -run" "$want" all "$UA" -run "$D/m2.c" "$D/m1.c"
# A header whose statics call each other FORWARD, included by one unit only --
# and the run where that unit is not first is the one that used to fail: the
# static was registered when its DEFINITION was reached, so a forward
# reference in the header's first function was spelled without the unit
# suffix while the label carried it.  [R13-0b: register at the start of the
# unit, before the walk]
checked "unisacc fwd 1 2 -run" "$fwdwant" all "$UA" -run "$D/fwd1.c" "$D/fwd2.c"
checked "unisacc fwd 2 1 -run" "$fwdwant" all "$UA" -run "$D/fwd2.c" "$D/fwd1.c"
checked "unisacc libc on demand" 5 last "$UA" -run "$D/n1.c" "$D/n2.c"
# 0.0.31 H1'': environ assigned in one unit is what getenv reads in the other (cc agrees).
envwant=$(printf 'before outer\nenv2 sees from-env1')
checked "unisacc environ env1 env2 -run" "$envwant" all env H1PROBE=outer "$UA" -run "$D/env1.c" "$D/env2.c"
checked "unisacc environ env2 env1 -run" "$envwant" all env H1PROBE=outer "$UA" -run "$D/env2.c" "$D/env1.c"
# 0.0.31 F3: a unit with quoted includes keeps -ftrim-libc; the quoted headers (nested, and a
# macro naming strdup) are scanned, so the program still links and runs, and trims exactly as
# flat.c, the same text with the headers pasted in (0.0.30 reference: 132322 vs 66274 bytes).
checked "unisacc F3 quoted-include trim -run" "5 abc" all "$UA" -run "$D/f3/main.c"
if bound 20 "$UA" "$D/f3/main.c" -o "$T/f3q" && bound 20 "$UA" "$D/f3/flat.c" -o "$T/f3flat"; then
    a=$(wc -c < "$T/f3q"); b=$(wc -c < "$T/f3flat")
    if [ "$a" -eq "$b" ]; then echo "  ok   F3 quoted $a = flat $b bytes"; else echo "  FAIL F3 quoted $a != flat $b bytes"; rc=1; fi
else echo "  FAIL F3 build"; rc=1; fi
if [ -n "$HOST" ]; then
    if bound 20 "$UA" "$D/m1.c" "$D/m2.c" -b "$HOST" -o "$T/um" >/dev/null 2>&1; then
        chmod +x "$T/um"
        if command -v codesign >/dev/null; then
            bound 10 codesign -f -s - "$T/um" >/dev/null 2>&1 || exit 1
        fi
        checked "unisacc -b $HOST" "$want" all "$T/um"
    else
        echo "  FAIL unisacc -b $HOST: compile failed"; rc=1
    fi
fi

echo
[ $rc -eq 0 ] && echo "multi-unit ok" || echo "multi-unit FAILING"
exit $rc
