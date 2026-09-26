#!/bin/sh
# exec/c/neg.sh -- the error paths of the two executors, which no real table
# reaches: each tiny delta must end the same way in exec/c/run.c and in
# exec/pp/sim.py (bad table / unreadable file: 2; reject: 1).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "neg: cc failed"; exit 1; }
printf 'x' > "$T/in"
ok=0; bad=0
t() {   # t NAME WANT C_STDERR_PREFIX PY_STDERR_PREFIX JSON [INCLUDE_DIR] -- the first stderr line names the path taken
    printf '%s' "$5" > "$T/d.json"
    b 60 python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl" || { echo "neg: tbl $1 failed"; bad=$((bad+1)); return; }
    b 10 "$T/run" "$T/d.tbl" "$T/in" "$T/in" ${6:-} >/dev/null 2>"$T/ce"; c=$?
    b 10 python3 exec/pp/sim.py "$T/d.json" "$T/in" ${6:-} >/dev/null 2>"$T/pe"; p=$?
    ce=$(head -1 "$T/ce"); pe=$(head -1 "$T/pe")
    # an empty expected prefix means stderr must be empty (""* would match anything)
    if [ -z "$3" ]; then [ -z "$ce" ] && cm=1 || cm=0; else case "$ce" in "$3"*) cm=1 ;; *) cm=0 ;; esac; fi
    if [ -z "$4" ]; then [ -z "$pe" ] && pm=1 || pm=0; else case "$pe" in "$4"*) pm=1 ;; *) pm=0 ;; esac; fi
    if [ $c -eq "$2" ] && [ $p -eq "$2" ] && [ $cm = 1 ] && [ $pm = 1 ]; then ok=$((ok+1))
    else bad=$((bad+1)); echo "  FAIL $1: run.c $c [$ce], sim.py $p [$pe], want $2 [$3] [$4]"; fi
}
t pop-empty 2 "run: pop of an empty stack" "run: pop of an empty stack" '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["POP"]]]}'
# an empty bundled-header name is the include directory itself: fopen succeeds
# (macOS and glibc), the read fails -- run.c's ferror branch, 'cannot read'
# ('cannot open' is the errno branch)
t read-dir 2 "run: cannot read" "run: [Errno 21]" '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["SBCLR"],["SBOUT",0],["SBOUT",104],["SBOUT",100],["SBOUT",114],["SBOUT",47],["SBFIND","f"]]]}' "$R/include"
# the same name with a plain FILE as the include directory: fopen fails with ENOTDIR -- the errno branch
t open-notdir 2 "run: cannot open" "run: [Errno 20]" '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["SBCLR"],["SBOUT",0],["SBOUT",104],["SBOUT",100],["SBOUT",114],["SBOUT",47],["SBFIND","f"]]]}' "$T/in"
t reject 1 "reject: k" "reject: k" '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["REJECT","k"]]]}'
t accept 0 "" "" '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["ACCEPT"]]]}'
echo "neg  ok $ok   fail $bad"
[ $bad -eq 0 ] && [ $ok -gt 0 ]
