#!/bin/bash
# unisacc next to `cc -std=c99`, from a USER's side of the terminal. [S-16 T3]
#
# 0.0.7 shipped two defects no suite saw because none of them used the
# compiler the way a person does: `unisacc FILE.c` printed a token dump
# instead of writing a.out, and a failed compile left an empty a.out behind.
# Every case here runs the same command line through `cc -std=c99` and
# through unisacc from a scratch directory and compares what a user can see:
# the exit status, which stream a message lands on, which files exist
# afterwards, and what the built program does.
#
# Where unisacc differs ON PURPOSE the case says so and asserts the
# documented behaviour instead.  A difference that is a known, reported
# defect is counted `known`, not `wrong`, and says why; it is a TODO, not a
# pass.
#
#   UA=$PWD/unisacc.com ./tests/ccparity.sh     # the shipped product
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"; ua_ready
CC=${CC:-cc}
T=$(scratch); cd "$T"
ok=0; bad=0; kn=0
say() {   # say <name> <want> <got> [defect note]
    if [ "$2" = "$3" ]; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %-26s want [%s] got [%s]%s\n" "$1" "$2" "$3" "${4:+ -- $4}"; fi
}
known() { # known <name> <want> <got> <why> -- a recorded defect
    if [ "$2" = "$3" ]; then ok=$((ok+1))
    else kn=$((kn+1)); printf "  KNOWN %-25s want [%s] got [%s] -- %s\n" "$1" "$2" "$3" "$4"; fi
}
# ua / cc ARGS...: compile, bounded; stdout -> o.<who>, stderr -> e.<who>
ua() { bound 20 "$UA_RUN" "$@" >o.ua 2>e.ua; }
cc_() { bound 20 "$CC" -std=c99 "$@" >o.cc 2>e.cc; }
# rb PROG ARGS...: run a built program on its own watchdog -- the compile's
# alarm does not cover it
rb() { bound 5 "$@"; }
nz() { [ "$1" -ne 0 ] && echo nonzero || echo zero; }
has() { [ -s "$1" ] && echo text || echo empty; }
ex() { [ -e "$1" ] && echo yes || echo no; }
both() {  # both <name> ARGS... -- same exit class and same a.out existence
    local a c
    rm -f a.out; cc_ "${@:2}"; a=$?; local fa; fa=$(ex a.out)
    rm -f a.out; ua "${@:2}"; c=$?; local fu; fu=$(ex a.out)
    say "$1" "$(nz $a) a.out=$fa" "$(nz $c) a.out=$fu"
}

cat > hello.c <<'EOF'
#include <stdio.h>
int main(void) { printf("hello\n"); return 0; }
EOF
printf 'int main(void){ int x = ; }\n' > bad.c
: > empty.c
printf 'int f(void) { return 1; }\n' > nomain.c
cat > st.c <<'EOF'
#include <stdlib.h>
int main(int argc, char **argv) { return argc > 1 ? atoi(argv[1]) : 0; }
EOF
cat > io.c <<'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    int c, n = 0, i;
    while ((c = getchar()) != EOF) n++;
    printf("argc=%d n=%d", argc, n);
    for (i = 1; i < argc; i++) printf(" [%s]", argv[i]);
    printf("\n");
    fprintf(stderr, "to-stderr\n");
    return 0;
}
EOF
printf 'int twice(int x);\n#include <stdio.h>\nint main(void){ printf("%%d\\n", twice(21)); return 0; }\n' > m1.c
printf 'int twice(int x) { return x * 2; }\n' > m2.c
cat > pp.c <<'EOF'
#ifdef ON
on=ON
#endif
#ifdef OFF
off
#endif
#include "hdr.h"
v=HV
EOF
mkdir inc; printf '#define HV 77\n' > inc/hdr.h
printf 'int main(void) { int unused; return 0; }\n' > warn.c

# --- the default output --------------------------------------------------
both "no flag -> a.out"         hello.c
rm -f a.out; ua hello.c; say "a.out runs" "hello" "$(rb ./a.out)"
say "no flag: silent stdout"    "empty" "$(has o.ua)"
rm -f prog a.out; ua -o prog hello.c; say "-o NAME" "yes hello" "$(ex prog) $(rb ./prog)"
say "-o NAME: no a.out"         "no" "$(ex a.out)"
rm -f p1 p2; cc_ -o p1 -o p2 hello.c; w="$(ex p1) $(ex p2)"
rm -f p1 p2; ua  -o p1 -o p2 hello.c; say "two -o: last wins" "$w" "$(ex p1) $(ex p2)"
cc_ -o nodir/x hello.c; w=$(nz $?); ua -o nodir/x hello.c
say "-o missing dir: exit"      "$w" "$(nz $?)"
say "-o missing dir: stderr"  "text empty" "$(has e.ua) $(has o.ua)" \
    "'cannot write' is printed on stdout (src/main.c wopen failure uses printf)"

# --- inputs that are not programs ----------------------------------------
both "missing input"            nope.c
ua nope.c; say "missing: named on stderr" "yes" "$(grep -q nope.c e.ua && echo yes || echo no)"
both "empty file"               empty.c
both "file without main"        nomain.c
both "syntax error"             bad.c
ua bad.c; say "syntax: diag on stderr" "text empty" "$(has e.ua) $(has o.ua)"
say "syntax: diag names line"   "yes" "$(grep -q 'bad.c:1:' e.ua && echo yes || echo no)"
rm -f out.x; ua -o out.x bad.c; say "syntax: -o not created" "no" "$(ex out.x)"
# cc leaves a pre-existing a.out untouched when the compile fails; so must we
echo keep > a.out; cc_ bad.c; w=$(cat a.out)
echo keep > a.out; ua bad.c;  say "syntax: old a.out kept" "$w" "$(cat a.out)"

