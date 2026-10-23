#!/bin/sh
# K6-18: unisa.gold prec_label C reference (seed/unisa_gold_prec.c) vs unisa/gold.py on the finite domain:
# every PREC_OPS entry (19, incl. "other") as a full table, plus boundary strings (empty, "**", "<<=",
# trailing blank, a near-miss of "||") that must answer "none".  The python side does NOT import gold.py
# (its module-level build() is not called): it extracts only PREC_OPS, PREC_LEV, _PREC and prec_label from
# the source with ast and executes those four definitions.  Outputs compared byte-for-byte; C double-run.
# Negatives (both sides refuse, stdout empty): a missing operator argument; an unknown mode.
# Point-sample equivalence over the finite domain and the listed boundary strings only; not the gold stage
# closure; not build(); not regen of any table; not weights; keeps gold.py and catalog.py.  First red stops.
# usage: tests/unisagoldpreccheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldprec.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisagoldprec: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldprec  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/gp" seed/unisa_gold_prec.c || fail "compile red: seed/unisa_gold_prec.c"

# python side: prec definitions extracted by ast from unisa/gold.py and executed in a private namespace
cat > "$T/pydump.py" <<'PY'
import ast, sys
src_path = 'unisa/gold.py'
mode = sys.argv[1]
try:
    tree = ast.parse(open(src_path, encoding='utf-8').read())
    want = {'PREC_OPS', 'PREC_LEV', '_PREC', 'prec_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    assert sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes]) == sorted(want), 'prec definitions'
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-prec', 'exec'), ns)
    prec_label, PREC_OPS = ns['prec_label'], ns['PREC_OPS']
    if mode == 'table':
        out = ''.join('%s\t%s\n' % (op, prec_label(op)) for op in PREC_OPS)
    elif mode == 'op':
        out = prec_label(sys.argv[2]) + '\n'
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
    "$B" 10 "$T/gp" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/gp" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; cat "$T/c.out"; echo "--- py"; cat "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
for op in '*' '/' '%' '+' '-' '||' '&&' '==' 'other'; do pair op "$op"; done
for op in '' '**' '<<=' '|| ' 'o'; do pair op "$op"; done
echo "SAME $same of $total points (full PREC_OPS table: 19 operators incl. other; 14 named/boundary operator points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/gp" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg missing-operator op
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_gold_prec.c $(sha256sum seed/unisa_gold_prec.c | cut -d' ' -f1)"
echo "unisagoldpreccheck.sh $(sha256sum tests/unisagoldpreccheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldprec  same $same  diff 0  neg 2/2"
exit 0
