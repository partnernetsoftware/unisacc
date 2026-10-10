#!/usr/bin/env python3
"""attemptchain controls (0.0.39 WF5): a deferral keeps its cost after a later success but leaves the
pending set; a START with no end is UNKNOWN; a DONE without START is BLOCKED with no seconds; near-limit
passes are listed per kind/limit; the replay reconciles with results.json."""
import importlib.util, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('attemptchain_under_test', ROOT / 'release/tools/attemptchain.py')
A = importlib.util.module_from_spec(spec); spec.loader.exec_module(A)
log = """START a limit=46 kind=tail left=30
DEFER a rc=142 20.00s
START a limit=46 kind=full left=47
DONE a rc=0 45.50s ok
START b limit=40 kind=full left=47
DEFER b rc=142 40.10s
START c limit=30 kind=full left=47
DONE d rc=1 0.00s predecessor c failed
START e limit=46 kind=full left=47
DONE e rc=1 3.00s wrong 1
""".splitlines()
s = A.summarise(A.replay(log))
assert s['suites'] == 5 and s['final'] == 3, s                      # a, d, e final; b pending; c unknown
assert s['pending'] == ['b'] and s['unknown'] == ['c'], s
assert s['job_seconds']['tail/DEFER'] == 20.0 and s['job_seconds']['full/DONE'] == 48.5, s   # deferral cost kept
assert s['attempts']['blocked/BLOCKED'] == 1 and s['job_seconds']['blocked/BLOCKED'] == 0.0, s
assert 'full/UNKNOWN' not in s['job_seconds'], s                     # no synthesised seconds for UNKNOWN
assert [r['suite'] for r in s['near_limit']] == ['a'] and s['near_limit'][0]['margin'] == 0.5, s
r = A.reconcile(s, {'results': {'a': {'rc': 0}, 'd': {'rc': 1}, 'e': {'rc': 0}, 'b': {'rc': None}}})
assert r['rc_mismatch'] == ['e'] and r['missing_from_replay'] == ['b'], r
print('attemptchain  deferral cost kept after success, pending/unknown/blocked separated, near-limit by kind/limit, results reconciled')