# --- the built program ----------------------------------------------------
rm -f a.out; ua st.c
for s in 0 3 255; do rb ./a.out $s; say "binary exit $s" "$s" "$?"; done
for s in 0 3 255; do bound 20 "$UA_RUN" -run st.c $s >/dev/null 2>&1; say "-run exit $s" "$s" "$?"; done
cc_ -o io.cc io.c; ua -o io.ua io.c
w=$(printf abcde | rb ./io.cc x 'y z' 2>/dev/null)
say "argv+stdin (binary)"       "$w" "$(printf abcde | rb ./io.ua x 'y z' 2>/dev/null)"
say "argv+stdin (-run)"         "argc=3 n=5 [x] [y z]" \
    "$(printf abcde | bound 20 "$UA_RUN" -run io.c x 'y z' 2>/dev/null)"
say "program stderr apart"      "to-stderr" "$(rb ./io.ua </dev/null 2>&1 >/dev/null)"
say "program stdout apart"      "argc=1 n=0" "$(rb ./io.ua </dev/null 2>/dev/null)"

# --- several inputs, and stdin as one -------------------------------------
cc_ -o mm m1.c m2.c; w=$(rb ./mm); rm -f mm
ua -o mm m1.c m2.c; say "two input files" "$w" "$(rb ./mm 2>&1)"
# DIFFERENCE: clang refuses a bare `-` without -x c; unisacc reads it as C,
# which is what `-x c -` means to cc.  Compare against `cc -x c -`.
rm -f sa; cc_ -x c -o sa - < st.c; w="$(nz $?) $(rb ./sa 3; echo $?)"
rm -f sa; ua -o sa - < st.c;       say "'-' reads stdin" "$w" "$(nz $?) $(rb ./sa 3; echo $?)"

# --- the preprocessor ------------------------------------------------------
# token spacing in -E output is not specified; compare the tokens
norm() { grep -v '^#' "$1" | tr -d ' \t' | grep -v '^$'; }
cc_ -E -DON=1 -Iinc pp.c; w=$(norm o.cc)
ua  -E -DON=1 -Iinc pp.c; say "-E to stdout, -D, -I" "$w" "$(norm o.ua)"
cc_ -E -DON -DOFF -UOFF -Iinc pp.c; w=$(norm o.cc)
ua  -E -DON -DOFF -UOFF -Iinc pp.c; say "-U after -D" "$w" "$(norm o.ua)"
rm -f x.i; ua -E -Iinc -o x.i pp.c
say "-E -o file, stdout quiet"  "yes empty" "$(ex x.i) $(has o.ua)"
cc_ -E -Iinc pp.c; say "-E -o file content" "$(norm o.cc)" "$(norm x.i)"

# --- warnings --------------------------------------------------------------
cc_ -Wall warn.c; w=$(nz $?); rm -f a.out
ua -Wall warn.c; say "warning keeps exit 0" "$w a.out=yes" "$(nz $?) a.out=$(ex a.out)"
say "warning on stderr"         "text" "$(has e.ua)"
rm -f a.out; cc_ -Wall -Werror warn.c; w="$(nz $?) a.out=$(ex a.out)"
rm -f a.out; ua  -Wall -Werror warn.c
say "-Werror fails the build" "$w" "$(nz $?) a.out=$(ex a.out)" \
    "-Werror is accepted and ignored: warning printed, exit 0, a.out written"

# --- options ----------------------------------------------------------------
ua --version; say "--version exit 0" "0 text" "$? $(has o.ua)"
say "--version names unisacc"   "yes" "$(grep -q '^unisacc ' o.ua && echo yes || echo no)"
rm -f a.out; cc_ --bogus hello.c; w=$(nz $?)
rm -f a.out; ua --bogus hello.c; say "unknown option: exit" "$w a.out=no" "$(nz $?) a.out=$(ex a.out)"
say "unknown option: stderr"  "text empty" "$(has e.ua) $(has o.ua)" \
    "'unknown option' is printed on stdout, not stderr"
ua; say "no input: exit nonzero" "nonzero" "$(nz $?)"
say "no input: usage on stderr" "text empty" "$(has e.ua) $(has o.ua)" \
    "the usage line goes to stdout"

# --- the documented non-cc boundary: -c ------------------------------------
# DIFFERENCE (README "Not a cc drop-in"): -c writes the tape, not hello.o.
rm -f hello.o; ua -c hello.c; rc=$?
say "-c: exit 0, no .o"         "0 no" "$rc $(ex hello.o)"
say "-c: tape text on stdout"   "yes" "$(grep -q '^_start:' o.ua && echo yes || echo no)"
rm -f t.s; ua -c -o t.s hello.c; say "-c -o: tape in file" "yes empty" \
    "$(grep -q '^_start:' t.s && echo yes || echo no) $(has o.ua)"
# A cc user typing `-c` gets a screenful of tape and no word about it.
known "-c: says it is not a .o" "text" "$(has e.ua)" \
    "-c silently dumps the tape to stdout; only --usage mentions it is not an object file"

echo
echo "ccparity  ok $ok   wrong $bad   known $kn"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
