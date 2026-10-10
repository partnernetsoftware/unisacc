#!/bin/sh
# K5-1: opt δ — seed/gen.c vs exec/build/gen.py byte-identical; a corrupted TSV is refused by name by both;
# exec/opt/gen-delta.sh prefers seed-gen with no silent Python fallback.  Does not change exec/opt/check.sh.
# 0.0.40 (机房主任 23:41): everything runs in a private scratch tree (git archive of HEAD), never the checkout:
# the negative mutates the scratch copy only, an EXIT/INT/TERM/HUP trap removes the scratch, and the checkout's
# rounds-result.tsv is hashed before and after (conserved).  usage: tests/seedoptcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
S=$(mktemp -d "${TMPDIR:-/tmp}/seedopt.XXXXXX") || exit 2
trap 'rm -rf "$S"' EXIT
trap 'exit 130' INT TERM HUP
NEG=exec/opt/rounds-result.tsv
before=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
W=$S/tree; mkdir "$W"
git archive HEAD exec seed unisa tests/bound tests/bound.c | tar -x -C "$W" || { echo "seedopt: scratch tree failed"; exit 2; }
cd "$W" || exit 2
B=$W/tests/bound
G=$S/seed-gen
"$B" 55 cc -std=c99 -O2 -w -Iseed -o "$G" seed/gen.c || { echo "seedopt: gen.c does not build"; exit 1; }
same=0; bad=0
ok() { echo "SAME $1"; same=$((same + 1)); }
no() { echo "DIFF $1"; bad=$((bad + 1)); }
# a refusal is a normal non-zero exit: not 0, not a timeout (124/142) and not a signal (>= 128)
refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }

run_pair() {   # run_pair LABEL [--o2]
    label=$1; shift
    rm -f "$S/c-$label.json" "$S/py-$label.json"
    "$B" 55 "$G" opt "$S/c-$label.json" "$@" 2>"$S/c-$label.err"; c=$?
    [ "$c" -eq 0 ] || { no "$label (C rc=$c) $(tail -1 "$S/c-$label.err")"; return 0; }
    "$B" 55 python3 exec/build/gen.py opt "$S/py-$label.json" "$@" 2>"$S/py-$label.err"; p=$?
    [ "$p" -eq 0 ] || { no "$label (Python rc=$p) $(tail -1 "$S/py-$label.err")"; return 0; }
    if cmp -s "$S/c-$label.json" "$S/py-$label.json"; then ok "$label"; else no "$label"; fi
}
run_pair o1
run_pair o2 --o2

# deterministic double-run (C)
for l in o1 o2; do
    rm -f "$S/c-$l-b.json"
    if [ "$l" = o2 ]; then "$B" 55 "$G" opt "$S/c-$l-b.json" --o2 2>"$S/c-$l-b.err"; else "$B" 55 "$G" opt "$S/c-$l-b.json" 2>"$S/c-$l-b.err"; fi
    r=$?
    if [ "$r" -ne 0 ]; then no "$l-det (second run rc=$r)"
    elif cmp -s "$S/c-$l.json" "$S/c-$l-b.json"; then ok "$l-det"; else no "$l nondeterministic"; fi
done

# negative: drop one column of one data row in the SCRATCH copy -> both refuse by name, write nothing
python3 - "$NEG" <<'PY' || { echo "seedopt: negative mutation failed"; exit 2; }
import sys
from pathlib import Path
p = Path(sys.argv[1])
lines = p.read_text().splitlines(True)
for i, l in enumerate(lines):
    if l and not l.startswith("#") and "\t" in l:
        lines[i] = "\t".join(l.rstrip("\n").split("\t")[:-1]) + "\n"
        break
else:
    raise SystemExit("no data row to mutate")
