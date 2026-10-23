#!/bin/sh
# K6-24: unisa.gold peep_label C reference (seed/unisa_gold_peep_label.c) vs unisa/gold.py on the finite domain
# PEEP_A x PEEP_B x PEEP_REL (8 x 17 x 12 = 1632 keys): the full table, byte-equal, plus named points that hit each
# rule and the fall-through of the store64 block.  The python side does NOT import gold.py (its module-level
# build() is not called): it extracts PEEP_A, PEEP_B, PEEP_REL and peep_label with ast and executes those four.
# Negatives (wrapper level, both sides refuse, stdout empty): a missing operand; an unknown mode.  peep_label has
# no refusal of its own (a value outside the tuples matches no rule and answers keep), so no function-level
# negative is claimed.  C double-run.  Point-sample equivalence over the finite domain only; not the gold stage
# closure; not build(); keeps gold.py.  First red stops.  usage: tests/unisagoldpeeplabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldpeep.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisagoldpeep: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldpeep  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/pl" seed/unisa_gold_peep_label.c || fail "compile red: seed/unisa_gold_peep_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'PEEP_A', 'PEEP_B', 'PEEP_REL', 'peep_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'peep definitions: %s' % got
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-peep', 'exec'), ns)
    pl, A, B, REL = ns['peep_label'], ns['PEEP_A'], ns['PEEP_B'], ns['PEEP_REL']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\t%s\n' % (a, b, r, pl(a, b, r)) for a, b, r in itertools.product(A, B, REL))
    elif mode == 'call':
        if len(sys.argv) != 5:
            raise ValueError('operand count')
        out = pl(sys.argv[2], sys.argv[3], sys.argv[4]) + '\n'
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
# named points: each rule, the store64 fall-through (a block that does not match continues), and the default keep
set -f   # the operand names are data; keep them out of globbing
for pt in 'store64 load64 same_slot' 'store64 load64 same_slot_same_reg' 'store64 store64 same_slot' \
          'store64 mov same_slot' 'imm add64 const0' 'imm lshr64 const0' 'imm mul64 const1' 'imm mov copy_dead' \
          'jump jump to_next' 'jumpz jump to_jump' 'jump jump to_jump' 'alu none a_dead' 'other mov dest_to_mov' \
          'mov none copy_into' 'mov store64 copy_into' 'load64 none keep_probe' 'alu add64 pow2' 'store64 load64 none'; do
    pair call $pt
done
set +f
echo "SAME $same of $total points (full 1632-row table: PEEP_A x PEEP_B x PEEP_REL; 18 named points incl. fall-through and keep)"

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
neg missing-operand call store64 load64
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "unisa_gold_peep_label.c $(sha256sum seed/unisa_gold_peep_label.c | cut -d' ' -f1)"
echo "unisagoldpeeplabelcheck.sh $(sha256sum tests/unisagoldpeeplabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldpeep  same $same  diff 0  neg 2/2"
exit 0
