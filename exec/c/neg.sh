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
t() {   # t NAME WANT JSON [INCLUDE_DIR]
    printf '%s' "$3" > "$T/d.json"
    b 60 python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl" || { echo "neg: tbl $1 failed"; bad=$((bad+1)); return; }
    b 10 "$T/run" "$T/d.tbl" "$T/in" "$T/in" ${4:-} >/dev/null 2>&1; c=$?
    b 10 python3 exec/pp/sim.py "$T/d.json" "$T/in" >/dev/null 2>&1; p=$?
    if [ $c -eq "$2" ] && [ $p -eq "$2" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL $1: run.c $c, sim.py $p, want $2"; fi
}
t pop-empty 2 '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["POP"]]]}'
# an empty bundled-header name is the include directory itself: opened, not readable
t read-dir 2 '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["SBCLR"],["SBOUT",0],["SBOUT",104],["SBOUT",100],["SBOUT",114],["SBOUT",47],["SBFIND","f"]]]}' "$R/include"
t reject 1 '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["REJECT","k"]]]}'
t accept 0 '{"start":"A","states":{"A":["b",{"120":["A",0]}]},"seqs":[[["ACCEPT"]]]}'
echo "neg  ok $ok   fail $bad"
[ $bad -eq 0 ] && [ $ok -gt 0 ]
