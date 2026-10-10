#!/bin/sh
# K5-1: opt δ — seed/gen.c vs exec/build/gen.py byte-identical; negative TSV
# refuse by name; helper prefers seed-gen with no silent Python fallback.
# Does not change exec/opt/check.sh acceptance.  usage: tests/seedoptcheck.sh
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=${TMPDIR:-/tmp}/unisacc-seedopt; mkdir -p "$T"
G=$T/seed-gen
"$B" 55 cc -std=c99 -O2 -w -Iseed -o "$G" seed/gen.c || { echo "seedopt: gen.c does not build"; exit 1; }

same=0
bad=0

run_pair() {
    label=$1; shift
    rm -f "$T/c-$label.json" "$T/py-$label.json"
    if ! "$B" 55 "$G" opt "$T/c-$label.json.$$" "$@" 2>"$T/c-$label.err"; then
        echo "DIFF $label (C rc=$?) $(tail -1 "$T/c-$label.err")"
        bad=$((bad + 1))
        return 0
    fi
    mv "$T/c-$label.json.$$" "$T/c-$label.json"
    if ! "$B" 55 python3 exec/build/gen.py opt "$T/py-$label.json.$$" "$@" 2>"$T/py-$label.err"; then
        echo "DIFF $label (Python rc=$?) $(tail -1 "$T/py-$label.err")"
        bad=$((bad + 1))
        return 0
    fi
    mv "$T/py-$label.json.$$" "$T/py-$label.json"
    if cmp -s "$T/c-$label.json" "$T/py-$label.json"; then
        echo "SAME $label"
        same=$((same + 1))
    else
        echo "DIFF $label"
        bad=$((bad + 1))
    fi
}

run_pair o1
run_pair o2 --o2

# deterministic double-run (C)
"$B" 55 "$G" opt "$T/c-o1-b.json" 2>/dev/null
if cmp -s "$T/c-o1.json" "$T/c-o1-b.json"; then
    echo "SAME o1-det"
    same=$((same + 1))
else
    echo "DIFF o1 nondeterministic"
    bad=$((bad + 1))
fi
"$B" 55 "$G" opt "$T/c-o2-b.json" --o2 2>/dev/null
if cmp -s "$T/c-o2.json" "$T/c-o2-b.json"; then
    echo "SAME o2-det"
    same=$((same + 1))
else
    echo "DIFF o2 nondeterministic"
    bad=$((bad + 1))
fi

# negative: corrupt one TSV data row → both refuse (named); restore always
NEG=exec/opt/rounds-result.tsv
cp "$NEG" "$T/rounds-result.tsv.bak"
python3 - "$NEG" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
lines = p.read_text().splitlines(True)
for i, l in enumerate(lines):
    if l and not l.startswith("#") and "\t" in l:
        parts = l.rstrip("\n").split("\t")
        lines[i] = "\t".join(parts[:-1]) + "\n"
        break
else:
    raise SystemExit("no data row to mutate")
p.write_text("".join(lines))
PY
py_rc=0
c_rc=0
"$B" 55 python3 exec/build/gen.py opt "$T/neg-py.json" 2>"$T/neg-py.err" || py_rc=$?
"$B" 55 "$G" opt "$T/neg-c.json" 2>"$T/neg-c.err" || c_rc=$?
mv "$T/rounds-result.tsv.bak" "$NEG"
case $py_rc in 0) echo "NEG Python did not refuse (rc=$py_rc)"; bad=$((bad + 1));; esac
case $c_rc in 0) echo "NEG C did not refuse (rc=$c_rc)"; bad=$((bad + 1));; esac
if ! grep -q 'expected section plus four columns' "$T/neg-py.err"; then
    echo "NEG Python refuse not named"
    bad=$((bad + 1))
fi
if ! grep -q 'section rule column count' "$T/neg-c.err"; then
    echo "NEG C refuse not named ($(tail -1 "$T/neg-c.err"))"
    bad=$((bad + 1))
fi
if [ -s "$T/neg-py.json" ] || [ -s "$T/neg-c.json" ]; then
    echo "NEG wrote output despite refuse"
    bad=$((bad + 1))
fi
echo "NEG refuse  py_rc=$py_rc c_rc=$c_rc"

# no silent fallback: explicit missing SEED_GEN_BIN must fail closed
rm -f "$T/fb.json"
if SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen-k5-missing \
    sh exec/opt/gen-delta.sh "$T/fb.json" 2>"$T/fb.err"; then
    echo "FALLBACK silently succeeded"
    bad=$((bad + 1))
else
    echo "NO_FALLBACK ok"
fi
[ ! -e "$T/fb.json" ] || { echo "FALLBACK wrote output"; bad=$((bad + 1)); }

# SEED_GEN=0 still reaches Python reference
if SEED_GEN=0 sh exec/opt/gen-delta.sh "$T/py-only.json" 2>/dev/null \
    && cmp -s "$T/py-only.json" "$T/py-o1.json"; then
    echo "SAME py-only"
    same=$((same + 1))
else
    echo "DIFF py-only forced"
    bad=$((bad + 1))
fi

# helper prefer seed-gen matches reference
if SEED_GEN=1 SEED_GEN_BIN="$G" sh exec/opt/gen-delta.sh "$T/h-o1.json" \
    && cmp -s "$T/h-o1.json" "$T/py-o1.json"; then
    echo "SAME helper-o1"
    same=$((same + 1))
else
    echo "DIFF helper-o1"
    bad=$((bad + 1))
fi
if SEED_GEN=1 SEED_GEN_BIN="$G" sh exec/opt/gen-delta.sh "$T/h-o2.json" --o2 \
    && cmp -s "$T/h-o2.json" "$T/py-o2.json"; then
    echo "SAME helper-o2"
    same=$((same + 1))
else
    echo "DIFF helper-o2"
    bad=$((bad + 1))
fi

echo "seedopt  same $same  bad $bad"
[ "$bad" = 0 ] && [ "$same" -gt 0 ]