p.write_text("".join(lines))
PY
rm -f "$S/neg-py.json" "$S/neg-c.json"
"$B" 55 python3 exec/build/gen.py opt "$S/neg-py.json" 2>"$S/neg-py.err"; py_rc=$?
"$B" 55 "$G" opt "$S/neg-c.json" 2>"$S/neg-c.err"; c_rc=$?
refused "$py_rc" || no "NEG Python did not refuse normally (rc=$py_rc)"
refused "$c_rc" || no "NEG C did not refuse normally (rc=$c_rc)"
grep -q 'expected section plus four columns' "$S/neg-py.err" || no "NEG Python refuse not named"
grep -q 'section rule column count' "$S/neg-c.err" || no "NEG C refuse not named ($(tail -1 "$S/neg-c.err"))"
[ ! -e "$S/neg-py.json" ] && [ ! -e "$S/neg-c.json" ] || no "NEG wrote output despite refuse"
echo "NEG refuse  py_rc=$py_rc c_rc=$c_rc"
(cd "$R" && git archive HEAD "$NEG") | tar -x -C "$W" || exit 2   # restore the scratch copy for the helper checks

# gen-delta.sh (the scratch copy, so its R is the scratch tree): no silent fallback, 0/1 only, no extra flags
D=$W/exec/opt/gen-delta.sh
gd() { out=$1; shift; rm -f "$out"; env "$@" sh "$D" "$out" 2>"$S/gd.err"; }
gd "$S/fb.json" SEED_GEN=1 SEED_GEN_BIN=/nonexistent/seed-gen-k5-missing; r=$?
{ [ "$r" -ne 0 ] && [ ! -e "$S/fb.json" ]; } && ok "no-fallback missing BIN" || no "missing BIN fell back (rc=$r)"
printf '#!/bin/sh\necho seed-gen-red >&2\nexit 3\n' > "$S/red-gen"; chmod +x "$S/red-gen"
gd "$S/fr.json" SEED_GEN=1 SEED_GEN_BIN="$S/red-gen"; r=$?
{ [ "$r" -eq 3 ] && [ ! -e "$S/fr.json" ] && grep -q seed-gen-red "$S/gd.err"; } && ok "no-fallback failing BIN (rc 3 kept)" || no "failing BIN fell back or lost rc (rc=$r)"
for v in 2 yes ''; do
    gd "$S/sv.json" SEED_GEN="$v"; r=$?
    { [ "$r" -eq 2 ] && [ ! -e "$S/sv.json" ]; } && ok "SEED_GEN='$v' refused" || no "SEED_GEN='$v' not refused (rc=$r)"
done
rm -f "$S/xf.json"; env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/xf.json" --o2 --extra 2>"$S/gd.err"; r=$?
{ [ "$r" -eq 2 ] && [ ! -e "$S/xf.json" ]; } && ok "extra flag refused" || no "extra flag not refused (rc=$r)"
gd "$S/py-only.json" SEED_GEN=0; r=$?
{ [ "$r" -eq 0 ] && cmp -s "$S/py-only.json" "$S/py-o1.json"; } && ok "SEED_GEN=0 reaches Python" || no "SEED_GEN=0 (rc=$r)"
for l in o1 o2; do
    rm -f "$S/h-$l.json"
    if [ "$l" = o2 ]; then env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/h-$l.json" --o2; else env SEED_GEN=1 SEED_GEN_BIN="$G" sh "$D" "$S/h-$l.json"; fi
    r=$?; { [ "$r" -eq 0 ] && cmp -s "$S/h-$l.json" "$S/py-$l.json"; } && ok "helper-$l" || no "helper-$l (rc=$r)"
done
# the default route (no SEED_GEN_BIN) builds its own keyed seed-gen into a private directory and matches too
rm -f "$S/dflt.json"; env SEED_GEN=1 SEED_GEN_DIR="$S/seedbin" sh "$D" "$S/dflt.json"; r=$?
{ [ "$r" -eq 0 ] && cmp -s "$S/dflt.json" "$S/py-o1.json"; } && ok "default-route o1" || no "default-route o1 (rc=$r)"

cd "$R" || exit 2
after=$(python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$NEG") || exit 2
[ "$before" = "$after" ] && ok "checkout $NEG untouched" || no "checkout $NEG changed"
echo "seedopt  same $same  bad $bad"
[ "$bad" = 0 ] && [ "$same" -gt 0 ]
