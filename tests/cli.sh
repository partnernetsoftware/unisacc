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
say "-Wall -O2 -g -std" "ok" "$( (cd "$T" && bound 120 \
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
printf '#define LEVEL 9\n' > "$T/pre.h"
say "-include"      "on 9" "$(run -DFEATURE -include pre.h def.c)"
say "-lm -L -x c accepted" "off" "$(run -lm -L/nowhere -x c def.c)"
say "stdin as input" "from stdin" \
    "$( (cd "$T" && printf '#include <stdio.h>\nint main(void){puts("from stdin");return 0;}\n' \
        | bound 120 "$UA_RUN" -run - 2>&1) )"
say "-o - is stdout" "_start:" \
    "$( (cd "$T" && bound 120 "$UA_RUN" def.c -S -o - 2>&1) | head -1)"
got=$( (cd "$T" && bound 120 "$UA_RUN" -nostdinc lib.c -S 2>&1) | head -1)
case "$got" in *"no such file for #include"*) got="refused, as cc does";; esac
say "-nostdinc refuses <stdio.h>" "refused, as cc does" "$got"

# -MD writes what make wants: `target: input headers`, only files that were
# really opened (the built-in header copies are not files) [S-15 C2]
(cd "$T" && bound 120 "$UA_RUN" -MD inc.c -S -I inc -o inc.tape >/dev/null 2>&1)
say "-MD names the target" "inc.tape:" "$(head -1 "$T/inc.d" 2>/dev/null | tr -d ' \\')"
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

# No mode at all: what `cc FILE.c` does -- a.out for this machine, not the
# lexer's token dump (which is -dump-tokens now)
printf '#include <stdio.h>\nint main(void){puts("plain build");return 0;}\n' > "$T/plain.c"
rm -f "$T/a.out"
(cd "$T" && bound 60 "$UA_RUN" plain.c >/dev/null 2>&1)
say "FILE.c alone writes a.out" "plain build" "$( (cd "$T" && bound 10 ./a.out) 2>&1)"
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
