#!/bin/bash
# commitgate.sh -m MSG [--suite NAME]... -- PATH... (0.0.39): the recommended commit path.  Runs the named gate
# suites (default: the light infra set) through checkrun.sh, so the decision is the gate's own exit status, and
# commits the given paths only when it is 0.  A red gate -- whatever its log tail says -- leaves no commit.
# COMMITGATE_GATE replaces tests/gate.sh only together with COMMITGATE_SELFTEST=1 (tests/commitgatecheck.sh).
set -u
R=$(cd "$(dirname "$0")/../.." && pwd)
msg=; suites=()
while [ $# -gt 0 ]; do
  case $1 in
    -m) msg=${2:?message}; shift 2;;
    --suite) suites+=(--suite "${2:?suite}"); shift 2;;
    --) shift; break;;
    *) echo "commitgate: unknown argument $1" >&2; exit 2;;
  esac
done
[ -n "$msg" ] && [ $# -gt 0 ] || { echo "commitgate: need -m MSG and -- PATH..." >&2; exit 2; }
# default set: infrastructure only (机房主任 ruling B).  A product change (freezecheck.product closure, plus unisacc.c) must name at least one
# suite outside the contract layer, or nothing is checked for it and nothing is committed.
# Resolve scoped directories/pathspecs against tracked and untracked paths, using
# the same product predicate as the freeze closure. Keep standalone unisacc.c.
# 0.0.40-prep (机房主任 18:28): every shipping path in the commit must be declared as an input of at least
# one NAMED suite (tests/gatedeps.json files/trees, or its family's); "any non-contract suite" is not enough.
# Unknown or unclassified suites fail closed.
named=$(printf '%s\n' ${suites[@]+"${suites[@]}"} | grep -v '^--suite$' || true)
python3 - "$R" "$named" "$@" <<'PY_PRODUCT' || exit 2
import json, subprocess, sys
R, named, args = sys.argv[1], sys.argv[2].split(), sys.argv[3:]
sys.path.insert(0, R + '/tests')
from freezecheck import product
import gatelayers
paths = set()
for arg in args:
    p = arg[2:] if arg.startswith('./') else arg
    paths.add(p)
    paths.update(subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard','--',p]).decode().split('\0'))
ship = sorted(p for p in paths if p and (p == 'unisacc.c' or product(p)))
if not ship: sys.exit(0)
def no(m): print('commitgate: ' + m, file=sys.stderr); sys.exit(1)
if not named: no('product closure changes need --suite naming a product (non-contract) suite that declares them')
d = json.load(open(R + '/tests/gatedeps.json'))
specs = []
for n in named:
    try: layer = gatelayers.layer(n)
    except ValueError: no('suite %s is not classified by tests/gatelayers.py (fail closed)' % n)
    if n not in d['suites']: no('suite %s has no declared inputs in tests/gatedeps.json (fail closed)' % n)
    if layer != 'contract': specs.append(d['suites'][n])
if not specs: no('product closure changes need --suite naming at least one product (non-contract) suite')
def covers(s, p):
    for src in (s, d['families'].get(s.get('family'), {})):
        if p in (src.get('files') or []): return True
        for k in ('trees', 'code_trees', 'all_files_trees', 'reviewed_trees'):
            if any(p == t or p.startswith(t.rstrip('/') + '/') for t in (src.get(k) or [])): return True
    return False
miss = [p for p in ship if not any(covers(s, p) for s in specs)]
if miss: no('no named product suite declares these shipping paths: ' + ' '.join(miss[:20]) + (' ...' if len(miss) > 20 else ''))
PY_PRODUCT
[ ${#suites[@]} -gt 0 ] || suites=(--suite gate-layers --suite script-inventory --suite checkrun)
gate=$R/tests/gate.sh
if [ -n "${COMMITGATE_GATE:-}" ]; then
  [ "${COMMITGATE_SELFTEST:-}" = 1 ] || { echo "commitgate: COMMITGATE_GATE is for the self-test only" >&2; exit 2; }
  gate=$COMMITGATE_GATE
fi
# the gate checks this repository, so the commit must land in it too (a scratch repo only under the self-test)
top=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "commitgate: not in a git repository" >&2; exit 2; }
[ "$top" = "$R" ] || [ "${COMMITGATE_SELFTEST:-}" = 1 ] || { echo "commitgate: cwd repository $top is not the checked repository $R" >&2; exit 2; }
log=$(mktemp "${TMPDIR:-/tmp}/commitgate.XXXXXX")
# 0.0.40-prep (机房主任 18:28): the committed inputs must be the checked inputs -- index entry (mode/blob/stage),
# working-tree bytes and mode, and membership of every given path are snapshotted before and after the gate.
# Boundary (named): only the COMMITTED pathspecs are bound; a suite's other inputs, its gatedeps declaration and
# its tools are not snapshotted here.  Coverage is declaration-level (gatedeps), not proof of assertion
# semantics or of a valid reviewed stamp.
snap() { python3 - "$@" <<'PY_SNAP'
import hashlib, os, subprocess, sys
h = hashlib.sha256()
h.update(subprocess.check_output(['git','ls-files','-s','-z','--'] + sys.argv[1:]))
for p in sorted(set(subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard','--'] + sys.argv[1:]).decode().split('\0')) - {''}):
    h.update(p.encode() + b'\0')
    try:
        st = os.lstat(p); h.update(oct(st.st_mode).encode())
        h.update(os.readlink(p).encode() if os.path.islink(p) else open(p, 'rb').read())
    except FileNotFoundError: h.update(b'<absent>')
print(h.hexdigest())
PY_SNAP
}
before=$(snap "$@") || { echo "commitgate: cannot snapshot inputs" >&2; exit 2; }
"$R/release/tools/checkrun.sh" "$log" -- "$gate" ${suites[@]+"${suites[@]}"} || { rc=$?; echo "commitgate: gate rc=$rc, nothing committed (log $log)" >&2; exit 1; }
after=$(snap "$@") || { echo "commitgate: cannot snapshot inputs" >&2; exit 2; }
[ "$before" = "$after" ] || { echo "commitgate: inputs changed while the gate ran ($before -> $after), nothing committed (log $log)" >&2; exit 4; }
UNISACC_COMMITGATE_RUN=1 git commit -q -m "$msg" -- "$@" || { echo "commitgate: gate was green but git commit failed" >&2; exit 3; }
echo "commitgate: committed $(git rev-parse --short HEAD) after gate rc=0 (log $log)"
