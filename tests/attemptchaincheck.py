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
DONE f rc=0 12.00s ran but its START was lost
DONE u rc=1 0.00s predecessor p1 p2 failed
DONE v rc=1 0.00s predecessor-ish text
START g limit=20
DONE g rc=0 5.00s old log without kind
START h limit=30 kind=full left=47
START h limit=30 kind=solo left=47
DONE h rc=0 9.00s
""".splitlines()
s = A.summarise(A.replay(log))
assert s['suites'] == 10 and s['final'] == 8, s                     # a, d, e, f, g, h, u, v final; b pending; c unknown
assert s['pending'] == ['b'] and s['unknown'] == ['c'], s
assert s['job_seconds']['tail/DEFER'] == 20.0 and s['job_seconds']['full/DONE'] == 48.5, s   # deferral cost kept
assert s['attempts']['blocked/BLOCKED'] == 2 and s['job_seconds']['blocked/BLOCKED'] == 0.0, s
assert s['attempts']['unstarted/UNSTARTED-DONE'] == 2 and s['job_seconds']['unstarted/UNSTARTED-DONE'] == 12.0, s   # not a zero-second BLOCKED
assert s['attempts']['unknown/DONE'] == 1, s                        # old START without kind
assert s['evidence_gaps'] == ['c', 'f', 'h', 'v'], s  # v: not the producer's BLOCKED format                    # h finished after an UNKNOWN attempt: still a gap
assert 'full/UNKNOWN' not in s['job_seconds'], s                     # no synthesised seconds for UNKNOWN
assert [r['suite'] for r in s['near_limit']] == ['a'] and s['near_limit'][0]['margin'] == 0.5, s
r = A.reconcile(s, {'results': {'a': {'rc': 0}, 'd': {'rc': 1}, 'e': {'rc': 0}, 'b': {'rc': None}, 'f': {'rc': 0}, 'g': {'rc': 0}, 'u': {'rc': 1}, 'v': {'rc': 1}}})
assert r['rc_mismatch'] == ['e'] and r['missing_from_replay'] == ['b'] and r['extra_in_replay'] == ['h'], r
s['reconcile'] = r; assert A.strict_rc(s) == 1
clean = A.summarise(A.replay(['START z limit=9 kind=full', 'DONE z rc=0 1.00s']))
clean['reconcile'] = A.reconcile(clean, {'results': {'z': {'rc': 0}}}); assert A.strict_rc(clean) == 0, clean
t = A.tail_risk(A.replay(['START x-1 limit=10 kind=tail', 'DEFER x-1 rc=142 10.00s', 'START x-2 limit=46 kind=full', 'DONE x-2 rc=0 30.00s',
                           'START x-3 limit=40 kind=tail', 'DONE x-3 rc=0 5.00s', 'START y-1 limit=9 kind=tail', 'DEFER y-1 rc=142 9.00s']))
assert t['tail_success'] == 1 and t['tail_deferred'] == 2, t
r = {row['suite']: row for row in t['rows']}
assert r['x-1']['sibling_max_s'] == 30.0 and r['x-1']['hint'].startswith('risk'), r       # sibling needed 30 s > tail 10 s
assert r['y-1']['hint'].startswith('UNKNOWN'), r                                         # no sibling evidence: UNKNOWN, not safe
c = A.summarise(A.replay(['START p limit=9 kind=full', 'DONE p rc=0 1.00s', 'START q limit=9 kind=full', 'DONE q rc=1 1.00s wrong']))
good = {'jobs': 2, 'pass': 1, 'classes': {'NEEDS_RULING': 1}, 'rows': [{'suite': 'q', 'cls': 'NEEDS_RULING'}]}
assert A.conserve(c, good)['ok'], A.conserve(c, good)
for bad in ({**good, 'pass': 2}, {**good, 'rows': []}, {**good, 'rows': [{'suite': 'p', 'cls': 'X'}, {'suite': 'q', 'cls': 'NEEDS_RULING'}]},
            {**good, 'rows': good['rows'] + [{'suite': 'z', 'cls': 'X'}]}, {**good, 'classes': {'NEEDS_RULING': 2}}):
    assert not A.conserve(c, bad)['ok'], bad
print('attemptchain  deferral cost kept after success, pending/unknown/blocked/unstarted separated, old START without kind, history gaps kept, near-limit by kind/limit, results reconciled both ways, strict, WF5 rc-class conservation vs exittable, WF2 tail-risk hints (siblings not a lower bound, no evidence UNKNOWN)')
