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
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
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
    # ...and the PYTHON front end says the same thing [S-10 #4].  It used
    # to report the line of its spliced buffer -- `line 553` for line 4 of
    # a six-line file -- so the two front ends disagreed about where an
    # error was even when they agreed that there was one.
    got=$(perl -e 'alarm 120; exec @ARGV' python3 -m unisa tape "$T/$name.c" 2>&1 \
          | grep -m1 "error:")
    case "$got" in
        *"$name.c:$want: error:"*"$msg"*) ok=$((ok+1));;
        *) bad=$((bad+1)); printf "  FAIL %-14s (python) want %s (%s)\n       got  %s\n" \
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

# More than one error per run [S-15 C3]: three functions, three mistakes,
# all three reported with the right line, and nothing after the third.
cat > "$T/multi.c" <<'XEOF'
#include <stdio.h>
int one(int a) {
    return a + nope1;
}
int two(int b) {
    int c = b
    return c;
}
int three(void) {
    return nope3;
}
int main(void) { return one(1) + two(2) + three(); }
XEOF
got=$(perl -e 'alarm 60; exec @ARGV' "$UA" -run "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 3 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want 3 errors, got %s\n" multi "$got"; fi
lines=$(perl -e 'alarm 60; exec @ARGV' "$UA" -run "$T/multi.c" 2>&1 | grep "error:" | sed 's/.*multi\.c:\([0-9]*\):.*/\1/' | tr '\n' ' ')
if [ "$lines" = "3 7 10 " ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want lines 3 7 10, got %s\n" multi-lines "$lines"; fi
# ...and the Python front end reports the same three [S-10 #4]
got=$(perl -e 'alarm 120; exec @ARGV' python3 -m unisa tape "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 3 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s (python) want 3 errors, got %s\n" multi "$got"; fi
# -ferror-limit=1 stops after the first, as clang's does
got=$(perl -e 'alarm 60; exec @ARGV' "$UA" -ferror-limit=1 -run "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 1 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want 1 error under -ferror-limit=1, got %s\n" limit "$got"; fi

# Recovery must not turn a wrong program into a hang or a crash.  Damage
# the first 40 corpus programs -- delete their third `;` -- and require a
# diagnosis, exit status 1, no signal, within the bound.  The seeds are
# the files themselves, so a failure names one.
dmg=0; dbad=0
for f in $(ls corpus/c-testsuite/tests/single-exec/*.c 2>/dev/null | head -40); do
    b=$(basename "$f" .c)
    awk 'BEGIN{n=0} { line=$0; out=""; while (match(line, /;/)) { n++; if (n==3) { out=out substr(line,1,RSTART-1); line=substr(line,RSTART+1) } else { out=out substr(line,1,RSTART); line=substr(line,RSTART+1) } } print out line }' "$f" > "$T/dmg_$b.c"
    perl -e 'alarm 20; exec @ARGV' "$UA" "$T/dmg_$b.c" -b lnx/x86_64 -o "$T/dmg.bin" > "$T/dmg.out" 2>&1; rc=$?
    if [ "$rc" -eq 1 ] && grep -q "error:" "$T/dmg.out"; then dmg=$((dmg+1))
    elif [ "$rc" -eq 0 ]; then dmg=$((dmg+1))   # the third `;` was in a comment or a string
    else dbad=$((dbad+1)); printf "  FAIL damaged %-20s exit %s: %s\n" "$b" "$rc" "$(head -1 "$T/dmg.out" | cut -c1-60)"; fi
done
printf "  damaged corpus: %d diagnosed cleanly, %d hung or crashed\n" "$dmg" "$dbad"
[ "$dbad" -eq 0 ] && [ "$dmg" -gt 0 ] && ok=$((ok+1)) || bad=$((bad+1))

echo
echo "diag  ok $ok   wrong $bad"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
