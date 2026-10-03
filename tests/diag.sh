#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
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
# Diagnostics that do not yet carry what a reader needs, one per line:
# <case> R13-0[b]#NN P0|P1|P2 <what is missing>.  Same contract as
# tests/c99.knownfail and the difftest lists: a listed case may fail, and a
# listed case that starts PASSING is a failure, so closing a gap means
# deleting its line.
# Two ledgers, because the two routes do not agree -- the same split the
# difftest lists needed.  07 is the case that forces it: ua_ref already has
# the position (5:11), the product has neither the position nor the header
# name, so one shared list would be a false green on one route or the other.
if [ "$(basename "${UA:-/tmp/ua_ref}")" = ua_ref ]; then
    KNOWN=$R/tests/diag.knownfail
else
    KNOWN=$R/tests/diag.com.knownfail
fi
T=$(scratch)
. "$R/tests/knownfail.sh"
knownfail_load "$KNOWN" "$T/known.keys" || exit 1
isknown() { knownfail_has "$1"; }
ok=0; bad=0

check() {   # check <name> <want line:col> <want text in message>
    local name=$1 want=$2 msg=$3
    local got
    got=$("$_BOUND" 60 "$UA" -run "$T/$name.c" 2>&1 | head -1)
    case "$got" in
        *"$name.c:$want: error:"*"$msg"*) ok=$((ok+1));;
        *) bad=$((bad+1)); printf "  FAIL %-14s want %s (%s)\n       got  %s\n" \
               "$name" "$want" "$msg" "$got";;
    esac
    # ...and the PYTHON front end says the same thing [S-10 #4].  It used
    # to report the line of its spliced buffer -- `line 553` for line 4 of
    # a six-line file -- so the two front ends disagreed about where an
    # error was even when they agreed that there was one.
    got=$("$_BOUND" 60 python3 -m unisa tape "$T/$name.c" 2>&1 \
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
got=$("$_BOUND" 60 "$UA" -run "$T/cont.c" 2>&1 | head -1)
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
got=$("$_BOUND" 60 "$UA" -run "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 3 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want 3 errors, got %s\n" multi "$got"; fi
lines=$("$_BOUND" 60 "$UA" -run "$T/multi.c" 2>&1 | grep "error:" | sed 's/.*multi\.c:\([0-9]*\):.*/\1/' | tr '\n' ' ')
if [ "$lines" = "3 7 10 " ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want lines 3 7 10, got %s\n" multi-lines "$lines"; fi
# ...and the Python front end reports the same three [S-10 #4]
got=$("$_BOUND" 60 python3 -m unisa tape "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 3 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s (python) want 3 errors, got %s\n" multi "$got"; fi
# -ferror-limit=1 stops after the first, as clang's does
got=$("$_BOUND" 60 "$UA" -ferror-limit=1 -run "$T/multi.c" 2>&1 | grep -c "error:")
if [ "$got" -eq 1 ]; then ok=$((ok+1)); else
    bad=$((bad+1)); printf "  FAIL %-14s want 1 error under -ferror-limit=1, got %s\n" limit "$got"; fi

# R13-0b #31, the other direction: a static that main DOES reach (through a
# forward prototype, defined after main) still reports the undefined function
# it calls.  This is a must-reject probe, so it lives here and not in tests/c,
# where closure/difftest would try to build and run it (fb12-31, the accepted
# direction, stays in tests/c).  The reference at c23f12b missed it: a call
# indented by two spaces was never an edge, main fell out of the reachable set,
# and the program built with a call to text offset 0 that spun forever.
cat > "$T/reach.c" <<'XEOF'
static int a(void);
int main(void) { return a(); }
static int a(void) { return nosuch2(); }
XEOF
got=$("$_BOUND" 20 "$UA" -run "$T/reach.c" 2>&1 | head -1)
case "$got" in
    *"undefined function 'nosuch2'"*) ok=$((ok+1));;
    *) bad=$((bad+1)); printf "  FAIL %-14s want undefined function 'nosuch2' from a static main reaches, got %s\n" reach "$got";;
esac

# Recovery must not turn a wrong program into a hang or a crash.  Damage
# the first 40 corpus programs -- delete their third `;` -- and require a
# diagnosis, exit status 1, no signal, within the bound.  The seeds are
# the files themselves, so a failure names one.
dmg=0; dbad=0
for f in $(ls corpus/c-testsuite/tests/single-exec/*.c 2>/dev/null | head -40); do
    b=$(basename "$f" .c)
    awk 'BEGIN{n=0} { line=$0; out=""; while (match(line, /;/)) { n++; if (n==3) { out=out substr(line,1,RSTART-1); line=substr(line,RSTART+1) } else { out=out substr(line,1,RSTART); line=substr(line,RSTART+1) } } print out line }' "$f" > "$T/dmg_$b.c"
    "$_BOUND" 20 "$UA" "$T/dmg_$b.c" -b lnx/x86_64 -o "$T/dmg.bin" > "$T/dmg.out" 2>&1; rc=$?
    if [ "$rc" -eq 1 ] && grep -q "error:" "$T/dmg.out"; then dmg=$((dmg+1))
    elif [ "$rc" -eq 0 ]; then dmg=$((dmg+1))   # the third `;` was in a comment or a string
    else dbad=$((dbad+1)); printf "  FAIL damaged %-20s exit %s: %s\n" "$b" "$rc" "$(head -1 "$T/dmg.out" | cut -c1-60)"; fi
done
printf "  damaged corpus: %d diagnosed cleanly, %d hung or crashed\n" "$dmg" "$dbad"
[ "$dbad" -eq 0 ] && [ "$dmg" -gt 0 ] && ok=$((ok+1)) || bad=$((bad+1))

# Every rejection carries a position -- and the position is the USER's.
# [R13-0 N8]
#
# The external trial of 0.0.12 found three rejection shapes that named nothing
# a reader could act on:
#
#   reject: not covered: <construct>            (no file:line:col at all)
#   unisacc: error: undefined function 'f'      (no position; and it is the
#                                                call site that matters)
#   CError: line 853: undefined function 'f'    (a position, but 853 is a line
#                                                in the compiler's SPLICED
#                                                buffer; the user's file is 13
#                                                lines long)
#
# The third is worse than the first two and is the reason this check states
# the line it expects rather than merely requiring one: a wrong line sends the
# reader somewhere else (the header of this file already says so).  Each case
# below is a real red example, and the expected line is the one the marker
# line sits on in the file as written.
pos_ok=0; pos_bad=0; pos_known=0; pos_revived=0
poscase() {  # poscase <file> <want line:col> <what must also appear> [<driver>] [<ledger key>]
    local f=$1 want=$2 text=$3 driver=${4:-ua} key=${5:-}
    local got
    # the product is tinycc-shaped: the file comes AFTER -O2 and `-run` closes
    # the line (difftest_o.sh:68).  Getting this wrong makes the compiler
    # compile nothing and the check compare an empty string -- which is how
    # this block first reported three failures that were its own.
    if [ "$driver" = ua ]; then
        got=$("$_BOUND" 30 "$UA" -O2 "$f" -run 2>&1 | head -1)
    elif [ "$driver" = builtin ]; then
        # a fixture directory: drive it the way fb12multi.sh does, or the case
        # tests something else entirely (this is the mistake the block above
        # records -- three phantom failures from the wrong command line)
        local dd; dd="$(cd "$(dirname "$f")" && pwd)"
        got=$(cd "$dd" && "$_BOUND" 45 sh -c "UC='$UA'; $(cat build 2>/dev/null || cat run)" 2>&1 | grep -m1 . )
    else
        got=$("$_BOUND" 30 python3 -m unisa run "$f" --drive built 2>&1 | head -1)
    fi
    # a fixture directory's source is main.c, and every one of them is main.c;
    # the ledger names the FIXTURE, so those cases pass their key explicitly
    local name; if [ -n "$key" ]; then name="$key"; else name="$(basename "$f" .c)"; fi
    case "$got" in
        *"$(basename "$f"):$want"*"$text"*)
            pos_ok=$((pos_ok+1))
            if isknown "$name"; then
                pos_revived=$((pos_revived+1))
                printf "  note %-12s listed in diag.knownfail but now correct: delete its line\n" "$name"
            fi;;
        *) if isknown "$name"; then
               pos_known=$((pos_known+1))
               printf "  known %-11s %s\n" "$name" "$(grep -m1 "^$name[[:space:]]" "$KNOWN" | cut -c1-56)"
           else
               pos_bad=$((pos_bad+1))
               printf "  FAIL %-12s want %s:%s (%s)\n       got  %s\n" \
                   "$name" "$(basename "$f")" "$want" "$text" "$got"
           fi;;
    esac
}
red=$R/tests/c
# 04 and 05 used to be here, asking for the position of a diagnostic.  Both
# constructs are now SUPPORTED on dev5 -- `*pp = b` and `*pp = mk(11)` compile,
# `int m[n][4]` compiles -- and both programs run to the same exit status as
# gcc (22 and 8, which are their return values).  There is no diagnostic left
# to locate, so the assertion lost its subject; the probes stay in tests/c as
# red examples for the difftest ledgers, and their positions are no longer
# this suite's business.
# 11/29/31 used to be here too (the `reject: not covered:` shape and the
# positionless `undefined function`).  All three constructs are SUPPORTED on
# both routes since 0.0.16 (R16-10, 2026-10-01): the programs compile and run
# to gcc's output and exit status, so there is no diagnostic left to locate;
# the probes stay in tests/c as red examples for the difftest ledgers.
# The former 28/35/36/34/24 diagnostics are no longer applicable: those
# constructs now compile.  fb12multi checks their output or target image.
# In particular, asking for a diagnostic from the nested-include fixture used
# to read `run` from lib/sub instead of the fixture root, a false green.
# 07's own probe compiles since include/locale.h exists (8025200); the shape it
# stood for -- a header that is not there -- keeps its case on a header nobody
# bundles, on both routes: the reference through err_at, the product through
# E2's DG.inc (the same file:line:col, the same words).
{ echo '/* a header that no distribution bundles */'; echo; echo; echo; echo '#include <no_such_header_zz.h>'; echo 'int main(void) { return 0; }'; } > "$T/nohdr.c"
poscase "$T/nohdr.c" "5:11" "no such file for #include"
printf "  rejection positions: %d named correctly, %d known missing, %d wrong\n" \
    "$pos_ok" "$pos_known" "$pos_bad"
[ "$pos_bad" -eq 0 ] && [ $((pos_ok + pos_known)) -gt 0 ] && [ "$pos_revived" -eq 0 ] \
    && ok=$((ok+1)) || bad=$((bad+1))

# R14-0: the old positioned refusal is now a successful six-argument call.
want=$(printf '15\n16')
got=$("$_BOUND" 60 "$UA" -run "$red/fb12-21-indirect-call-six-args.c" 2>"$T/indirect6.err"); rc=$?
if [ "$rc" -eq 0 ] && [ "$got" = "$want" ]; then ok=$((ok+1));
else bad=$((bad+1)); echo "  FAIL indirect6: rc=$rc output=[$got]"; fi

echo
echo "diag  ok $ok   wrong $bad"
# A suite that checked nothing is not green: `closure.sh` with no
# probes once printed `identical 0 differ 0` and exited 0.
[ "$bad" -eq 0 ] && [ "$ok" -gt 0 ]
