#!/bin/sh
# exec/c/check.sh DELTA.json FILE... -- the C executor (exec/c/run.c) against
# the Python one (exec/pp/sim.py through exec/parse/compare.py) on E3: every
# file must get the same verdict -- the same tape, or the same reject reason.
# Each step is bounded (AGENTS.md: 60 s).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
D=$1; shift
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || exit 1
perl -e 'alarm 60; exec @ARGV' python3 exec/c/tbl.py "$D" "$T/d.tbl" || exit 1
E3V=1 perl -e 'alarm 60; exec @ARGV' python3 exec/parse/compare.py "$D" "$@" > "$T/py" 2>&1
same=0; diff=0
t0=$(perl -MTime::HiRes=time -e 'print time')
for f in "$@"; do
    UA_TYPESPELL=1 perl -e 'alarm 10; exec @ARGV' /tmp/ua_tdump -dump-tokens "$f" > "$T/x" 2>/dev/null
    perl -e 'alarm 10; exec @ARGV' "$T/run" "$T/d.tbl" "$T/x" "$f" > "$T/o" 2> "$T/e"; rc=$?
    v=$(awk -v f="$f" '$2==f {print $1}' "$T/py")
    case "$rc:$v" in
    0:equal) perl -e 'alarm 10; exec @ARGV' /tmp/ua_ref "$f" -S -o - | cmp -s - "$T/o" && ok=1 || ok=0 ;;
    1:not-covered) py=$(awk -v f="$f" '$2==f {$1 = ""; $2 = ""; sub(/^ +/, ""); print}' "$T/py")
                   c=$(head -1 "$T/e" | sed 's/^reject: //'); [ "$py" = "$c" ] && ok=1 || ok=0 ;;
    *) ok=0 ;;
    esac
    if [ $ok = 1 ]; then same=$((same+1)); else diff=$((diff+1)); echo "  DIFFER $f  c rc=$rc  py=$v  $(head -1 "$T/e")"; fi
done
t1=$(perl -MTime::HiRes=time -e 'print time')
echo "exec/c  files $#   same verdict $same   differ $diff   C run $(perl -e "printf '%.2f', $t1 - $t0") s"
[ $diff -eq 0 ] && [ $same -gt 0 ]
