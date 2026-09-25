#!/bin/bash
# The documents say what the code says. [A-48]
#
# The table of model stages lived in four files, copied by hand.  After the
# 11 -> 14 stage refactor, prd.tree.md and prd.map.md went on describing
# the eleven-stage compiler for a day -- `type 960` where the gold said
# 4,275 -- and nothing noticed, because nothing compared them with
# anything.  The table is generated now (`python3 -m unisa docs`), between
# <!-- stages:begin --> and <!-- stages:end -->, and this fails when a
# marked region is out of date or a marker has gone missing.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
out=$(perl -e 'alarm 60; exec @ARGV' python3 -m unisa docs --check 2>&1); rc=$?
[ -n "$out" ] && echo "$out"
n=$(grep -l "stages:begin" prd.tree.md prd.map.md README.md 2>/dev/null | wc -l | tr -d ' ')
echo
echo "docs  generated tables $n   stale $([ $rc -eq 0 ] && echo 0 || echo yes)"
[ "$rc" -eq 0 ] && [ "$n" -eq 3 ]
