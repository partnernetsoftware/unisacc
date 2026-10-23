#!/bin/sh
# K6-23: unisa.gold opinfo_label C reference (seed/unisa_gold_opinfo_label.c) vs unisa/gold.py on the finite op
# domain OPINFO_OPS (tape.SHAPE's 72 ops incl. the fp ops, then "other": 73 rows, full byte-equal table), plus
# named and boundary points ("none" and an unknown name fall through to the default classes).  The python side
# does NOT import gold.py (its module-level build() is not called): it extracts the definitions _tape_ops,
# OPINFO_OPS, OPINFO_SIMPLE, OPINFO_ALU, PEEP_B and opinfo_label with ast; _tape_ops' relative import of tape is
# rewritten to the absolute unisa.tape (same object, read-only).  Negatives (wrapper level, both sides refuse,
# stdout empty): a missing op argument; an unknown mode.  C double-run.  Point-sample equivalence over the
# finite domain only; not the gold stage closure; not build(); keeps gold.py and tape.py.  First red stops.
# usage: tests/unisagoldopinfolabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldopinfo.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisagoldopinfo: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldopinfo  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/go" seed/unisa_gold_opinfo_label.c || fail "compile red: seed/unisa_gold_opinfo_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'_tape_ops', 'OPINFO_OPS', 'OPINFO_SIMPLE', 'OPINFO_ALU', 'PEEP_B', 'opinfo_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'opinfo definitions: %s' % got
    # _tape_ops does `from .tape import SHAPE`; inside the private namespace the same object is `unisa.tape`
    for n in nodes:
        for sub in ast.walk(n):
            if isinstance(sub, ast.ImportFrom) and sub.level == 1 and sub.module == 'tape':
                sub.level = 0
                sub.module = 'unisa.tape'
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-opinfo', 'exec'), ns)
    ol, OPS = ns['opinfo_label'], ns['OPINFO_OPS']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\t%s\n' % (op, ol(op)['simple'], ol(op)['acls'], ol(op)['bcls']) for op in OPS)
    elif mode == 'call':
        if len(sys.argv) != 3:
            raise ValueError('operand count')
        d = ol(sys.argv[2])
        out = '%s %s %s\n' % (d['simple'], d['acls'], d['bcls'])
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
    "$B" 10 "$T/go" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/go" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
set -f   # operator names include * and [] as data; keep them out of globbing
for op in imm load64 store64 mov jump jumpz add64 mul64 sub64 eq slt64 ult64 .ld .lea other fadd64 none bogus; do
    pair call "$op"
done
set +f
echo "SAME $same of $total points (full 73-row OPINFO_OPS table; 18 named and boundary points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/go" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg missing-op call
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "tape.py $(sha256sum unisa/tape.py | cut -d' ' -f1)"
echo "unisa_gold_opinfo_label.c $(sha256sum seed/unisa_gold_opinfo_label.c | cut -d' ' -f1)"
echo "unisagoldopinfolabelcheck.sh $(sha256sum tests/unisagoldopinfolabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldopinfo  same $same  diff 0  neg 2/2"
exit 0
