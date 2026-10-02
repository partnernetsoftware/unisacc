#!/bin/sh
# cc interop slices 1-2 (research/cc-interop-plan.md): code compiled by
# `unisacc -c -b TARGET` (whole-program object) calls integer/pointer
# functions defined in a cc-compiled object; the system linker joins them and
# the output must equal the all-cc build.  Unsupported signatures must be
# refused by name (tests/ccinterop/refuse_*.err).
#   UA=path/to/unisacc ./tests/ccinterop.sh
#   CCI_VM_ARM (default `default`), CCI_VM_X86 (default minicon-lnx-x86_64): Lima VMs
#   STRICT=1: a skip is a failure
here=$(cd "$(dirname "$0")" && pwd)
UA=${UA:-/tmp/cci-ua}
P="$here/ccinterop"
B="python3 $here/bound.py"
tmp=$(mktemp -d /tmp/ccinterop.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
ok=0; wrong=0; skipped=0
pass() { ok=$((ok + 1)); echo "ccinterop  ok     $1"; }
fail() { wrong=$((wrong + 1)); echo "ccinterop  WRONG  $1"; }
skip() { skipped=$((skipped + 1)); echo "ccinterop  skip   $1"; }
[ -x "$UA" ] || { echo "ccinterop: no compiler at UA=$UA"; exit 2; }

# refusals: exact first error line, nonzero exit
for f in "$P"/refuse_*.c; do
    n=$(basename "$f" .c)
    $B 20 "$UA" -c -b osx/arm64 "$f" -o "$tmp/$n.o" > "$tmp/$n.out" 2>&1; rc=$?
    if [ $rc -ne 0 ] && [ "$(head -1 "$tmp/$n.out")" = "$(cat "$P/$n.err")" ]; then pass "$n"; else fail "$n (rc=$rc: $(head -1 "$tmp/$n.out"))"; fi
done

# pairs NAME:A:B -- A by unisacc -c -b, B by cc; int = outbound (slice 1),
# inb = inbound: cc calls the functions the unisacc half exports (slice 2),
# fp = outbound floating-point arguments/returns and 8-/10-argument calls (2B/2C)
PAIRS="int:int_a:int_b inb:inb_a:inb_b fp:fp_a:fp_b"
# osx/arm64 on this machine
osx() {   # name a b
    sdk=$(xcrun --show-sdk-path 2>/dev/null)
    if $B 20 "$UA" -c -b osx/arm64 "$P/$2.c" -o "$tmp/$1-a.o" \
       && $B 20 cc -O1 -c "$P/$3.c" -o "$tmp/$1-b.o" \
       && $B 20 ld -arch arm64 -o "$tmp/$1-p" "$tmp/$1-a.o" "$tmp/$1-b.o" -e _start -lSystem -syslibroot "$sdk" 2>/dev/null \
       && $B 20 "$tmp/$1-p" > "$tmp/$1-got" \
       && $B 30 cc -o "$tmp/$1-r" "$P/$2.c" "$P/$3.c" && $B 20 "$tmp/$1-r" > "$tmp/$1-want" \
       && cmp -s "$tmp/$1-got" "$tmp/$1-want"; then pass "$1 osx/arm64"; else fail "$1 osx/arm64"; fi
}
for pr in $PAIRS; do
    set -- $(echo "$pr" | tr : ' ')
    if [ "$(uname -s)/$(uname -m)" = Darwin/arm64 ]; then osx "$1" "$2" "$3"; else skip "$1 osx/arm64 (not an arm64 Mac)"; fi
done

# lnx/* inside Lima: link with the guest's cc, oracle is the guest's all-cc build
lnx() {   # arch vm name a b
    if ! command -v limactl > /dev/null 2>&1; then skip "$3 lnx/$1 (no limactl)"; return; fi
    st=$($B 15 limactl list --format '{{.Status}}' "$2" 2>/dev/null)
    if [ "$st" != Running ]; then skip "$3 lnx/$1 (Lima VM $2 not running)"; return; fi
    d="$tmp/lnx-$1-$3"; mkdir -p "$d"
    if ! $B 20 "$UA" -c -b "lnx/$1" "$P/$4.c" -o "$d/a.o"; then fail "$3 lnx/$1 (compile)"; return; fi
    cp "$P/$4.c" "$d/a.c"; cp "$P/$5.c" "$d/b.c"
    r=$( (cd "$d" && tar cf - a.o a.c b.c) | $B 55 limactl shell "$2" -- sh -c \
        'rm -rf /tmp/ccinterop && mkdir /tmp/ccinterop && cd /tmp/ccinterop && tar xf - && cc -O1 -fno-stack-protector -c b.c && cc -nostdlib -nostartfiles -static -o p a.o b.o && ./p > got && cc -o r a.c b.c && ./r > want && cmp -s got want && echo SAME' 2>&1); rc=$?
    case "$r" in
        *SAME*) pass "$3 lnx/$1" ;;
        *) if [ $rc -eq 124 ] || [ $rc -eq 137 ] || [ $rc -eq 142 ]; then skip "$3 lnx/$1 (VM $2 did not answer in time)"; else fail "$3 lnx/$1 ($(echo "$r" | tail -3 | tr '\n' ' '))"; fi ;;
    esac
}
for pr in $PAIRS; do
    set -- $(echo "$pr" | tr : ' ')
    lnx arm64 "${CCI_VM_ARM:-default}" "$1" "$2" "$3"
    lnx x86_64 "${CCI_VM_X86:-minicon-lnx-x86_64}" "$1" "$2" "$3"
done

echo "ccinterop  ok $ok   wrong $wrong   skipped $skipped"
[ $wrong -eq 0 ] || exit 1
[ "${STRICT:-0}" = 1 ] && [ $skipped -ne 0 ] && exit 1
[ $ok -gt 0 ] || exit 1
exit 0
