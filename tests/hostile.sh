#!/bin/bash
# Input the compiler was not expecting. [A-47]
#
# Every other suite feeds it C.  This one feeds it the things that arrive
# when a compiler meets the world: a truncated file, an unterminated
# string, a cycle of includes, an identifier longer than any buffer, an
# expression nested a thousand deep, binary rubbish, a file that is empty.
#
# The bar is deliberately low and absolutely firm: for each one the
# compiler must EXIT -- with a diagnosis or even with a compile, but it
# must exit, within the time limit, without a signal.  A segfault or a
# hang on bad input is the failure; refusing the program is not.
#
# Why this is worth a suite: a crash here is a crash in the field, and a
# hang is worse because nothing tells the user it will never finish.  Both
# are invisible to every other suite, which only ever feeds it valid C.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
ok=0; bad=0; known=0

try() {   # try <name> -- the file $T/<name>.c must make it exit cleanly
    local name=$1 rc out
    out=$(bound 20 "$UA" "$T/$name.c" -c 2>&1 >/dev/null); rc=$?
    # 139 SIGSEGV, 138 SIGBUS, 134 SIGABRT, 142 SIGALRM (our own timeout)
    case "$rc" in
        139|138|134|136|11)
            printf "  FAIL %-22s crashed (signal, rc=%s)\n" "$name" "$rc"
            bad=$((bad+1));;
        142)
            printf "  FAIL %-22s did not finish in 20s\n" "$name"
            bad=$((bad+1));;
        *)
            ok=$((ok+1));;
    esac
}

# The second class: input the standard says must be DIAGNOSED.  Surviving
# is not enough here -- silently accepting invalid C is how a compiler
# produces a program its author never wrote.  The ones we get wrong today
# are listed in tests/hostile.knownfail with a reason, so the list cannot
# rot and closing one means deleting a line.
KNOWN=$R/tests/hostile.knownfail
refuse() {   # refuse <name> -- $T/<name>.c must be rejected, with a message
    local name=$1 rc out
    out=$(bound 20 "$UA" "$T/$name.c" -c 2>&1 >/dev/null); rc=$?
    case "$rc" in
        139|138|134|136|11|142)
            printf "  FAIL %-22s crashed or hung (rc=%s)\n" "$name" "$rc"
            bad=$((bad+1)); return;;
    esac
    if [ "$rc" -ne 0 ] && [ -n "$out" ]; then
        if grep -qs "^$name[[:space:]]" "$KNOWN"; then
            printf "  REVIVED %s is diagnosed now -- delete it from hostile.knownfail\n" "$name"
            bad=$((bad+1))
        else ok=$((ok+1)); fi
        return
    fi
    if grep -qs "^$name[[:space:]]" "$KNOWN"; then known=$((known+1)); return; fi
    if [ "$rc" -eq 0 ]; then
        printf "  FAIL %-22s accepted silently; it is not valid C\n" "$name"
    else
        printf "  FAIL %-22s rejected with NO message (rc=%s)\n" "$name" "$rc"
    fi
    bad=$((bad+1))
}

printf '' > "$T/empty.c"
try empty

printf '/* a comment that never ends\n' > "$T/unterm_comment.c"
refuse unterm_comment

printf '#include <stdio.h>\nint main(void) { char *s = "no end;\n  return 0; }\n' > "$T/unterm_string.c"
refuse unterm_string

printf 'int main(void) { return 0;\n' > "$T/truncated.c"
refuse truncated

# a #include cycle: a.c includes b.h includes a.h includes b.h ...
printf '#include "a.h"\nint main(void){return 0;}\n' > "$T/cycle.c"
printf '#include "b.h"\n' > "$T/a.h"
printf '#include "a.h"\n' > "$T/b.h"
(cd "$T" && bound 20 "$UA" cycle.c -c -I . >/dev/null 2>&1)
case "$?" in
    139|138|134|136|11) echo "  FAIL include cycle        crashed"; bad=$((bad+1));;
    142) echo "  FAIL include cycle        did not finish in 20s"; bad=$((bad+1));;
    *) ok=$((ok+1));;
