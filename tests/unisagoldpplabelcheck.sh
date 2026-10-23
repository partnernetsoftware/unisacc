#!/bin/sh
# K6-26: unisa.gold pp_label C reference (seed/unisa_gold_pp_label.c) vs unisa/gold.py on the finite domain
# DIRS x DEFINED (9 x 2 = 18 keys): the full table, byte-equal, plus named and boundary points (each directive's
# rule, and a DEFINED value other than "1" that takes the off branch).  The python side does NOT import gold.py
# (its module-level build() is not called): it extracts DIRS, DEFINED and pp_label with ast and executes those.
# Negatives (wrapper level, both sides refuse, stdout empty): a missing defined operand; an unknown mode.
# pp_label has no refusal of its own, so no function-level negative is claimed.  C double-run.  Point-sample
# equivalence over the finite domain only; not the gold stage closure; not build(); keeps gold.py.  First red
# stops.  usage: tests/unisagoldpplabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldpp.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisagoldpp: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldpp  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/pp" seed/unisa_gold_pp_label.c || fail "compile red: seed/unisa_gold_pp_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'DIRS', 'DEFINED', 'pp_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'pp definitions: %s' % got
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-pp', 'exec'), ns)
    pl, DIRS, DEFINED = ns['pp_label'], ns['DIRS'], ns['DEFINED']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\n' % (d, f, pl(d, f)) for d, f in itertools.product(DIRS, DEFINED))
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
    "$B" 10 "$T/pp" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/pp" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
# named points: each directive's rule on both defined values, and a defined value outside DEFINED (off branch)
for pt in 'ifdef 1' 'ifdef 0' 'ifndef 1' 'ifndef 0' 'if 1' 'if 0' 'elif 1' 'elif 0' 'else 1' 'else 0' \
          'endif 0' 'define 1' 'undef 0' 'include 1' 'include 0' 'ifdef x' 'bogus 1' 'ifndef x'; do
    pair call $pt
done
echo "SAME $same of $total points (full 18-row table DIRS x DEFINED; 18 named and boundary points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/pp" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg missing-defined call ifdef
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "unisa_gold_pp_label.c $(sha256sum seed/unisa_gold_pp_label.c | cut -d' ' -f1)"
echo "unisagoldpplabelcheck.sh $(sha256sum tests/unisagoldpplabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldpp  same $same  diff 0  neg 2/2"
exit 0
