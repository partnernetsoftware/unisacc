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
UA_RUN=${UA_RUN:-/tmp/ua_ref}
[ -x "$UA_RUN" ] || "$R/tests/build_ref.sh" >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0
say() {   # say <name> <want> <got>
    if [ "$2" = "$3" ]; then ok=$((ok+1))
    else bad=$((bad+1)); printf "  FAIL %-22s want [%s] got [%s]\n" "$1" "$2" "$3"; fi
}
run() { (cd "$T" && perl -e 'alarm 120; exec @ARGV' "$UA_RUN" -run "$@" 2>&1); }

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

echo
echo "cli  ok $ok   wrong $bad   (from a scratch dir, no repo in sight)"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
