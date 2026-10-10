#!/usr/bin/env python3
"""rulingwait controls (0.0.39 WF4): waits are computed from the ledger; APPLIED without applied_at/ref,
PENDING with a decision time, or out-of-order times are malformed (rc 1); the real ledger parses."""
import contextlib, importlib.util, io, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('rulingwait_under_test', ROOT / 'release/tools/rulingwait.py')
W = importlib.util.module_from_spec(spec); spec.loader.exec_module(W)
def run(text):
    with tempfile.NamedTemporaryFile('w', suffix='.tsv', delete=False) as f: f.write(text)
    out = io.StringIO()
    with contextlib.redirect_stdout(out): rc = W.main([f.name, '--now', '2026-01-01T01:00:00+00:00'])
    return rc, out.getvalue()
T = '2026-01-01T00:00:00+00:00'; D = '2026-01-01T00:30:00+00:00'; A = '2026-01-01T00:40:00+00:00'
rc, out = run('X1\t%s\t\t\tPENDING\t\t\ts\nX2\t%s\t%s\t%s\tAPPLIED\towner\tabc\ts\n' % (T, T, D, A))
assert rc == 0 and 'waiting   60.0 min' in out and 'decided after   30.0 min, applied after   10.0 min' in out and 'pending 1 (X1)' in out, out
for bad in ('X\t%s\t\t\tAPPLIED\towner\t\ts\n' % T,                 # APPLIED without anything
            'X\t%s\t%s\t%s\tAPPLIED\towner\t\ts\n' % (T, D, A),     # APPLIED without ref
            'X\t%s\t%s\t\tPENDING\t\t\ts\n' % (T, D),               # PENDING with a decision time
            'X\t%s\t%s\t\tDECIDED\towner\t\ts\n' % (D, T)):         # decided before requested
    assert run(bad)[0] == 1, bad
assert W.main([str(ROOT / 'release/ruling-requests.tsv')]) == 0
print('rulingwait  waits from the ledger, malformed APPLIED/PENDING/order rejected, real ledger parses')
