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
    # the product links the same objects through its own route (LINK_PRODUCT: the shipped .com)
    if [ -n "${LINK_PRODUCT:-}" ]; then
        got=$("$_BOUND" 30 sh "$LINK_PRODUCT" "$T/$n.a.o" "$T/$n.b.o" 2>&1; echo "rc=$?")
        say "$n product-linked = cc" "$want" "$got"
    fi
}
pair m       tests/multi/m1.c tests/multi/m2.c
pair m-rev   tests/multi/m2.c tests/multi/m1.c
pair n       tests/multi/n1.c tests/multi/n2.c nocc
pair static  tests/multi/static1.c tests/multi/static2.c
pair fwd     tests/multi/fwd1.c tests/multi/fwd2.c
pair fwd-rev tests/multi/fwd2.c tests/multi/fwd1.c
# archives (R17-3): `unisacc ar` writes a GNU/BSD archive the system `ar` lists,
# and the linker pulls only the members that define a wanted name
printf 'int sq(int x){return x*x;}\n' > "$T/sq.c"
printf 'int unusedfn(void){return 9;}\nint never_used_object = 7;\n' > "$T/un.c"
printf 'int helper2(int v){return v+1;}\n' > "$T/h2.c"
printf '#include <stdio.h>\nint helper2(int);\nint sq(int);\nint main(void){ printf("%%d\\n", sq(helper2(3))); return 0; }\n' > "$T/um.c"
for f in sq un h2 um; do "$_BOUND" 30 "$UA" "$T/$f.c" -c -b "$HOST" -funit -o "$T/$f.o" 2>/dev/null; done
(cd "$T" && "$_BOUND" 20 "$UA" ar rcs lib.a sq.o un.o h2.o)
say "ar t lists members" "sq.o un.o h2.o" "$( (cd "$T" && "$_BOUND" 20 "$UA" ar t lib.a) | tr '\n' ' ' | sed 's/ $//')"
cp "$T/sq.o" "$T/long_member_name_123.o"
(cd "$T" && "$_BOUND" 20 "$UA" ar rcs long.a long_member_name_123.o)
say "ar t BSD long name" "long_member_name_123.o" "$("$_BOUND" 20 "$UA" ar t "$T/long.a")"
command -v ar >/dev/null && say "system ar reads it" "sq.o un.o h2.o" "$( (cd "$T" && ar t lib.a) | tr '\n' ' ' | sed 's/ $//')"
say "link pulls needed members" "16 rc=0" "$( ("$_BOUND" 30 "$UA" "$T/um.o" "$T/lib.a"; echo "rc=$?") | tr '\n' ' ' | sed 's/ $//')"
say "unneeded member not pulled" "0" "$("$_BOUND" 30 "$UA" "$T/um.o" "$T/lib.a" -S -o - 2>/dev/null | grep -c 'unusedfn')"
python3 - "$T/truncated.a" <<'PY'
from pathlib import Path
import sys
header = bytearray(b' ' * 60)
header[:16] = b'ghost.o/        '
header[48:58] = b'999999999 '
header[58:60] = b'`\n'
Path(sys.argv[1]).write_bytes(b'!<arch>\n' + header)
PY
ar_out=$("$_BOUND" 10 "$UA" ar t "$T/truncated.a" 2>&1); ar_rc=$?
say "truncated archive refused" "rc=1" "rc=$ar_rc"
say "truncated member not listed" "0" "$(printf '%s' "$ar_out" | grep -c 'ghost.o')"
if [ -n "${LINK_PRODUCT:-}" ]; then
    say "product links the archive" "16 rc=0" "$( ("$_BOUND" 30 sh "$LINK_PRODUCT" "$T/um.o" "$T/lib.a"; echo "rc=$?") | tr '\n' ' ' | sed 's/ $//')"
    (cd "$T" && "$_BOUND" 20 sh "$LINK_PRODUCT" ar rcs libp.a sq.o un.o h2.o)
    say "product ar = reference ar bytes" "same" "$(cmp -s "$T/lib.a" "$T/libp.a" && echo same || echo differ)"
