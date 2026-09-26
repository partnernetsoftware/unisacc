#!/bin/sh
# exec/opt/check.sh FILE... -- E4 on the generic C executor, at -O1 and -O2: for
# each file and level, the reference's -O0 tape through that level's E4 table
# must equal the reference's tape at that level.  A failing tool, a stage that does not accept, or a differing tape fails.
# Every step is bounded (AGENTS.md: 60 s).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
UA=${UA:-/tmp/ua_ref}; . "$R/tests/lib.sh"; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "e4: cc failed"; exit 1; }
for L in 1 2; do
    b 60 python3 exec/opt/gen.py "$T/e4_$L.json" $L 2>/dev/null || { echo "e4: gen -O$L failed"; exit 1; }
    b 60 python3 exec/c/tbl.py "$T/e4_$L.json" "$T/e4_$L.tbl" || { echo "e4: tbl -O$L failed"; exit 1; }
done
eq=0; bad=0; skip=0
for f in "$@"; do
    b 30 "$UA" "$f" -S -o - > "$T/o0" 2>/dev/null; r0=$?
    for L in 1 2; do
        b 30 "$UA" -O$L "$f" -S -o - > "$T/oL" 2>/dev/null; rl=$?
        if [ $r0 -ne 0 ] || [ $rl -ne 0 ]; then
            # both refuse the program: an exploration skip -- but with E4STRICT=1 (the gate's fixed
            # sets) every listed file must compile at both levels, so a refusal fails
            if [ -z "${E4STRICT:-}" ] && [ $r0 -eq $rl ] && [ $r0 -lt 128 ]; then skip=$((skip+1)); continue; fi
            bad=$((bad+1)); echo "  BAD $f -O$L  reference -O0 $r0 -O$L $rl"; continue
        fi
        UNISA_MAXSTEPS=400000000000 b 60 "$T/run" "$T/e4_$L.tbl" "$T/o0" > "$T/m" 2> "$T/e"; rc=$?
        if [ $rc -eq 0 ] && cmp -s "$T/m" "$T/oL"; then eq=$((eq+1)); else bad=$((bad+1)); echo "  BAD $f -O$L  e4 rc=$rc $(head -1 "$T/e")"; fi
    done
done
echo "e4 -O1/-O2  files $#   equal $eq   refused by the reference $skip   bad $bad"
[ $bad -eq 0 ] && [ $eq -gt 0 ]
