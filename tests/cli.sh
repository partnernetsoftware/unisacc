#!/bin/bash
# The compiler as a TOOL, the way tcc is one. [A-39] [S-11]
#
# Everything here runs from a scratch directory that contains no part of this
# repository: a compiler people can use is one that works when it is the only
# file you have.  That is why the headers travel inside the binary -- a
# shipped `unisacc` has no include/ to read.
#
# Checked: the built-in headers, `-I`, `-D` (bare and with a value), a
# shebang line, argv passing, the exit status, and a diagnostic on a file
# that does not exist.
set -u
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
ok=0; bad=0
say() {   # say <name> <want> <got>
    if [ "$2" = "$3" ]; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %-22s want [%s] got [%s]\n" "$1" "$2" "$3"; fi
}
run() { (cd "$T" && bound 20 "$UA_RUN" -run "$@" 2>&1); }

mkdir -p "$T/inc"
printf '#define GREETING "hello from an include dir"\n' > "$T/inc/greet.h"

# the C library we carry: none of these files exist next to the compiler
cat > "$T/lib.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
int main(void) {
    char b[32];
    strcpy(b, "carried");
    printf("%s %d %.3f\n", b, (int)strlen(b), sqrt(2.0));
    return 0;
}
EOF
say "built-in headers" "carried 7 1.414" "$(run lib.c)"

cat > "$T/inc.c" <<'EOF'
#include <stdio.h>
#include "greet.h"
int main(void) { printf("%s\n", GREETING); return 0; }
EOF
say "-I dir"  "hello from an include dir" "$(run -I inc inc.c)"
say "-Idir"   "hello from an include dir" "$(run -Iinc inc.c)"

cat > "$T/def.c" <<'EOF'
#include <stdio.h>
#ifdef FEATURE
int main(void) { printf("on %d\n", LEVEL); return 0; }
#else
int main(void) { printf("off\n"); return 0; }
#endif
EOF
say "-D bare + value" "on 7"  "$(run -D FEATURE -D LEVEL=7 def.c)"
say "-Dname=value"    "on 42" "$(run -DFEATURE -DLEVEL=42 def.c)"
say "without -D"      "off"   "$(run def.c)"

cat > "$T/args.c" <<'EOF'
#!/usr/bin/env unisacc
#include <stdio.h>
int main(int argc, char **argv) {
    int i;
    printf("%d:", argc);
    for (i = 1; i < argc; i++) printf(" %s", argv[i]);
    printf("\n");
    return argc;
}
EOF
say "shebang + argv" "3: one two" "$(run args.c one two)"
run args.c a b c >/dev/null; say "exit status" "4" "$?"

got=$(run nosuch.c); case "$got" in *"cannot open"*) got="diagnosed";; esac
say "missing input" "diagnosed" "$got"

# -E: the preprocessor on its own, the way cc's is.  A build system that
# runs `CC -E` to chase dependencies gets text, not a refusal.
cat > "$T/pp.c" <<'EOF'
#define TWICE(x) ((x) + (x))
int v = TWICE(21);
EOF
got=$( (cd "$T" && bound 60 "$UA_RUN" -E pp.c 2>&1) | tr -d ' \n')
# spacing between tokens is the preprocessor's own (C leaves it
# unspecified); what matters is that the argument was substituted twice
case "$got" in *"intv=((21)+(21))"*) got="expanded";; esac
say "-E preprocesses" "expanded" "$got"

# the flags a Makefile passes that mean nothing here must not be refused:
# there is one dialect, one optimisation level, no separate debug info
say "-Wall -O2 -g -std" "ok" "$( (cd "$T" && bound 60 \
    "$UA_RUN" -Wall -O2 -g -std=c99 -run def.c 2>&1) | sed 's/^off$/ok/')"

# -S is the name for "write the tape", the assembly-level IR, and -c is
# only a synonym -- there are no object files here [S-10 #7]
a=$( (cd "$T" && bound 30 "$UA_RUN" def.c -S) | head -1)
b=$( (cd "$T" && bound 30 "$UA_RUN" def.c -c) | head -1)
say "-S writes the tape" "_start:" "$a"
say "-c is the same" "$a" "$b"

