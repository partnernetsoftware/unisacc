#!/bin/bash
# 0.0.37 L2: a speedtest1-shaped SQL workload (bulk insert in a transaction, indexes, range and
# aggregate queries, join, update, delete, vacuum) run by the sqlite shell built by cc (the oracle),
# by the reference ($UA) and, with MODEL_COM, by the product.  Output and exit status must match cc.
#   sqlspeed.sh [ref|com]
# Builds are cached under $TMPDIR keyed by compiler and source sha, so each run is one bounded build
# (~12 s for the reference) plus the workload.
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh
Q=${REALPROG_CACHE:-$R/corpus}/sqlite
[ -f "$Q/sqlite3.c" ] && [ -f "$Q/shell.c" ] || { echo "sqlspeed: corpus/sqlite absent (run tests/realprog.sh once)"; exit 1; }
mode=${1:-ref}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
SRCSHA=$(cat "$Q/sqlite3.c" "$Q/shell.c" | shasum -a 256 | cut -c1-16)
build() {   # build NAME KEY COMMAND... -> $T/NAME, cached by KEY
    local name=$1 key=$2; shift 2
    local C=${TMPDIR:-/tmp}/unisacc-sqlspeed-$name-$key
    if [ ! -x "$C" ]; then
        (cd "$Q" && "$_BOUND" 58 "$@" -o "$C.part") > "$T/$name.err" 2>&1 || { rm -f "$C.part"; echo "sqlspeed: $name build failed: $(head -1 "$T/$name.err")"; return 1; }
        mv -f "$C.part" "$C"
    fi
    cp "$C" "$T/$name"
}
build cc "$( { echo "$SRCSHA"; cc --version 2>&1; } | shasum -a 256 | cut -c1-16)" cc -w -O1 -DSQLITE_OMIT_LOAD_EXTENSION -I. shell.c sqlite3.c || exit 1
case "$mode" in
ref) ua_ready; b=ref; build ref "$( { echo "$SRCSHA"; shasum -a 256 < "$UA"; } | shasum -a 256 | cut -c1-16)" "$UA" -I. shell.c sqlite3.c || exit 1;;
com) [ -n "${MODEL_COM:-}" ] || { echo "sqlspeed: com needs MODEL_COM"; exit 1; }
     b=com; build com "$( { echo "$SRCSHA"; shasum -a 256 < "$MODEL_COM"; } | shasum -a 256 | cut -c1-16)" sh "$MODEL_COM" -I. shell.c sqlite3.c || exit 1;;
*) echo "usage: sqlspeed.sh [ref|com]" >&2; exit 2;;
esac
cat > "$T/work.sql" <<'EOF'
CREATE TABLE t1(a INTEGER PRIMARY KEY, b INTEGER, c TEXT);
CREATE TABLE t2(a INTEGER, b INTEGER, c TEXT);
BEGIN;
WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM n WHERE i<5000)
  INSERT INTO t1 SELECT i, (i*7919)%10007, printf('row %d %x', i, i*31) FROM n;
INSERT INTO t2 SELECT a, b%97, upper(c) FROM t1 WHERE a%3=0;
COMMIT;
CREATE INDEX i1b ON t1(b);
CREATE INDEX i2b ON t2(b);
SELECT count(*), sum(b), min(c), max(c) FROM t1;
SELECT count(*), avg(b), total(a) FROM t1 WHERE b BETWEEN 1000 AND 3000;
SELECT b, count(*) FROM t2 GROUP BY b ORDER BY count(*) DESC, b LIMIT 5;
SELECT count(*), sum(t1.b) FROM t1 JOIN t2 ON t1.a=t2.a WHERE t2.b<10;
SELECT group_concat(a) FROM (SELECT a FROM t1 WHERE c LIKE 'row 12%' ORDER BY a LIMIT 8);
UPDATE t1 SET b=b*2 WHERE a%5=0;
DELETE FROM t1 WHERE a%7=0;
SELECT count(*), sum(b), printf('%.3f', avg(b)) FROM t1;
SELECT substr(hex(randomblob(0)),1,0) || length(group_concat(c,'')) FROM t2;
VACUUM;
PRAGMA integrity_check;
EOF
run() { "$_BOUND" 30 "$T/$1" :memory: < "$T/work.sql" 2>&1; echo "status $?"; }
want=$(run cc); got=$(run "$b")
if [ "$got" = "$want" ]; then echo "sqlspeed  $b  same as cc ($(echo "$want" | wc -l | tr -d ' ') lines)"; exit 0; fi
echo "  FAIL $b"; diff <(echo "$want") <(echo "$got") | head -8
echo "sqlspeed  $b  differs from cc"; exit 1
