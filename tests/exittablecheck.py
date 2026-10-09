#!/usr/bin/env python3
"""exittable controls (0.0.38 P1): each red lands in exactly one pre-authorised class; PASS,
PENDING and the classes stay apart; an accepted class is never counted as PASS."""
import contextlib, importlib.util, io, json, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('exittable_under_test', ROOT / 'release/tools/exittable.py')
X = importlib.util.module_from_spec(spec); spec.loader.exec_module(X)
with tempfile.TemporaryDirectory() as td:
    t = pathlib.Path(td)
    (t / 'plan.md').write_text('| H1 | 超时：closure-c1..2、selfelf(+package)、tools-2 | cc | x | 0 |\n')
    res = {'ok': {'rc': 0}, 'host': {'rc': 77, 'status': 'UNVERIFIED'}, 'cut': {'rc': None, 'status': 'INTERRUPTED'},
           'tool': {'rc': 1}, 'closure-c2': {'rc': 142}, 'exec-selfelf-package': {'rc': 142},
           'slow-unlisted': {'rc': 142}, 'wrong': {'rc': 1}}
    logs = {'tool': 'csmithdiff: csmith not found (brew install csmith)\n', 'wrong': 'differ 3\n'}
    for n, l in logs.items(): (t / (n + '.log')).write_text(l)
    (t / 'results.json').write_text(json.dumps({'jobs': {**{n: [] for n in res}, 'later': []}, 'results': res}))
    out = io.StringIO()
    with contextlib.redirect_stdout(out): X.main([str(t), '--h1', str(t / 'plan.md'), '--json'])
    d = json.loads(out.getvalue())
    got = {r['suite']: r['cls'] for r in d['rows']}
    assert got == {'host': 'UNVERIFIED_HOST', 'cut': 'OUTER_INTERRUPTED', 'tool': 'TOOL_MISSING', 'closure-c2': 'HOST_TIMEOUT',
                   'exec-selfelf-package': 'HOST_TIMEOUT', 'slow-unlisted': 'NEEDS_RULING', 'wrong': 'NEEDS_RULING', 'later': 'PENDING'}, got
    assert d['pass'] == 1 and d['jobs'] == 9, d
print('exittable: UNVERIFIED/INTERRUPTED/tool/H1 timeout/unlisted/wrong-output/pending classes and PASS separation pass')
