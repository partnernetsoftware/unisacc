#!/bin/bash
# What the compiler says when the program is wrong. [A-40] [S-12]
#
# An error has to name the user's file, the user's line and the column, and
# show the line -- the shape every C programmer already reads:
#
#     prog.c:4:9: error: unknown identifier
#       x = undefined_thing + 1;
#           ^
#
# The buffer the parser sees is NOT that file: `#include` is replaced by the
# header's text (hundreds of lines), the compiler adds headers of its own,
# and continuation lines are joined.  So each case here states the line it
# expects, and the suite checks the compiler agrees -- a wrong line is worse
# than no line, because it sends the reader somewhere else.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
UA=${UA:-/tmp/ua_ref}
[ -x "$UA" ] || ./tests/build_ref.sh >/dev/null || exit 1
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
ok=0; bad=0

check() {   # check <name> <want line:col> <want text in message>
    local name=$1 want=$2 msg=$3
    local got
    got=$(perl -e 'alarm 60; exec @ARGV' "$UA" -run "$T/$name.c" 2>&1 | head -1)
    case "$got" in
        *"$name.c:$want: error:"*"$msg"*) ok=$((ok+1));;
        *) bad=$((bad+1)); printf "  FAIL %-14s want %s (%s)\n       got  %s\n" \
               "$name" "$want" "$msg" "$got";;
    esac
}

cat > "$T/ident.c" <<'EOF'
#include <stdio.h>
int main(void) {
    int x;
    x = undefined_thing + 1;
    return x;
}
EOF
check ident "4:9" "unknown identifier"

cat > "$T/semi.c" <<'EOF'
#include <stdio.h>
int main(void) {
    int y = 3
    return y;
}
EOF
check semi "4:5" "expected ';'"

# no include at all: the line must still be the user's
cat > "$T/bare.c" <<'EOF'
int main(void) {
    return nope;
}
EOF
check bare "2:12" "unknown identifier"

# an error AFTER a header, with the compiler's own auto-includes in play
cat > "$T/deep.c" <<'EOF'
#include <stdio.h>
#include <string.h>
int helper(int a) { return a + 1; }
int main(void) {
    char b[8];
    strcpy(b, "hi");
    printf("%s\n", b);
    return helper(missing);
}
EOF
check deep "8:19" "unknown identifier"

# a continuation line joins two physical lines: the ones after it must not
# drift
cat > "$T/cont.c" <<'EOF'
#include <stdio.h>
#define TWO(a, b) \
    ((a) + (b))
int main(void) {
    return TWO(1, 2) + gone;
}
EOF
# The line is what matters here.  The COLUMN is measured in the line the
# parser saw, and on a line where a macro expanded that is not the line the
# user typed -- `TWO(1, 2)` became `((1) + (2))`.  The message prints that
# same expanded line, so the caret still points at the right token; it just
# is not the file's column.  Checking the line only says exactly that.
got=$(perl -e 'alarm 60; exec @ARGV' "$UA" -run "$T/cont.c" 2>&1 | head -1)
case "$got" in
    *"cont.c:5:"*"unknown identifier"*) ok=$((ok+1));;
    *) bad=$((bad+1)); printf "  FAIL %-14s want line 5\n       got  %s\n" cont "$got";;
esac

echo
echo "diag  ok $ok   wrong $bad"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
