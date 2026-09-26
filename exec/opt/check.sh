#!/bin/sh
# exec/opt/check.sh FILE... -- E4 (-O1) on the generic C executor: for each file,
# the reference's -O0 tape through the E4 table must equal the reference's -O1
# tape.  A failing tool, a stage that does not accept, or a differing tape fails.
# Every step is bounded (AGENTS.md: 60 s).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
UA=${UA:-/tmp/ua_ref}; . "$R/tests/lib.sh"; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "e4: cc failed"; exit 1; }
b 60 python3 exec/opt/gen.py "$T/e4.json" 2>/dev/null || { echo "e4: gen failed"; exit 1; }
b 60 python3 exec/c/tbl.py "$T/e4.json" "$T/e4.tbl" || { echo "e4: tbl failed"; exit 1; }
eq=0; bad=0; skip=0
for f in "$@"; do
    b 30 "$UA" "$f" -S -o - > "$T/o0" 2>/dev/null; r0=$?
    b 30 "$UA" -O1 "$f" -S -o - > "$T/o1" 2>/dev/null; r1=$?
    if [ $r0 -ne 0 ] || [ $r1 -ne 0 ]; then
        if [ $r0 -eq $r1 ] && [ $r0 -lt 128 ]; then skip=$((skip+1)); continue; fi   # both refuse the program
        bad=$((bad+1)); echo "  BAD $f  reference -O0 $r0 -O1 $r1"; continue
    fi
    UNISA_MAXSTEPS=4000000000 b 30 "$T/run" "$T/e4.tbl" "$T/o0" > "$T/m" 2> "$T/e"; rc=$?
    if [ $rc -eq 0 ] && cmp -s "$T/m" "$T/o1"; then eq=$((eq+1)); else bad=$((bad+1)); echo "  BAD $f  e4 rc=$rc $(head -1 "$T/e")"; fi
done
echo "e4 -O1  files $#   equal $eq   refused by the reference $skip   bad $bad"
[ $bad -eq 0 ] && [ $eq -gt 0 ]
