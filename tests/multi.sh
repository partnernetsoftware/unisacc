#!/bin/bash
# Several translation units, one program.  [A-30]
#
# There is no linker and no object format: `unisa compile a.c b.c` walks both
# units with one walker, so a call in the first reaches a definition in the
# last the same way it reaches one further down its own file.  The fixture is
# built so that accidental sharing would be VISIBLE -- both units define
# `hidden` and `helper` at file scope, with different values -- and so that
# the units really do have to meet: unit two writes unit one's global.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
D=$R/tests/multi
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
U="python3 -m unisa"
rc=0

case "$(uname -s)/$(uname -m)" in
    Darwin/arm64)  HOST=osx/arm64;;
    Darwin/x86_64) HOST=osx/x86_64;;
    Linux/x86_64)  HOST=lnx/x86_64;;
    Linux/aarch64) HOST=lnx/arm64;;
    *)             HOST="";;
esac

# The reference is the system compiler doing what it has always done: two
# translation units and a linker.
if ! cc -std=c99 -w -o "$T/ref" "$D/m1.c" "$D/m2.c" 2>"$T/cc.err"; then
    echo "  skip (no system compiler)"; exit 0
fi
want=$("$T/ref")
printf "  reference (cc m1.c m2.c)   %s\n" "$want"

check() {   # check <banner> <got>
    if [ "$2" = "$want" ]; then printf "  ok   %-24s %s\n" "$1" "$2"
    else printf "  FAIL %-24s got '%s' want '%s'\n" "$1" "$2" "$want"; rc=1; fi
}

check "unisa run m1 m2"  "$($U run "$D/m1.c" "$D/m2.c" 2>"$T/e1" || cat "$T/e1")"
# Order must not matter: `main` is in m1, and the entry sequence is emitted
# after every unit has been walked.
check "unisa run m2 m1"  "$($U run "$D/m2.c" "$D/m1.c" 2>"$T/e2" || cat "$T/e2")"

# libc on demand has to reach EVERY unit: `strlen` is used in n2.c with no
# <string.h>, and our headers define their functions `static`.  There is no cc
# reference for this one -- clang rejects the implicit declaration outright,
# which is the very situation the driver's retry exists to paper over.
got=$($U run "$D/n1.c" "$D/n2.c" 2>&1 | tail -1)
if [ "$got" = "5" ]; then printf "  ok   %-24s %s\n" "libc on demand" "$got"
else printf "  FAIL %-24s got '%s' want '5'\n" "libc on demand" "$got"; rc=1; fi

# Six lowerings of the same tape, in the target interpreter.
fold=$($U run "$D/m1.c" "$D/m2.c" --fold 2>&1 | tail -1)
if [ "$fold" = "6/6 match" ]; then printf "  ok   %-24s %s\n" "--fold" "$fold"
else printf "  FAIL %-24s %s\n" "--fold" "$fold"; rc=1; fi

# And a real image on this host, because the interpreter is the forgiving one.
if [ -n "$HOST" ]; then
    if $U compile "$D/m1.c" "$D/m2.c" -o "$T/m" --target "$HOST" >/dev/null; then
        chmod +x "$T/m"
        command -v codesign >/dev/null && codesign -f -s - "$T/m" >/dev/null 2>&1
        check "native $HOST" "$("$T/m")"
    else
        echo "  FAIL native $HOST: compile failed"; rc=1
    fi
fi

# The SHIPPED compiler, not only the Python driver: `unisacc a.c b.c` is
# what someone with one binary and two files actually types.
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || "$R/tests/build_ref.sh" >/dev/null || rc=1
if [ -x "$UA" ]; then
    check "unisacc -run m1 m2" \
        "$(perl -e 'alarm 120; exec @ARGV' "$UA" -run "$D/m1.c" "$D/m2.c" 2>&1 | tail -1)"
    check "unisacc m2 m1 -run" \
        "$(perl -e 'alarm 120; exec @ARGV' "$UA" -run "$D/m2.c" "$D/m1.c" 2>&1 | tail -1)"
    got=$(perl -e 'alarm 120; exec @ARGV' "$UA" -run "$D/n1.c" "$D/n2.c" 2>&1 | tail -1)
    if [ "$got" = "5" ]; then printf "  ok   %-24s %s\n" "unisacc libc on demand" "$got"
    else printf "  FAIL %-24s got '%s' want '5'\n" "unisacc libc on demand" "$got"; rc=1; fi
    if [ -n "$HOST" ]; then
        if perl -e 'alarm 200; exec @ARGV' "$UA" "$D/m1.c" "$D/m2.c" \
               -b "$HOST" -o "$T/um" >/dev/null 2>&1; then
            chmod +x "$T/um"
            command -v codesign >/dev/null && codesign -f -s - "$T/um" >/dev/null 2>&1
            check "unisacc -b $HOST" "$("$T/um")"
        else
            echo "  FAIL unisacc -b $HOST: compile failed"; rc=1
        fi
    fi
fi

echo
[ $rc -eq 0 ] && echo "multi-unit ok" || echo "multi-unit FAILING"
exit $rc
