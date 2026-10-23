#!/bin/sh
# K6-22: unisa.gold binsel_label C reference (seed/unisa_gold_binsel_label.c) vs unisa/gold.py on the stage's
# own finite domain: BINSEL_OPS x ("s", "u") = 16 x 2 = 32 keys (full table, byte-equal), plus named and
# boundary points (the unsigned differences, operators with no unsigned difference, a sign outside the stage's
# vocabulary that takes the signed branch).  The python side does NOT import gold.py (its module-level build()
# is not called): it extracts BINSEL_OPS, _BIN_S, _BIN_U and binsel_label with ast and executes those four
# definitions.  Negatives (both sides refuse, stdout empty): an operator outside _BIN_S (python KeyError; a
# function-level refusal); a missing sign argument (command line); an unknown mode.  C double-run.  Point-sample
# equivalence over the stage domain only; not the gold stage closure; not build(); not regen; keeps gold.py and
# catalog.py.  First red stops.  usage: tests/unisagoldbinsellabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldbinsel.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisagoldbinsel: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldbinsel  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/gb" seed/unisa_gold_binsel_label.c || fail "compile red: seed/unisa_gold_binsel_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'BINSEL_OPS', '_BIN_S', '_BIN_U', 'binsel_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'binsel definitions: %s' % got
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-binsel', 'exec'), ns)
    bl, OPS = ns['binsel_label'], ns['BINSEL_OPS']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\n' % (op, s, bl(op, s)) for op, s in itertools.product(OPS, ('s', 'u')))
    elif mode == 'call':
        if len(sys.argv) != 4:
            raise ValueError('operand count')
        out = bl(sys.argv[2], sys.argv[3]) + '\n'
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
    "$B" 10 "$T/gb" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/gb" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
# named points: the unsigned differences, operators with no unsigned difference, and a sign outside the stage vocabulary
set -f   # the operator tokens include * and %, which must not be globbed
for pt in '/ u' '/ s' '% u' '% s' '< u' '< s' '> u' '>= u' '<= u' '>> u' '>> s' '+ u' '* u' '& u' '<< u' \
          '/ x' '% x' '== s' '!= u'; do
    pair call $pt
done
set +f
echo "SAME $same of $total points (full 32-row table: BINSEL_OPS x (s,u); 19 named and boundary points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/gb" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
set -f
neg operator-outside-bin-s call '&&' s
set +f
neg missing-sign call / 
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_gold_binsel_label.c $(sha256sum seed/unisa_gold_binsel_label.c | cut -d' ' -f1)"
echo "unisagoldbinsellabelcheck.sh $(sha256sum tests/unisagoldbinsellabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldbinsel  same $same  diff 0  neg 3/3"
exit 0
