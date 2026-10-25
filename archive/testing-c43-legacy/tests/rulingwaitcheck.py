#!/usr/bin/env python3
"""rulingwait controls (0.0.39 WF4 + 0.0.40 idle-cut): waits from the ledger; APPLIED without
applied_at/ref, PENDING with decision/begin, or out-of-order times are malformed (rc 1);
--blockers listing an APPLIED id is STALE_BLOCK (rc 2); unchanged --prev-pending with no
newer applied/begin is IDLE_NO_PROGRESS (rc 3).  Real ledger parses (8-col compatible)."""
import contextlib, importlib.util, io, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('rulingwait_under_test', ROOT / 'release/tools/rulingwait.py')
W = importlib.util.module_from_spec(spec); spec.loader.exec_module(W)

def run(text, extra=None):
    with tempfile.NamedTemporaryFile('w', suffix='.tsv', delete=False) as f: f.write(text); led = f.name
    args = [led, '--now', '2026-01-01T01:00:00+00:00'] + (extra or [])
    out = io.StringIO()
    with contextlib.redirect_stdout(out): rc = W.main(args)
    return rc, out.getvalue()

def tmp_ids(ids):
    with tempfile.NamedTemporaryFile('w', suffix='.txt', delete=False) as f:
        f.write(' '.join(ids) + '\n'); return f.name

T = '2026-01-01T00:00:00+00:00'; D = '2026-01-01T00:30:00+00:00'; A = '2026-01-01T00:40:00+00:00'; B = '2026-01-01T00:50:00+00:00'

# baseline 8-col still works
rc, out = run('X1\t%s\t\t\tPENDING\t\t\ts\nX2\t%s\t%s\t%s\tAPPLIED\towner\tabc\ts\n' % (T, T, D, A))
assert rc == 0 and 'waiting   60.0 min' in out and 'decided after   30.0 min, applied after   10.0 min' in out and 'pending 1 (X1)' in out, out

# 9-col begin chain
rc, out = run('X2\t%s\t%s\t%s\tAPPLIED\towner\tabc\ts\t%s\n' % (T, D, A, B))
assert rc == 0 and 'begun after   10.0 min' in out, out

for bad in ('X\t%s\t\t\tAPPLIED\towner\t\ts\n' % T,                 # APPLIED without anything
            'X\t%s\t%s\t%s\tAPPLIED\towner\t\ts\n' % (T, D, A),     # APPLIED without ref
            'X\t%s\t%s\t\tPENDING\t\t\ts\n' % (T, D),               # PENDING with a decision time
            'X\t%s\t%s\t\tDECIDED\towner\t\ts\n' % (D, T),          # decided before requested
            'X\t%s\tUNKNOWN\t%s\tAPPLIED\towner\tabc\ts\n' % (D, T), # applied before requested
            'X\t%s\t%s\t%s\tAPPLIED\towner\tabc\ts\t%s\n' % (T, D, B, A),  # begin before applied
            'X\t%s\t%s\t\tDECIDED\towner\t\ts\t%s\n' % (T, D, B)):   # begin without APPLIED
    assert run(bad)[0] == 1, bad

for bad in ('X\t%s\t\tPENDING\t\t\ts\n' % T,                       # 7 columns
            'X\t%s\t\t\tPENDING\t\t\ts\nX\t%s\t\t\tPENDING\t\t\ts\n' % (T, T),   # duplicate id
            'X\t2026-01-01T00:00:00\t\t\tPENDING\t\t\ts\n'):          # no timezone
    try: run(bad); raise AssertionError(bad)
    except SystemExit as e: assert 'MALFORMED' in str(e), e

# STALE_BLOCK: F4/R9-style — APPLIED id still listed as current blocker (rc 2)
led = 'R9\t%s\tUNKNOWN\t%s\tAPPLIED\towner\tr9ref\tcarry deferred to 0.0.40\nF4\t%s\t%s\t%s\tAPPLIED\towner\tf4ref\tF4 deferred 0.0.40\nP1\t%s\t\t\tPENDING\t\t\topen\n' % (T, A, T, D, A, T)
rc, out = run(led, ['--blockers', tmp_ids(['R9', 'F4', 'P1'])])
assert rc == 2 and 'STALE_BLOCK' in out and 'R9' in out and 'F4' in out, out
rc, out = run(led, ['--blockers', tmp_ids(['P1'])])
assert rc == 0 and 'blockers ok' in out, out

# IDLE_NO_PROGRESS: same pending, no newer applied/begin (rc 3)
led2 = 'P1\t%s\t\t\tPENDING\t\t\topen\nR9\t%s\tUNKNOWN\t%s\tAPPLIED\towner\tr9ref\tdone\n' % (T, T, A)
# applied A is NOT newer than earliest pending request T? A > T, so motion exists → not idle
rc, out = run(led2, ['--prev-pending', tmp_ids(['P1'])])
assert rc == 0 and 'watch ok' in out, out
# pure pending-only ledger, prev matches → idle
led3 = 'P1\t%s\t\t\tPENDING\t\t\topen\nP2\t%s\t\t\tPENDING\t\t\topen2\n' % (T, T)
rc, out = run(led3, ['--prev-pending', tmp_ids(['P1', 'P2'])])
assert rc == 3 and 'IDLE_NO_PROGRESS' in out, out
# pending moved → ok
rc, out = run(led3, ['--prev-pending', tmp_ids(['P1'])])
assert rc == 0 and 'watch ok' in out, out

assert W.main([str(ROOT / 'release/ruling-requests.tsv')]) == 0
# real ledger + empty blockers file (no ids) ok; claiming R9 blocks must fail
assert W.main([str(ROOT / 'release/ruling-requests.tsv'), '--blockers', tmp_ids([])]) == 0
assert W.main([str(ROOT / 'release/ruling-requests.tsv'), '--blockers', tmp_ids(['R9'])]) == 2

print('rulingwait  waits+begin; malformed APPLIED/PENDING/order/begin; STALE_BLOCK rc2; IDLE_NO_PROGRESS rc3; real ledger parses')
