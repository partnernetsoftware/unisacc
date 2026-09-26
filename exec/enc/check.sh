#!/bin/sh
# exec/enc/check.sh -- E5's first slice: the x86_64 encoder delta on the
# hand-written fixture, against unisa/emit_x86 through exec/enc/ref.py (which
# fails if any line does not encode).  Both executors; any failure fails.
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
F=exec/enc/x86-fixture.txt
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "e5: cc failed"; exit 1; }
b 60 python3 exec/enc/gen.py "$T/d.json" 2>/dev/null || { echo "e5: gen failed"; exit 1; }
b 60 python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl" || { echo "e5: tbl failed"; exit 1; }
b 60 python3 exec/enc/ref.py "$F" > "$T/ref" || { echo "e5: the referee failed"; exit 1; }
[ -s "$T/ref" ] || { echo "e5: empty reference"; exit 1; }
b 60 "$T/run" "$T/d.tbl" "$F" > "$T/c" || { echo "e5: run.c failed"; exit 1; }
b 60 python3 exec/pp/sim.py "$T/d.json" "$F" > "$T/p" || { echo "e5: sim.py failed"; exit 1; }
n=$(grep -c . "$F")
if cmp -s "$T/c" "$T/ref" && cmp -s "$T/p" "$T/ref"; then
    echo "e5 x86 slice  lines $n   bytes $(wc -c < "$T/ref" | tr -d ' ')   equal on both executors"; exit 0
fi
echo "e5 x86 slice  lines $n   DIFFER"; exit 1
