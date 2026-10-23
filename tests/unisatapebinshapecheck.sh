#!/bin/sh
# K6-11b: unisa.tapebin_shape C reference (seed/unisa_tapebin_shape.c) vs unisa/tapebin_shape.py:
# render (opcode table, 71 rows incl. fp.OPS3/OPS2), render_records, render_c byte-equal against python;
# the same three renderings compared read-only with the repository snapshots (docs/*.tsv, src/*.inc);
# negatives: unknown check kind, stale expectation in a temporary copy, unknown mode, missing file.
# Never writes to the repository: no --write, no edit of docs/ or src/.  C double-run.
# Point-sample equivalence only; not a product closure member; keeps tapebin_shape.py and tape.py.
# First red stops.  usage: tests/unisatapebinshapecheck.sh
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R" || exit 2
B=$R/tests/bound
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-unisatapebinshape.XXXXXX") || exit 2
trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${T:?}"' EXIT
fail() { echo "unisatapebinshape: $*"; exit 1; }
H=$(git rev-parse --verify HEAD) || exit 2
echo "unisatapebinshape  tree $H"

"$B" 30 cc -std=c99 -O2 -w -o "$T/shape" seed/unisa_tapebin_shape.c || fail "compile red: seed/unisa_tapebin_shape.c"

# python side: the module's own renderers, and the same stale comparison main(--check) applies to a path
cat > "$T/pydump.py" <<'PY'
import sys
from unisa import tapebin_shape as m
mode = sys.argv[1]
try:
    if mode == 'render':
        out = m.render()
    elif mode == 'records':
        out = m.render_records()
    elif mode == 'c':
        out = m.render_c()
    elif mode == 'check':
        want = {'render': m.render, 'records': m.render_records, 'c': m.render_c}[sys.argv[2]]()
        got = open(sys.argv[3], 'rb').read()
        if got != want.encode('ascii'):
            raise SystemExit('stale')
        out = 'OK\n'
    else:
        raise ValueError('mode')
except SystemExit as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
except Exception as e:
    sys.stderr.write('refused: %s\n' % e)
    sys.exit(1)
sys.stdout.write(out)
PY

same=0; total=0
pair() {   # pair MODE [ARGS] -- C and python stdout byte-equal; C double-run
    m=$1; shift; total=$((total + 1))
    "$B" 10 "$T/shape" "$m" "$@" >"$T/c.out" 2>"$T/c.err"; c=$?
    "$B" 10 "$T/shape" "$m" "$@" >"$T/c2.out" 2>/dev/null; c2=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$m" "$@" >"$T/p.out" 2>"$T/p.err"; p=$?
    [ "$c" -eq 0 ] || fail "C refused $m $* rc=$c $(cat "$T/c.err")"
    [ "$c2" -eq 0 ] && cmp -s "$T/c.out" "$T/c2.out" || fail "C not deterministic $m $*"
    [ "$p" -eq 0 ] || fail "python refused $m $*: $(tail -1 "$T/p.err")"
    cmp -s "$T/c.out" "$T/p.out" || { echo "--- C"; head -5 "$T/c.out"; echo "--- py"; head -5 "$T/p.out"; fail "DIFF $m $*"; }
    same=$((same + 1))
}
pair render
pair records
pair c

# the repository snapshots, compared read-only (C output against the committed file)
snap() {   # snap MODE REPOFILE
    "$B" 10 "$T/shape" "$1" >"$T/s.out" 2>"$T/s.err" || fail "C refused $1 for snapshot"
    cmp -s "$T/s.out" "$2" || fail "snapshot differs from C render: $2"
    same=$((same + 1)); total=$((total + 1))
}
snap render docs/tapebin-v1.shape.tsv
snap records docs/tapebin-v1.records.tsv
snap c src/tapebin_shape.inc
echo "snapshots: docs/tapebin-v1.shape.tsv docs/tapebin-v1.records.tsv src/tapebin_shape.inc equal to C render (read-only)"

# the module's own read-only --check (no --write anywhere in this script)
PYTHONPATH="$R" "$B" 20 python3 -m unisa.tapebin_shape --check >"$T/chk.out" 2>"$T/chk.err"; chk=$?
[ "$chk" -eq 0 ] || fail "python --check rc=$chk $(tail -1 "$T/chk.err")"
echo "python --check: $(cat "$T/chk.out")"
echo "C check on the repository snapshots:"
"$B" 10 "$T/shape" check render docs/tapebin-v1.shape.tsv || fail "C check render"
"$B" 10 "$T/shape" check records docs/tapebin-v1.records.tsv || fail "C check records"
"$B" 10 "$T/shape" check c src/tapebin_shape.inc || fail "C check c"
echo "SAME $same of $total outputs (render/records/c byte-equal to python; repository snapshots read-only)"

refused() { [ "$1" -ne 0 ] && [ "$1" -ne 124 ] && [ "$1" -lt 128 ]; }
neg() {   # neg NAME ARGS... -- both refuse, empty stdout
    nm=$1; shift
    "$B" 10 "$T/shape" "$@" >"$T/n-c.out" 2>"$T/n-c.err"; c=$?
    PYTHONPATH="$R" "$B" 10 python3 "$T/pydump.py" "$@" >"$T/n-p.out" 2>"$T/n-p.err"; p=$?
    refused "$c" || fail "NEG-$nm C accepted (rc=$c): first red"
    refused "$p" || fail "NEG-$nm python accepted (rc=$p): first red"
    [ ! -s "$T/n-c.out" ] || fail "NEG-$nm C printed stdout: first red"
    [ ! -s "$T/n-p.out" ] || fail "NEG-$nm python printed stdout: first red"
    echo "NEG-$nm both refuse (c_rc=$c py_rc=$p) c_err=$(tail -1 "$T/n-c.err")"
}
# stale expectation: a temporary copy of the snapshot with one opcode's shape changed
python3 -c "import sys; s=open(sys.argv[1]).read(); i=s.index('\\tri\\n'); open(sys.argv[2],'w').write(s[:i]+'\\trr\\n'+s[i+len('\\tri\\n'):])" docs/tapebin-v1.shape.tsv "$T/stale.tsv" || fail "stale fixture"
cmp -s "$T/stale.tsv" docs/tapebin-v1.shape.tsv && fail "fixture did not change the snapshot copy"
neg stale-snapshot check render "$T/stale.tsv"
neg unknown-kind check bogus docs/tapebin-v1.shape.tsv
neg unknown-mode write
neg missing-file check c "$T/does-not-exist"

echo "---- identity ----"
echo "HEAD $H"
echo "tapebin_shape.py $(sha256sum unisa/tapebin_shape.py | cut -d' ' -f1)"
echo "tape.py $(sha256sum unisa/tape.py | cut -d' ' -f1)"
echo "docs/tapebin-v1.shape.tsv $(sha256sum docs/tapebin-v1.shape.tsv | cut -d' ' -f1)"
echo "docs/tapebin-v1.records.tsv $(sha256sum docs/tapebin-v1.records.tsv | cut -d' ' -f1)"
echo "src/tapebin_shape.inc $(sha256sum src/tapebin_shape.inc | cut -d' ' -f1)"
echo "unisa_tapebin_shape.c $(sha256sum seed/unisa_tapebin_shape.c | cut -d' ' -f1)"
echo "unisatapebinshapecheck.sh $(sha256sum tests/unisatapebinshapecheck.sh | cut -d' ' -f1)"
cc --version 2>&1 | head -1 | sed 's/^/host-cc /'
echo "unisatapebinshape  same $same  diff 0  neg 4/4"
exit 0