fi
# refusals, by name
"$_BOUND" 30 "$UA" tests/multi/m1.c -c -b "$OTHER" -funit -o "$T/x1.o" 2>/dev/null
"$_BOUND" 30 "$UA" tests/multi/m2.c -c -b "$HOST" -funit -o "$T/x2.o" 2>/dev/null
say "mixed targets refused" "different targets" "$("$_BOUND" 30 "$UA" "$T/x1.o" "$T/x2.o" -o "$T/x" 2>&1 | grep -o 'different targets')"
say "foreign run refused" "another target" "$("$_BOUND" 30 "$UA" "$T/x1.o" 2>&1 | grep -o 'another target')"
printf 'int main(void){return 0;}\n' > "$T/plain.c"; "$_BOUND" 30 "$UA" "$T/plain.c" -c -b "$HOST" -o "$T/plain.o" 2>/dev/null
say "whole-program object refused" "no unit tape" "$("$_BOUND" 30 "$UA" "$T/plain.o" 2>&1 | grep -o 'no unit tape')"
# duplicate definitions are refused by name (R19-8, dsh 2026-10-01): gcc refuses all three
printf 'int shared = 1;\nint get1(void){ return shared; }\n' > "$T/d1.c"
printf 'int shared = 2;\nint get1(void);\nint main(void){ return get1(); }\n' > "$T/d2.c"
say "two initialised globals refused" "multiple definitions" "$("$_BOUND" 30 "$UA" "$T/d1.c" "$T/d2.c" 2>&1 | grep -o 'multiple definitions' | head -1)"
printf 'int dup = 1;\nint dup = 2;\nint main(void){ return dup; }\n' > "$T/d3.c"
say "same-unit redefinition refused" "redefinition" "$("$_BOUND" 30 "$UA" "$T/d3.c" 2>&1 | grep -o 'redefinition' | head -1)"
printf 'static int sx = 1;\nstatic int sx = 2;\nint main(void){ return sx; }\n' > "$T/d4.c"
say "same-unit static redefinition refused" "redefinition" "$("$_BOUND" 30 "$UA" "$T/d4.c" 2>&1 | grep -o 'redefinition' | head -1)"
printf 'static int sy = 1;\nint gy(void){ return sy; }\n' > "$T/d5.c"; printf 'static int sy = 2;\nint gy(void);\nint main(void){ return gy() + sy; }\n' > "$T/d6.c"
say "statics in two units stay apart" "rc=3" "$("$_BOUND" 30 "$UA" "$T/d5.c" "$T/d6.c" >/dev/null 2>&1; echo "rc=$?")"
printf 'int main(void){ return 1; }\n' > "$T/ma.c"; printf 'int main(void){ return 2; }\n' > "$T/mb.c"
say "two mains refused (one step)" "multiple definitions" "$("$_BOUND" 30 "$UA" "$T/ma.c" "$T/mb.c" 2>&1 | grep -o 'multiple definitions' | head -1)"
"$_BOUND" 30 "$UA" "$T/ma.c" -c -b "$HOST" -funit -o "$T/ma.o" 2>/dev/null; "$_BOUND" 30 "$UA" "$T/mb.c" -c -b "$HOST" -funit -o "$T/mb.o" 2>/dev/null
say "two mains refused (unit objects)" "multiple definitions of main" "$("$_BOUND" 30 "$UA" "$T/ma.o" "$T/mb.o" 2>&1 | grep -o 'multiple definitions of main')"
printf 'int t;\nint main(void){ return t; }\n' > "$T/t1.c"; printf 'int t;\nint f(void){ return t; }\n' > "$T/t2.c"
say "tentative definitions still merge" "rc=0" "$("$_BOUND" 30 "$UA" "$T/t1.c" "$T/t2.c" >/dev/null 2>&1; echo "rc=$?")"
# R20-3: initialisers marked in the unit tape (.unit 2 / .gdef), checked at link time
u() { "$_BOUND" 30 "$UA" "$1.c" -c -b "$HOST" -funit -o "$1.o" 2>/dev/null; }
u "$T/d1"; u "$T/d2"
say "two initialised globals refused (unit objects)" "multiple definitions of shared" "$("$_BOUND" 30 "$UA" "$T/d1.o" "$T/d2.o" 2>&1 | grep -o 'multiple definitions of shared')"
printf 'int shared;\nint main(void){ return shared; }\n' > "$T/d7.c"; u "$T/d7"
say "one initialiser + tentative keeps it (unit objects)" "rc=1" "$("$_BOUND" 30 "$UA" "$T/d1.o" "$T/d7.o" >/dev/null 2>&1; echo "rc=$?")"
printf 'extern int nowhere;\nint main(void){ return nowhere; }\n' > "$T/x3.c"; printf 'int other(void){ return 0; }\n' > "$T/x4.c"; u "$T/x3"; u "$T/x4"
say "extern-only object refused (unit objects)" "undefined reference to nowhere" "$("$_BOUND" 30 "$UA" "$T/x3.o" "$T/x4.o" 2>&1 | grep -o 'undefined reference to nowhere')"
say "extern-only object refused (one step)" "undefined reference to nowhere" "$("$_BOUND" 30 "$UA" "$T/x3.c" "$T/x4.c" 2>&1 | grep -o 'undefined reference to nowhere')"
python3 - "$T/x4.o" "$T/old.o" <<'PY'
import sys; b=open(sys.argv[1],'rb').read(); i=b.find(b'.unit 2\n'); assert i>0
open(sys.argv[2],'wb').write(b[:i]+b';unit 2\n'+b[i+8:])   # same length: the old object, without the record
PY
say "object without .unit 2 refused" "predates .unit 2" "$("$_BOUND" 30 "$UA" "$T/old.o" "$T/x3.o" 2>&1 | grep -o 'predates .unit 2')"
echo
echo "linkunits  ok $ok   wrong $bad"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
