#!/bin/sh
# exec/c/chain.sh FILE... -- source to tape through ONE generic executor
# (exec/c/run.c) and three tables: E2 (preprocess), E1 (lex, typed), E3
# (parse).  An accepted tape must equal the reference's (`ua_ref -S`); a
# stage that rejects is `not covered`; an accepted tape that differs fails.
# Deltas are built from the tree here; every step is bounded (60 s).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
[ $# -gt 0 ] || { echo "no input files"; exit 1; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm shift; exec @ARGV' "$@"; }
b 60 cc -O2 -std=c99 -w -o "$T/run" exec/c/run.c || { echo "chain: cc failed"; exit 1; }
b 60 python3 exec/pp/gen.py "$T/e2.json" >/dev/null 2>&1 || { echo "chain: E2 gen failed"; exit 1; }
b 60 python3 exec/lex/gen.py --typed "$T/e1.json" >/dev/null 2>&1 || { echo "chain: E1 gen failed"; exit 1; }
b 60 python3 exec/parse2/gen2.py "$T/e3.json" >/dev/null 2>&1 || { echo "chain: E3 gen failed"; exit 1; }
for s in e2 e1 e3; do b 60 python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl" || { echo "chain: tbl $s failed"; exit 1; }; done
eq=0; nc=0; bad=0
for f in "$@"; do
    b 10 /tmp/ua_ref "$f" -S -o - > "$T/ref" 2>/dev/null; rr=$?
    b 10 "$T/run" "$T/e2.tbl" "$f" "$f" include > "$T/s1" 2>/dev/null &&
    b 10 "$T/run" "$T/e1.tbl" "$T/s1" > "$T/s2" 2>/dev/null &&
    b 10 "$T/run" "$T/e3.tbl" "$T/s2" "$f" > "$T/s3" 2>/dev/null; rc=$?
    if [ $rc -eq 1 ]; then nc=$((nc+1))
    elif [ $rc -eq 0 ] && [ $rr -eq 0 ] && cmp -s "$T/s3" "$T/ref"; then eq=$((eq+1))
    else bad=$((bad+1)); echo "  BAD $f  chain rc=$rc  reference rc=$rr"; fi
done
echo "chain  files $#   equal $eq   not covered $nc   bad $bad"
[ $bad -eq 0 ] && [ $eq -gt 0 ]
