#!/bin/sh
# K6-6: unisa.libneed C reference (seed/unisa_libneed.c) vs unisa/libneed.py on point samples:
# PREFIX / ROOTS / MACRO_NAME_LIMIT, guard(name), bodies(text) on a synthetic source, and table/roots
# on a scratch micro incdir (never the product include/ tree).  Byte-equal observable lines; C double
# run; negatives (duplicate carried body, over-long need name, unbalanced body) refused by both sides
# with empty stdout.  Point-sample equivalence only; not a product closure member; keeps libneed.py.
# First red stops.  usage: tests/unisalibneedcheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisalibneed.XXXXXX") || exit 2
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT
fail() { echo "unisalibneed: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisalibneed  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/libneed" seed/unisa_libneed.c || fail "compile red: seed/unisa_libneed.c"

cat > "$T/pydump.py" <<'PY'
import sys, os
from unisa import libneed as L
cmd = sys.argv[1]
if cmd == 'consts':
    print('PREFIX ' + L.PREFIX)
    print('LIMIT %d' % L.MACRO_NAME_LIMIT)
    print('ROOTS ' + ' '.join(L.ROOTS))
elif cmd == 'guard':
    print(L.guard(sys.argv[2]))
elif cmd == 'bodies':
    text = open(sys.argv[2]).read()
    for name, a, b in L.bodies(text):
        print('BODY %s %d %d' % (name, a, b))
elif cmd == 'table':
    keys, closure, names = L.table(sys.argv[2])
    print('NAMES' + ''.join(' ' + n for n in names))
    for k in keys:
        print('KEY ' + k + ''.join(' ' + x for x in closure[k]))
    rs = sorted({b for r in L.ROOTS for b in closure.get(r, ())})
    print('ROOTS_CLOSURE' + ''.join(' ' + x for x in rs))
PY

# synthetic source for bodies(): comments, string/char literals with braces, nested blocks,
# a non-function static, a non-static function, and a static constructor-like body
cat > "$T/src.txt" <<'SRC'
static int fa(int x) { /* a } comment { */ return x + 1; }
static char *s(void) { const char *q = "}{ not a brace"; char c = '}'; (void)c; return (char *)q; }
static int y = 3;
int plain(void) { return 0; }
static void nest(void) { if (1) { { } } }
static int g(int a,
             int b) { return a * b; }
SRC

# scratch micro incdir: bodies, macros, a subdirectory, a reserved macro name
mkdir -p "$T/inc/sub"
cat > "$T/inc/a.h" <<'H'
#define M(x) fa(x)
#define _UNISA_SKIP fb
static void exit(int c) { fa(c); }
static int fa(int x) { return fb(x) + 1; }
H
cat > "$T/inc/b.h" <<'H'
static int fb(int x) { return fc(x); }
static int fc(int x) { return x; }
#define __UNISA_X (fa)
#define LONEMAC 7
H
cat > "$T/inc/sub/c.h" <<'H'
static int fz(void) { return 0; }
static void _unisa_environ_init(void) { fb(0); }
H

cat > "$T/dup.h" <<'H'
static int fa(void) { return 1; }
H
mkdir -p "$T/dupinc"; cp "$T/dup.h" "$T/dupinc/x.h"; cp "$T/dup.h" "$T/dupinc/y.h"
mkdir -p "$T/longinc"
printf 'static int %s(void) { return 0; }\n' "$(printf 'n%.0s' $(seq 1 60))" > "$T/longinc/z.h"
printf 'static int open_body(void) {\n  return 0;\n' > "$T/unbal.txt"

same=0
run_both() {   # run_both LABEL ARGS...
    lab=$1; shift
    "$B" 10 "$T/libneed" "$@" >"$T/c-$lab.txt" 2>"$T/c-$lab.err"; c=$?
    [ "$c" -eq 0 ] || fail "C refused $lab rc=$c $(cat "$T/c-$lab.err")"
    "$B" 10 "$T/libneed" "$@" >"$T/c2-$lab.txt" 2>/dev/null || fail "C second run rc!=0 $lab"
    cmp -s "$T/c-$lab.txt" "$T/c2-$lab.txt" || fail "C not deterministic $lab"
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/p-$lab.txt" 2>"$T/p-$lab.err" || fail "python refused $lab: $(tail -1 "$T/p-$lab.err")"
    cmp -s "$T/c-$lab.txt" "$T/p-$lab.txt" || { diff -u "$T/p-$lab.txt" "$T/c-$lab.txt" | head -30; fail "DIFF $lab"; }
    same=$((same + 1)); echo "SAME $lab ($(wc -l <"$T/c-$lab.txt" | tr -d ' ') lines)"
}
run_both consts consts
run_both guard-fa guard fa
run_both guard-long guard _unisa_environ_init
run_both bodies bodies "$T/src.txt"
run_both table table "$T/inc"
run_both table-sub table "$T/inc/sub"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME INCDIR-OR-FILE KIND
    nm=$1; kind=$2; arg=$3
    "$B" 10 "$T/libneed" "$kind" "$arg" >"$T/n-$nm-c.out" 2>"$T/n-$nm-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$kind" "$arg" >"$T/n-$nm-p.out" 2>"$T/n-$nm-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-$nm-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-$nm-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-$nm-c.err")"
}
neg dup-body table "$T/dupinc"
neg long-name table "$T/longinc"
neg unbalanced bodies "$T/unbal.txt"

echo "---- identity ----"
echo "HEAD $H"
echo "libneed.py $(sha256sum unisa/libneed.py | cut -d' ' -f1)"
echo "unisa_libneed.c $(sha256sum seed/unisa_libneed.c | cut -d' ' -f1)"
echo "unisalibneedcheck.sh $(sha256sum tests/unisalibneedcheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisalibneed  same $same  diff 0  neg 3/3"
exit 0
