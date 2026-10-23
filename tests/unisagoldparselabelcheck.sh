#!/bin/sh
# K6-29: unisa.gold parse_label C reference (seed/unisa_gold_parse_label.c) vs unisa/gold.py on the parse stage's own
# domain NT x TOKS (5 x 68 = 340 keys, full enumeration in gold.py's order; this is the stage domain, not the whole token
# universe of any front end): the full table, byte-equal, plus named and boundary points (each nonterminal's rules, the
# SAME_NAME keywords, the unary map, the PARSE_DEFAULT fall-through, the union alias).  The python side does NOT import
# gold.py (its module-level build() is not called): it extracts NT, TOKS, PRODS, SAME_NAME, PARSE_DEFAULT and parse_label
# with ast and executes those six definitions.  The script also asserts that every label of the full table is in PRODS.
# Negatives: an unknown nonterminal is a function-level refusal (python KeyError on PARSE_DEFAULT); a missing token operand
# and an unknown mode are wrapper-level.  C double-run.  Point-sample equivalence over the stage domain only; not the
# gold stage closure; not the front-end parse; keeps gold.py.  First red stops.  usage: tests/unisagoldparselabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldparse.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisagoldparse: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldparse  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/pl" seed/unisa_gold_parse_label.c || fail "compile red: seed/unisa_gold_parse_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'NT', 'TOKS', 'PRODS', 'SAME_NAME', 'PARSE_DEFAULT', 'parse_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'parse definitions: %s' % got
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-parse', 'exec'), ns)
    pl, NT, TOKS, PRODS = ns['parse_label'], ns['NT'], ns['TOKS'], ns['PRODS']
    if mode == 'table':
        rows = [(n, t, pl(n, t)) for n, t in itertools.product(NT, TOKS)]
        assert all(r[2] in PRODS for r in rows), 'a label outside PRODS'
        out = ''.join('%s\t%s\t%s\n' % r for r in rows)
    elif mode == 'call':
        if len(sys.argv) != 4:
            raise ValueError('operand count')
        out = pl(sys.argv[2], sys.argv[3]) + '\n'
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
    "$B" 10 "$T/pl" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/pl" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
set -f   # the token names include * ( ) [ ] and friends as data; keep them out of globbing
for pt in 'top eof' 'top struct' 'top union' 'top typedef' 'top id' 'after_name (' 'after_name {' 'after_name ;' \
          'stmt type' 'stmt union' 'stmt {' 'stmt if' 'stmt goto' 'stmt num' 'unary - ' 'unary sizeof' 'unary ~' \
          'postfix [' 'postfix ++' 'postfix .' 'postfix )' 'unary nosuchtok'; do
    pair call $pt
done
set +f
echo "SAME $same of $total points (full 340-row stage domain NT x TOKS; 22 named and boundary points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/pl" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg unknown-nonterminal call bogus eof
neg missing-token call top
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "unisa_gold_parse_label.c $(sha256sum seed/unisa_gold_parse_label.c | cut -d' ' -f1)"
echo "unisagoldparselabelcheck.sh $(sha256sum tests/unisagoldparselabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldparse  same $same  diff 0  neg 3/3"
exit 0
