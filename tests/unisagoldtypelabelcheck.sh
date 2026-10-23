#!/bin/sh
# K6-21: unisa.gold type_label C reference (seed/unisa_gold_type_label.c) vs unisa/gold.py on the FULL finite
# domain TYS x TOPS x TYS (15 x 19 x 15 = 4275 triples, byte-equal table), plus named points (integer and
# floating arithmetic, the usual conversions to unsigned, pointer arithmetic, bitwise, shifts, comparisons,
# the address-of rule, the comma operator, and boundary names that fall through to "illegal").  The python
# side does NOT import gold.py (its module-level build() is not called): it extracts TYS, TOPS, NUM, FLT,
# RANK, _promote, _uns, _arith, _farith and type_label with ast and executes those definitions.  Negatives
# (both sides refuse, stdout empty): a wrong operand count (command line); an unknown mode.  C double-run.
# Point-sample equivalence over the finite domain only; not the gold stage closure; not build(); not regen;
# keeps gold.py and catalog.py.  First red stops.  usage: tests/unisagoldtypelabelcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisagoldtype.XXXXXX") || exit 2
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisagoldtype: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisagoldtype  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/gt" seed/unisa_gold_type_label.c || fail "compile red: seed/unisa_gold_type_label.c"

cat > "$T/pydump.py" <<'PY'
import ast, itertools, sys
mode = sys.argv[1]
try:
    tree = ast.parse(open('unisa/gold.py', encoding='utf-8').read())
    want = {'TYS', 'TOPS', 'NUM', 'FLT', 'RANK', '_farith', '_promote', '_uns', '_arith', 'type_label'}
    nodes = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in want:
            nodes.append(n)
        elif isinstance(n, ast.FunctionDef) and n.name in want:
            nodes.append(n)
    got = sorted([n.targets[0].id if isinstance(n, ast.Assign) else n.name for n in nodes])
    assert got == sorted(want), 'type definitions: %s' % got
    ns = {}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), 'gold-type', 'exec'), ns)
    tl, TYS, TOPS = ns['type_label'], ns['TYS'], ns['TOPS']
    if mode == 'table':
        out = ''.join('%s\t%s\t%s\t%s\n' % (a, o, b, tl(a, o, b)) for a, o, b in itertools.product(TYS, TOPS, TYS))
    elif mode == 'call':
        if len(sys.argv) != 5:
            raise ValueError('operand count')
        out = tl(sys.argv[2], sys.argv[3], sys.argv[4]) + '\n'
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
    "$B" 10 "$T/gt" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/gt" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $*"
    [ "$p" -eq 0 ] || fail "python refused $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $*"; }
    same=$((same + 1))
}
pair table
# named points: integer rules, floating rules, the usual conversions, pointer rules, bitwise and shifts,
# comparisons (int result), the address-of rule, comma, and boundary names (fall through to illegal)
set -f   # the operator tokens include * and [], which must not be globbed
for pt in 'i32 + i32' 'u8 + i8' 'u32 + i32' 'i64 + u32' 'u64 + i64' 'f32 + i64' 'f64 + f32' 'i32 < f64' \
          'ptr - ptr' 'ptr + i32' 'i32 + arr' 'ptr * i32' 'i64 & i64' 'i32 & u16' 'i32 << u64' 'u8 << i32' \
          'i8 | i16' 'ptr ^ i8' 'i32 == i32' 'struct . i32' 'fn call i32' 'arr [] i8' 'void = i32' 'i16 , f32' \
          'bogus + i32' 'i32 bogus i32' 'i32 sizeof void'; do
    pair call $pt
done
set +f
echo "SAME $same of $total points (full 4275-row table: TYS x TOPS x TYS; 27 named points incl. boundary names)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/gt" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
neg missing-operand call i32 +
neg unknown-mode build

echo "---- identity ----"
echo "HEAD $H"
echo "gold.py $(sha256sum unisa/gold.py | cut -d' ' -f1)"
echo "catalog.py $(sha256sum unisa/catalog.py | cut -d' ' -f1)"
echo "unisa_gold_type_label.c $(sha256sum seed/unisa_gold_type_label.c | cut -d' ' -f1)"
echo "unisagoldtypelabelcheck.sh $(sha256sum tests/unisagoldtypelabelcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisagoldtype  same $same  diff 0  neg 2/2"
exit 0
