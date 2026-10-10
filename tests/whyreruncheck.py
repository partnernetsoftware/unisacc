#!/usr/bin/env python3
"""whyrerun controls (0.0.39): explain() names the changed part per suite and classifies it -- a product
input, a test input or command (contract), the shared environment (host), the global fallback (always
UNKNOWN cause, never narrowed); equal stamps are reusable; the real fingerprint keeps its stamps when
parts are collected."""
import importlib.util, os, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('whyrerun_under_test', ROOT / 'release/tools/whyrerun.py')
W = importlib.util.module_from_spec(spec); spec.loader.exec_module(W)
def suite(audited=True, **kv):
    base = {'audited': audited, 'common': {k: '0' for k in W.COMMON_KEYS}, 'command': 'c', 'inputs': {'tests/x.py': '1', 'exec/a.py': '1'}}
    for k, v in kv.items(): base[k] = v
    return base
A = {'stamps': {n: '0' for n in 'pqrstu'}, 'suites': {'p': suite(), 'q': suite(), 'r': suite(), 's': suite(audited=False, global_inputs={'f': '1'}),
     't': suite(inventory={'exec': '1'}), 'u': suite()}}
B = {'stamps': {'p': '1', 'q': '1', 'r': '1', 's': '1', 't': '1', 'u': '0'}, 'suites': {
     'p': suite(inputs={'tests/x.py': '1', 'exec/a.py': '2'}),                        # product input
     'q': suite(inputs={'tests/x.py': '2', 'exec/a.py': '1'}),                        # test input
     'r': suite(common={**{k: '0' for k in W.COMMON_KEYS}, 'platform': '9'}),         # host
     's': suite(audited=False, global_inputs={'f': '2'}),                             # global fallback
     't': suite(inventory={'exec': '2'}), 'u': suite()}}                              # reviewed product tree; u reusable
r = W.explain(A, B)['rows']
assert r['p']['category'] == ['product'] and r['q']['category'] == ['contract'] and r['r']['category'] == ['host'], r
assert r['s']['category'] == ['global'] and 'UNKNOWN' in r['s']['reasons'][0] and r['t']['category'] == ['product'] and 'u' not in r, r
sys.path.insert(0, str(ROOT / 'tests')); os.chdir(ROOT)
spec = importlib.util.spec_from_file_location('gq_under_test', ROOT / 'tests/gatequeue.py')
G = importlib.util.module_from_spec(spec); spec.loader.exec_module(G)
jobs = {'whyrerun-probe': ['true']}; parts = {}
assert G.fingerprint(jobs) == G.fingerprint(jobs, parts) and set(parts) == {'whyrerun-probe'}
print('whyrerun  product/contract/host/global reasons per suite, global stays UNKNOWN-cause, reusable suites omitted, stamps unchanged by parts')