# The rest of what a Makefile passes [S-15 C1]: -U undoes a -D (and a
# predefined name), -include prepends a file, -l/-L/-x are accepted, `-` is
# stdin and `-o -` is stdout, -nostdinc drops the built-in headers.
say "-U undoes -D"  "off" "$(run -DFEATURE -UFEATURE def.c)"
# Every target predefinition must be removable, including those after Linux's.
for target in lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64; do
    case "$target" in lnx/*) names="__linux__ __unix__ __ELF__";; osx/*) names="__APPLE__ __MACH__ __unix__";; win/*) names="_WIN32 _WIN64";; esac
    case "$target" in */x86_64) names="$names __x86_64__";; *) names="$names __aarch64__";; esac
    for name in $names __LP64__ __UNISA__; do
        printf '#ifdef %s\n1\n#else\n0\n#endif\n' "$name" > "$T/predef.c"
        got=$(cd "$T" && bound 30 "$UA_RUN" -b "$target" -U "$name" -E predef.c 2>&1)
        rc=$?
        if [ "$rc" != 0 ]; then got="exit $rc: $got"; else got=$(printf '%s' "$got" | tr -d '[:space:]'); fi
        say "-U $target $name" "0" "$got"
    done
done
printf '#define LEVEL 9\n' > "$T/pre.h"
say "-include"      "on 9" "$(run -DFEATURE -include pre.h def.c)"
say "-lm -L -x c accepted" "off" "$(run -lm -L/nowhere -x c def.c)"
say "stdin as input" "from stdin" \
    "$( (cd "$T" && printf '#include <stdio.h>\nint main(void){puts("from stdin");return 0;}\n' \
        | bound 60 "$UA_RUN" -run - 2>&1) )"
say "-o - is stdout" "_start:" \
    "$( (cd "$T" && bound 60 "$UA_RUN" def.c -S -o - 2>&1) | head -1)"
got=$( (cd "$T" && bound 60 "$UA_RUN" -nostdinc lib.c -S 2>&1) | head -1)
case "$got" in *"no such file for #include"*) got="refused, as cc does";; esac
say "-nostdinc refuses <stdio.h>" "refused, as cc does" "$got"
# -nostdinc and printf with no declaration: the only path left to the
# compile-time lowering (do_printf, W-9) since printf became a real call
printf 'int main(void){ int n; n = 42; printf("hi %%d %%s%%c\\n", n, "ok", 33); return 0; }\n' > "$T/nsi.c"
say "-nostdinc printf lowered" "hi 42 ok!" "$( (cd "$T" && bound 60 "$UA_RUN" -nostdinc -run nsi.c 2>&1) )"

# -MD writes what make wants: `target: input headers`, only files that were
# really opened (the built-in header copies are not files) [S-15 C2]
(cd "$T" && bound 60 "$UA_RUN" -MD inc.c -S -I inc -o inc.tape >/dev/null 2>&1)
say "-MD names the target" "inc.tape:" "$(head -1 "$T/inc.d" 2>/dev/null | cut -d' ' -f1)"   # gcc's .d: one line, names separated by spaces (R16-11)
say "-MD lists the header" "greet.h" "$(grep -o 'greet.h' "$T/inc.d" 2>/dev/null | head -1)"

# no #include at all, outside the repo: the header is found on demand from
# the copies we carry.  autoinc only tried ./include/, so this program got
# no strcpy, and -run spun in the garbage the missing label resolved to
printf 'int main(void){ char b[8]; strcpy(b, "auto"); printf("%%s %%d\\n", b, (int)strlen(b)); return 0; }\n' > "$T/noinc.c"
say "libc with no #include" "auto 4" "$(run noinc.c)"
# a call nobody defines is an error, not a jump to offset 0
printf 'int main(void){ return nosuchfn(3); }\n' > "$T/undef.c"
got=$( (cd "$T" && bound 30 "$UA_RUN" undef.c -S -o undef.tape 2>&1; echo "rc=$?") | tr '\n' ' ')
case "$got" in *"undefined function 'nosuchfn'"*"rc=1"*) got=refused;; esac
say "undefined function" "refused" "$got"

# which build is this: a release has to be identifiable from the binary
got=$( (cd "$T" && bound 30 "$UA_RUN" --version 2>&1) )
case "$got" in "unisacc "[0-9]*) got="a version";; esac
say "--version" "a version" "$got"