esac

# an identifier far longer than any name buffer
python3 -c "
n = 'x' * 5000
print('int %s = 1;' % n)
print('int main(void) { return %s - 1; }' % n)
" > "$T/long_ident.c"
try long_ident

# an expression nested a thousand deep: recursive descent has a stack
python3 -c "
d = 1000
print('int main(void) { return ' + '('*d + '1' + ')'*d + ' - 1; }')
" > "$T/deep_parens.c"
try deep_parens

# ...and a thousand binary operators, which nests the other way
python3 -c "
print('int main(void) { return ' + ' + '.join(['1']*2000) + ' - 2000; }')
" > "$T/long_expr.c"
try long_expr

# a thousand nested blocks
python3 -c "
d = 500
print('int main(void) {' + '{'*d + 'int x = 1;' + '}'*d + ' return 0; }')
" > "$T/deep_blocks.c"
try deep_blocks

# binary rubbish, deterministic so a failure reproduces
python3 -c "
import sys
b = bytes((i * 37 + 11) & 0xFF for i in range(4096))
sys.stdout.buffer.write(b)
" > "$T/binary.c"
try binary

# a NUL in the middle of the text
python3 -c "
import sys
sys.stdout.buffer.write(b'int main(void) { return 0; }\n\x00int trailing;\n')
" > "$T/embedded_nul.c"
try embedded_nul

# a macro that expands to itself, and a pair that expand to each other:
# C99 6.10.3.4p2 says a macro is not replaced during its own expansion
printf '#define A A\n#define B C\n#define C B\nint main(void){ return 0; }\nint a = A; int b = B;\n' > "$T/macro_loop.c"
try macro_loop

# a function-like macro invoked with far too many arguments
printf '#define M(a,b) ((a)+(b))\nint main(void){ return M(1,2,3,4,5,6,7,8,9,10,11,12); }\n' > "$T/macro_args.c"
try macro_args

# #if with an expression that is not one
printf '#if )(\n#endif\nint main(void){return 0;}\n' > "$T/bad_if.c"
try bad_if

# a very long string literal, and one with every escape
python3 -c "
print('#include <stdio.h>')
print('int main(void) { printf(\"%s\", \"' + 'ab'*20000 + '\"); return 0; }')
" > "$T/long_string.c"
try long_string

# a file that is one very long line: no newline anywhere
python3 -c "
import sys
sys.stdout.write('int main(void) { int x = 0; ' + 'x++; '*5000 + 'return x - 5000; }')
" > "$T/one_line.c"
try one_line

# a struct with a thousand members, and one that contains itself by value
python3 -c "
print('struct S {')
for i in range(1000): print('  int m%d;' % i)
print('};')
print('int main(void){ struct S s; s.m999 = 1; return s.m999 - 1; }')
" > "$T/wide_struct.c"
try wide_struct

printf 'struct S { struct S inner; };\nint main(void){ return 0; }\n' > "$T/self_struct.c"
refuse self_struct

# an #include of a file that does not exist.  Both front ends used to drop
# the line without a word, and the program compiled without its header.
printf '#include "nowhere_at_all.h"\nint main(void){return 0;}\n' > "$T/missing_include.c"
refuse missing_include

# an input that does not exist at all, and a directory where a file goes
bound 20 "$UA" "$T/nothing_here.c" -c >/dev/null 2>&1
case "$?" in 139|138|134|136|11|142) echo "  FAIL missing file crashed"; bad=$((bad+1));; *) ok=$((ok+1));; esac
mkdir -p "$T/adir.c"
bound 20 "$UA" "$T/adir.c" -c >/dev/null 2>&1
case "$?" in 139|138|134|136|11|142) echo "  FAIL directory-as-input crashed"; bad=$((bad+1));; *) ok=$((ok+1));; esac

echo
echo "hostile  survived $ok   crashed-or-hung-or-accepted $bad   known gaps $known"
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
