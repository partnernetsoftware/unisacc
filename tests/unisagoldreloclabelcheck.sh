#!/bin/sh
# K6-27: unisa.gold reloc_label C reference (seed/unisa_gold_reloc_label.c) vs unisa/gold.py on the finite domain
# JMPKIND x ARCH (3 x 2 = 6 keys): the full table, byte-equal, plus named points (each architecture's rule and the
# conditional-jump split on arm64).  The python side does NOT import gold.py (its module-level build() is not
# called): it extracts JMPKIND and reloc_label from gold.py with ast, and ARCH from catalog.py with ast, and executes
# those definitions.  Negatives (wrapper level, both sides refuse, stdout empty): a missing arch operand; an unknown
# mode.  reloc_label has no refusal of its own, so no function-level negative is claimed.  C double-run.  Point-
# sample equivalence over the finite domain only; not the gold stage closure; not build(); keeps gold.py and
# catalog.py.  First red stops.  usage: tests/unisagoldreloclabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldreloc.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisagoldreloc: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldreloc  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/rl" seed/unisa_gold_reloc_label.c || fail "compile red: seed/unisa_gold_reloc_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
def extract(path, names):
    tree = ast.parse(open(path, encoding='utf-8').read())
    out = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in names:
            out.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in names:
            out.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in out])
    assert got == sorted(names), '%s definitions: %s' % (path, got)
    return out
try:
    nodes = extract('unisa/gold.py', {'JMPKIND', 'reloc_label'}) + extract('unisa/catalog.py', {'ARCH'})
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-reloc', 'exec'), ns)
    rl, K, A = ns['reloc_label'], ns['JMPKIND'], ns['ARCH']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\n' % (k, a, rl(k, a)) for k, a in itertools.product(K, A))
    elif mode == 'call':
        if len(sys.argv) != 4:
            raise ValueError('operand count')
        out = rl(sys.argv[2], sys.argv[3]) + '\n'
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
    "$B" 10 "$T/rl" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/rl" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
# named points: x86_64 for every kind, and the arm64 split (jz to arm19, jmp and call to arm26)
for pt in 'jmp x86_64' 'jz x86_64' 'call x86_64' 'jmp arm64' 'jz arm64' 'call arm64'; do
    pair call $pt
done
echo "SAME $same of $total points (full 6-row table JMPKIND x ARCH; 6 named points)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/rl" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg missing-arch call jz
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_gold_reloc_label.c $(sha256sum seed/unisa_gold_reloc_label.c | cut -d' ' -f1)"
echo "unisagoldreloclabelcheck.sh $(sha256sum tests/unisagoldreloclabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldreloc  same $same  diff 0  neg 2/2"
exit 0
