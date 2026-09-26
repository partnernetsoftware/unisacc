#!/bin/sh
# exec/c/check.sh DELTA.json FILE... -- the C executor (exec/c/run.c) against
# the Python one (exec/pp/sim.py through exec/parse/compare.py) on E3.
# Each file must get the same verdict:
#   accept -- the C tape equals the reference's tape, which compare.py found
#             equal to sim.py's (so C = Python through the same reference);
#   reject -- the first stderr line's reason equals sim.py's `not covered`
#             reason.  The rest of stderr, and any other diagnostic, is NOT
#             compared.
# Any tool that fails -- cc, tbl.py, compare.py, the token dump, the
# reference -- fails the check.  Every step is bounded (AGENTS.md: 60 s).
# The time printed is the whole loop (dump + reference + cmp included), not
# the executor's own time.
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
D=$1; shift
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "exec/c: cc failed"; exit 1; }
b 60 python3 exec/c/tbl.py "$D" "$T/d.tbl" || { echo "exec/c: tbl.py failed"; exit 1; }
E3V=1 b 60 python3 exec/parse/compare.py "$D" "$@" > "$T/py" 2>&1; prc=$?
# compare.py exits 1 on a DIFF or a tool failure: either is a failure here
[ $prc -eq 0 ] || { echo "exec/c: compare.py exited $prc"; tail -3 "$T/py"; exit 1; }
same=0; diff=0
t0=$(perl -MTime::HiRes=time -e 'print time')
for f in "$@"; do
    UA_TYPESPELL=1 b 10 /tmp/ua_tdump -dump-tokens "$f" > "$T/x" 2>/dev/null; drc=$?
    [ $drc -eq 0 ] || { echo "exec/c: token dump of $f exited $drc"; exit 1; }
    b 10 "$T/run" "$T/d.tbl" "$T/x" "$f" > "$T/o" 2> "$T/e"; rc=$?
    v=$(awk -v f="$f" '$2==f {print $1}' "$T/py")
    ok=0
    case "$rc:$v" in
    0:equal)
        b 10 /tmp/ua_ref "$f" -S -o - > "$T/ref"; rrc=$?
        [ $rrc -eq 0 ] || { echo "exec/c: reference on $f exited $rrc"; exit 1; }
        cmp -s "$T/ref" "$T/o" && ok=1 ;;
    1:not-covered)
        py=$(awk -v f="$f" '$2==f {$1 = ""; $2 = ""; sub(/^ +/, ""); print}' "$T/py")
        c=$(head -1 "$T/e" | sed 's/^reject: //'); [ "$py" = "$c" ] && ok=1 ;;
    esac
    if [ $ok = 1 ]; then same=$((same+1)); else diff=$((diff+1)); echo "  DIFFER $f  c rc=$rc  py=$v  $(head -1 "$T/e")"; fi
done
t1=$(perl -MTime::HiRes=time -e 'print time')
echo "exec/c  files $#   same verdict $same   differ $diff   loop $(perl -e "printf '%.2f', $t1 - $t0") s (dump, reference and cmp included)"
[ $diff -eq 0 ] && [ $same -gt 0 ]
