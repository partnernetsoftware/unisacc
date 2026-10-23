#!/bin/sh
# K6-20: unisa.gold lex_label C reference (seed/unisa_gold_lex_label.c) vs unisa/gold.py on the finite domain:
# the full table of the 12 CHARC classes x peek in {slash, star, d, other} (48 rows, byte-equal), plus named
# points for the comment starts, the dot rule, and the class table.  The python side does NOT import gold.py
# (its module-level build() is not called): it extracts CHARC and lex_label from the source with ast and
# executes those two definitions.  Negatives (both sides refuse, stdout empty): an unknown class (python
# KeyError); a missing peek argument.  C double-run.  Point-sample equivalence over the finite domain only;
# not the gold stage closure; not build(); not regen; keeps gold.py and catalog.py.  First red stops.
# usage: tests/unisagoldlexlabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldlex.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisagoldlex: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldlex  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/gl" seed/unisa_gold_lex_label.c || fail "compile red: seed/unisa_gold_lex_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'CHARC', 'lex_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    assert sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes]) == sorted(want), 'lex definitions'
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-lex', 'exec'), ns)
    lex_label, CHARC = ns['lex_label'], ns['CHARC']
    PEEKS = ('slash', 'star', 'd', 'other')
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\n' % (c, p, lex_label(c, p)) for c in CHARC for p in PEEKS)
    elif mode == 'call':
        out = lex_label(sys.argv[2], sys.argv[3]) + '\n'
    else:
        raise ValueError('mode')
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out)
PY

same=0; total=0
pair() {   # pair ARGS... -- C and python stdout byte-equal; C double-run
    total=$((total + 1))
    "$B" 10 "$T/gl" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/gl" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
# named points: the comment starts, the dot rule (digit and non-digit peek), and the class table
for pt in 'slash slash' 'slash star' 'slash other' 'dot d' 'dot other' 'dot slash' 'ws other' 'nl other' 'A d' 'q other' 'sq other' 'punct other' 'eof other' 'other other' 'star slash'; do
    pair call $pt
done
echo "SAME $same of $total points (full 48-row table: 12 classes x 4 peek values; 15 named points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/gl" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg unknown-class call bogus slash
neg missing-peek call dot

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_gold_lex_label.c $(sha256sum seed/unisa_gold_lex_label.c | cut -d' ' -f1)"
echo "unisagoldlexlabelcheck.sh $(sha256sum tests/unisagoldlexlabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldlex  same $same  diff 0  neg 2/2"
exit 0
