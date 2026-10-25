#!/bin/bash
_BOUND=$(cd "$(dirname "$0")/.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
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
out=$("$_BOUND" 60 python3 -m unisa docs --check 2>&1); rc=$?
[ -n "$out" ] && echo "$out"
n=$(grep -l "stages:begin" README.md 2>/dev/null | wc -l | tr -d ' ')
echo
echo "docs  generated tables $n   stale $([ $rc -eq 0 ] && echo 0 || echo yes)"
# the referee ledger names every stage, no more, no fewer, and each provider exists
led=$(python3 - <<'PY'
import os
from unisa.gold import ALL
rows = [l.rstrip("\n").split("\t") for l in open("research/referee.tsv") if l.strip() and not l.startswith("#")]
bad = [r for r in rows if len(r) != 4 or r[1] not in ("external", "agreement", "unnamed", "offpath", "measurement")
       or (r[2] != "-" and not os.path.isfile(r[2])) or (r[1] in ("external", "agreement")) != (r[2] != "-")]
st = [r[0] for r in rows]
if bad or sorted(st) != sorted(ALL) or len(st) != len(set(st)):
    print("referee ledger out of step:", bad, sorted(set(ALL) ^ set(st)))
# The counts quoted by the papers must be the ledger's counts (they drifted three ways once).
import re
n = {c: sum(1 for r in rows if r[1] == c) for c in ("external", "agreement", "unnamed", "offpath")}
words = {1: "one", 2: "two", 3: "three", 4: "four", 5: "five", 6: "six", 7: "seven", 8: "eight", 9: "nine", 10: "ten", 11: "eleven", 12: "twelve"}
zh = open("research/unisacc-paper.md", encoding="utf-8").read(); en = open("research/unisacc-paper.en.md", encoding="utf-8").read()
want = [(zh, f"{n['external']} 个阶段有具名的外部裁判"), (zh, f"{n['agreement']} 个阶段只有实现间一致性"), (zh, f"{n['unnamed']} 个阶段尚无直接裁判"),
        (zh, f"{n['external']}/18 个阶段有覆盖范围明确的外部裁判"),
        (en, f"{words[n['external']]} stages have named external referees"), (en, f"{words[n['agreement']]} have only cross-implementation agreement"),
        (en, f"{words[n['unnamed']]} have no direct referee yet"), (en, f"{n['external']}/18 stages have external referees")]
missing = [t for doc, t in want if t not in doc]
if missing: print("paper referee counts differ from the ledger:", missing)
PY
)
[ -n "$led" ] && echo "$led"
echo "docs  referee ledger $([ -z "$led" ] && echo ok || echo STALE)"
model_rc=0
python3 "$R/tests/bound.py" 60 python3 tests/modelbytes.py || model_rc=$?
# A current-state number belongs in a generated region or in a sentence that
# points at the generated ledger -- nowhere else.  The three documents used to
# restate the same two facts with three different values (24/1088 in the
# ledger, 33/1082 in README and prd, 32/938 in ARCHITECTURE) and no check here
# could see it, because docgen owns only the stage table and modelbytes owns
# only the byte ledger: every wrong number sat outside both.
num_rc=0
python3 "$R/tests/bound.py" 60 python3 tests/numberrestatements.py || num_rc=$?
# ... and the assertion must be able to fail.  Both mutations plant a number
# into a real document and require the check to catch it, so this is a
# mutation test and not just a call.
if [ "$num_rc" -eq 0 ]; then
    for m in size stale; do
        python3 "$R/tests/bound.py" 60 python3 tests/numberrestatements.py --mutation "$m" --quiet || {
            echo "docs  number mutation '$m' was not caught" >&2; num_rc=1; }
    done
fi
echo "docs  state numbers $([ "$num_rc" -eq 0 ] && echo ok || echo STALE)"
# README carries a known-limitations table, and it names the private calling
# convention.  The external trial of 0.0.12 reported that the limitations it
# hit were documented nowhere; a table that exists but goes empty, or loses the
# calling-convention row while W-16 still describes one, would put that back.
lim_rc=0
if ! grep -q "^## Known limitations" README.md; then
    echo "README: no '## Known limitations' section" >&2; lim_rc=1
elif ! grep -q "Private calling convention" README.md; then
    echo "README: the limitations table does not name the private calling convention" >&2; lim_rc=1
fi
# ...and the calling convention must not be attributed to W-13 anywhere: W-13
# is "startup initialisation of global pointers", W-16 is the convention.
# A wrong clause number sends a reader to a clause that says something else.
# Both spellings, and anywhere on the line: the README table is in English
# ("calling convention") while prd.md is in Chinese ("调用约定").  Two rounds
# of mutation testing were needed to get here -- the first version matched only
# the Chinese spelling, the second used `[^|]*` between the two, which cannot
# cross the `|` that separates the table's cells, so changing README's
# `prd W-16` to `prd W-13` left the suite green both times.
if grep -rnE "(调用约定|calling convention).*W-13|W-13.*(调用约定|calling convention)" README.md prd.md spec.md ARCHITECTURE.md exec/README.md 2>/dev/null | grep -q .; then
    echo "a document still attributes the calling convention to W-13 (it is W-16)" >&2; lim_rc=1
fi
echo "docs  README limitations $([ "$lim_rc" -eq 0 ] && echo ok || echo MISSING)"
[ "$rc" -eq 0 ] && [ "$n" -eq 1 ] && [ -z "$led" ] && [ "$model_rc" -eq 0 ] && [ "$num_rc" -eq 0 ] && [ "$lim_rc" -eq 0 ]