# No mode at all: RUN (owner decision 2026-10-01, R17-10) -- deliberately not
# cc's silent a.out; a file is written only with -o / -b / -S / -c / -E.
printf '#include <stdio.h>\nint main(void){puts("plain build");return 0;}\n' > "$T/plain.c"
rm -f "$T/a.out"
say "FILE.c alone runs" "plain build" "$( (cd "$T" && bound 60 "$UA_RUN" plain.c) 2>&1)"
say "FILE.c alone leaves no a.out" "none" "$([ -e "$T/a.out" ] && echo left || echo none)"
rm -f "$T/runout" "$T/runs" "$T/runc" "$T/rune" "$T/runb"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -o runout >/dev/null 2>&1)
say "-run -o leaves no file" "none" "$([ -e "$T/runout" ] && echo left || echo none)"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -S -o runs >/dev/null 2>&1)
say "-run -S leaves no file" "none" "$([ -e "$T/runs" ] && echo left || echo none)"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -c -o runc >/dev/null 2>&1)
say "-run -c leaves no file" "none" "$([ -e "$T/runc" ] && echo left || echo none)"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -E -o rune >/dev/null 2>&1)
say "-run -E leaves no file" "none" "$([ -e "$T/rune" ] && echo left || echo none)"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -b lnx/x86_64 -o runb >/dev/null 2>&1)
say "-run -b leaves no file" "none" "$([ -e "$T/runb" ] && echo left || echo none)"
printf 'KEEP' > "$T/runkeep"
(cd "$T" && bound 20 "$UA_RUN" plain.c -run -o runkeep >/dev/null 2>&1)
say "-run leaves an existing file" "KEEP" "$(cat "$T/runkeep" 2>/dev/null)"
printf 'int ;\n' > "$T/nomain.c"
printf 'KEEP' > "$T/nomain.out"
nomain_out=$(cd "$T" && bound 20 "$UA_RUN" nomain.c -o nomain.out 2>"$T/nomain.err")
say "refusal leaves an existing file" "KEEP" "$(cat "$T/nomain.out" 2>/dev/null)"
say "refusal stdout is empty" "" "$nomain_out"
say "refusal is named" "named" "$(grep -q 'not covered:' "$T/nomain.err" && echo named)"
printf 'int f(void){return;}\n' > "$T/bare.c"
printf 'KEEP' > "$T/bare.s"
bare_out=$(cd "$T" && bound 20 "$UA_RUN" -S bare.c -o bare.s 2>"$T/bare.err")
say "assembly refusal leaves an existing file" "KEEP" "$(cat "$T/bare.s" 2>/dev/null)"
say "assembly refusal stdout is empty" "" "$bare_out"
say "assembly refusal is named" "named" "$(grep -q 'not covered:' "$T/bare.err" && echo named)"
printf 'KEEP' > "$T/bare.o"
(cd "$T" && bound 20 "$UA_RUN" -c bare.c -o bare.o >/dev/null 2>&1)
say "object refusal leaves an existing file" "KEEP" "$(cat "$T/bare.o" 2>/dev/null)"
printf 'int main(void){return 0;}\n' > "$T/ok.c"
printf 'KEEP' > "$T/ok.out"
ok_out=$(cd "$T" && bound 20 "$UA_RUN" ok.c -o ok.out 2>"$T/ok.err")
say "success replaces an existing file" "ELF" "$(od -An -tx1 -N4 "$T/ok.out" | tr -d ' \n' | grep -q '^7f454c46$' && echo ELF || echo kept)"
say "success stdout is empty" "" "$ok_out"
say "success stderr is empty" "0" "$(wc -c < "$T/ok.err" | tr -d ' ')"
printf 'KEEP' > "$T/ok.s"
s_out=$(cd "$T" && bound 20 "$UA_RUN" -S ok.c -o ok.s 2>"$T/ok.serr")
say "assembly replaces an existing file" "_start:" "$(head -1 "$T/ok.s")"
say "assembly success stdout is empty" "" "$s_out"
say "assembly success stderr is empty" "0" "$(wc -c < "$T/ok.serr" | tr -d ' ')"
printf 'KEEP' > "$T/ok.e"
e_out=$(cd "$T" && bound 20 "$UA_RUN" -E ok.c -o ok.e 2>"$T/ok.eerr")
say "preprocess replaces an existing file" "same" "$(cmp -s "$T/ok.c" "$T/ok.e" && echo same || echo kept)"
say "preprocess success stdout is empty" "" "$e_out"
say "preprocess success stderr is empty" "0" "$(wc -c < "$T/ok.eerr" | tr -d ' ')"
printf 'KEEP' > "$T/ok.o"
c_out=$(cd "$T" && bound 20 "$UA_RUN" -c ok.c -o ok.o 2>"$T/ok.cerr")
say "object success replaces an existing file" "replaced" "$(grep -q KEEP "$T/ok.o" && echo KEEP || echo replaced)"
say "object success stdout is empty" "" "$c_out"
say "object success stderr is empty" "0" "$(wc -c < "$T/ok.cerr" | tr -d ' ')"
(cd "$T" && bound 20 "$UA_RUN" ok.c -o ok.out2 >/dev/null 2>&1)
say "link bytes repeat" "same" "$(cmp -s "$T/ok.out" "$T/ok.out2" && echo same || echo differ)"
(cd "$T" && bound 20 "$UA_RUN" -S ok.c -o ok.s2 >/dev/null 2>&1)
say "assembly bytes repeat" "same" "$(cmp -s "$T/ok.s" "$T/ok.s2" && echo same || echo differ)"
(cd "$T" && bound 20 "$UA_RUN" -E ok.c -o ok.e2 >/dev/null 2>&1)
say "preprocess bytes repeat" "same" "$(cmp -s "$T/ok.e" "$T/ok.e2" && echo same || echo differ)"
(cd "$T" && bound 20 "$UA_RUN" -c ok.c -o ok.o2 >/dev/null 2>&1)
say "object bytes repeat" "same" "$(cmp -s "$T/ok.o" "$T/ok.o2" && echo same || echo differ)"
printf 'int main(void){return 0;}\n' > "$T/pathmark_9f3a.c"
(cd "$T" && bound 20 "$UA_RUN" pathmark_9f3a.c -o pathmark.out >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -S pathmark_9f3a.c -o pathmark.s >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -c pathmark_9f3a.c -o pathmark.o >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -E pathmark_9f3a.c -o pathmark.e >/dev/null 2>&1)
pathhit=no
grep -a -q pathmark_9f3a "$T/pathmark.out" "$T/pathmark.s" "$T/pathmark.o" "$T/pathmark.e" && pathhit=yes
say "outputs omit the source path" "no" "$pathhit"
mkdir -p "$T/dirmark_c4e1"
printf 'int k;\n' > "$T/dirmark_c4e1/local.h"
printf '#include "local.h"\nint main(void){return k;}\n' > "$T/dirmark_c4e1/use.c"
(cd "$T" && bound 20 "$UA_RUN" -E dirmark_c4e1/use.c -o inc.e >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -S dirmark_c4e1/use.c -o inc.s >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -c dirmark_c4e1/use.c -o inc.o >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" dirmark_c4e1/use.c -o inc.out >/dev/null 2>&1)
dirhit=no
grep -a -q dirmark_c4e1 "$T/inc.e" "$T/inc.s" "$T/inc.o" "$T/inc.out" && dirhit=yes
say "outputs omit the include directory" "no" "$dirhit"
printf '#include <stdio.h>\nint main(void){return 0;}\n' > "$T/sys.c"
(cd "$T" && bound 20 "$UA_RUN" -E sys.c -o sys.e >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -S sys.c -o sys.s >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -c sys.c -o sys.o >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" sys.c -o sys.out >/dev/null 2>&1)
syshit=no
grep -a -q -e /usr -e /lib -e stdio.h "$T/sys.e" "$T/sys.s" "$T/sys.o" "$T/sys.out" && syshit=yes
say "system header outputs omit host paths" "no" "$syshit"
mkdir -p "$T/iquote_d17b"
printf 'int k;\n' > "$T/iquote_d17b/q.h"
printf '#include "q.h"\nint main(void){return k;}\n' > "$T/iq.c"
(cd "$T" && bound 20 "$UA_RUN" -E -I iquote_d17b iq.c -o iq.e >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -E -I iquote_d17b iq.c -o iq.e2 >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -S -I iquote_d17b iq.c -o iq.s >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -c -I iquote_d17b iq.c -o iq.o >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -I iquote_d17b iq.c -o iq.out >/dev/null 2>&1)
iqhit=no
grep -a -q iquote_d17b "$T/iq.e" "$T/iq.s" "$T/iq.o" "$T/iq.out" && iqhit=yes
say "outputs omit the dash-I directory" "no" "$iqhit"
say "dash-I preprocess bytes repeat" "same" "$(cmp -s "$T/iq.e" "$T/iq.e2" && echo same || echo differ)"
printf '#include "missing_q.h"\nint main(void){return 0;}\n' > "$T/missq.c"
printf 'KEEP' > "$T/missq.e"
missq_out=$(cd "$T" && bound 20 "$UA_RUN" -E missq.c -o missq.e 2>"$T/missq.err")
say "missing quote include leaves an existing file" "KEEP" "$(cat "$T/missq.e" 2>/dev/null)"
say "missing quote include stdout is empty" "" "$missq_out"
say "missing quote include is named" "named" "$(grep -q 'no such file' "$T/missq.err" && echo named)"
mkdir -p "$T/force_a91c"
printf 'int from_inc;\n' > "$T/force_a91c/force.h"
printf 'int main(void){return from_inc;}\n' > "$T/force.c"
(cd "$T" && bound 20 "$UA_RUN" -E -include force_a91c/force.h force.c -o force.e >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -E -include force_a91c/force.h force.c -o force.e2 >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -S -include force_a91c/force.h force.c -o force.s >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -c -include force_a91c/force.h force.c -o force.o >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -include force_a91c/force.h force.c -o force.out >/dev/null 2>&1)
forcehit=no
grep -a -q force_a91c "$T/force.e" "$T/force.s" "$T/force.o" "$T/force.out" && forcehit=yes
say "forced include omits its directory" "no" "$forcehit"
say "forced include is in the stream" "yes" "$(grep -q from_inc "$T/force.e" && echo yes)"
say "forced include bytes repeat" "same" "$(cmp -s "$T/force.e" "$T/force.e2" && echo same || echo differ)"
(cd "$T" && bound 20 "$UA_RUN" -S -include force_a91c/force.h force.c -o force.s2 >/dev/null 2>&1)
say "forced include reaches assembly" "yes" "$(grep -q from_inc "$T/force.s" && echo yes)"
say "forced include assembly repeats" "same" "$(cmp -s "$T/force.s" "$T/force.s2" && echo same || echo differ)"
(cd "$T" && bound 20 "$UA_RUN" -c -include force_a91c/force.h force.c -o force.o2 >/dev/null 2>&1)
(cd "$T" && bound 20 "$UA_RUN" -include force_a91c/force.h force.c -o force.out2 >/dev/null 2>&1)
say "forced include reaches object" "yes" "$(grep -a -q from_inc "$T/force.o" && echo yes)"
say "forced include object repeats" "same" "$(cmp -s "$T/force.o" "$T/force.o2" && echo same || echo differ)"
say "forced include link repeats" "same" "$(cmp -s "$T/force.out" "$T/force.out2" && echo same || echo differ)"
printf 'KEEP' > "$T/forcemiss.e"
forcemiss_out=$(cd "$T" && bound 20 "$UA_RUN" -E -include nope.h force.c -o forcemiss.e 2>"$T/forcemiss.err")
say "missing forced include leaves an existing file" "KEEP" "$(cat "$T/forcemiss.e" 2>/dev/null)"
say "missing forced include stdout is empty" "" "$forcemiss_out"
say "missing forced include is named" "named" "$(grep -q 'no such file' "$T/forcemiss.err" && echo named)"
printf '#include <no-such-hdr.h>\n' > "$T/miss.c"
printf 'KEEP' > "$T/miss.e"
miss_out=$(cd "$T" && bound 20 "$UA_RUN" -E miss.c -o miss.e 2>"$T/miss.err")
say "missing include leaves an existing file" "KEEP" "$(cat "$T/miss.e" 2>/dev/null)"
say "missing include stdout is empty" "" "$miss_out"
say "missing include is named" "named" "$(grep -q 'no such file' "$T/miss.err" && echo named)"
printf '#include <stdio.h>\nint main(int c,char**v){printf("%%d %%s\\n",c,v[1]);return 0;}\n' > "$T/argv.c"
say "FILE.c args go to the program" "2 hi" "$( (cd "$T" && bound 60 "$UA_RUN" argv.c hi) 2>&1)"
say "-- passes dash arguments" "2 --flag" "$( (cd "$T" && bound 60 "$UA_RUN" argv.c -- --flag) 2>&1)"
say "dash argument without -- is named" "go after --" "$( (cd "$T" && bound 60 "$UA_RUN" argv.c --flag) 2>&1 | grep -o 'go after --' | head -1)"
(cd "$T" && bound 60 "$UA_RUN" plain.c -o plainx >/dev/null 2>&1)
say "FILE.c -o names it" "plain build" "$( (cd "$T" && bound 10 ./plainx) 2>&1)"
say "-dump-tokens" "6 tokens" "$( (cd "$T" && printf 'int x = 1;\n' > tk.c && bound 20 "$UA_RUN" -dump-tokens tk.c) | tail -1)"

# a failed compile leaves nothing behind; a missing input is named on stderr
printf 'int main(void){ int x = ; }\n' > "$T/bad.c"; rm -f "$T/a.out"
(cd "$T" && bound 60 "$UA_RUN" bad.c >/dev/null 2>&1)
say "error leaves no a.out" "none" "$([ -e "$T/a.out" ] && echo left || echo none)"
say "missing input on stderr" "unisacc: error: cannot open nope.c" \
    "$( (cd "$T" && bound 20 "$UA_RUN" nope.c 2>&1 >/dev/null) | head -1)"

echo
echo "cli  ok $ok   wrong $bad   (from a scratch dir, no repo in sight)"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
