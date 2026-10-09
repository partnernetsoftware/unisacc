#!/usr/bin/env python3
"""exittable controls (0.0.38 P1): each red lands in exactly one pre-authorised class; PASS,
PENDING and the classes stay apart; an accepted class is never counted as PASS."""
import contextlib, importlib.util, io, json, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('exittable_under_test', ROOT / 'release/tools/exittable.py')
X = importlib.util.module_from_spec(spec); spec.loader.exec_module(X)
with tempfile.TemporaryDirectory() as td:
    t = pathlib.Path(td)
    (t / 'plan.md').write_text('| H1 | 超时：closure-c1..2、selfelf(+package)、tools-2 | cc | x | 0 |\n| H2 | 宿主红：wrong-host、cut-job、plan-only | cc | x | 0 |\n')
    (t / 'rulings.tsv').write_text('wrong-host\tFAILED\tmincore\tH2\tr\ncut-job\tINTERRUPTED\t\tH1\tr\nnew-sig\tFAILED\tmincore\tH2\tr\n')
    res = {'ok': {'rc': 0}, 'host': {'rc': 77, 'status': 'UNVERIFIED'}, 'cut-job': {'rc': None, 'status': 'INTERRUPTED'},
           'tool': {'rc': 1}, 'closure-c2': {'rc': 142}, 'exec-selfelf-package': {'rc': 142},
           'slow-unlisted': {'rc': 142}, 'wrong': {'rc': 1}, 'wrong-host': {'rc': 1}, 'new-sig': {'rc': 1}, 'plan-only': {'rc': 1}}
    logs = {'tool': 'csmithdiff: csmith not found (brew install csmith)\n', 'wrong': 'differ 3\n', 'wrong-host': 'error: implicit declaration of mincore\n', 'new-sig': 'output mismatch\n'}
    for n, l in logs.items(): (t / (n + '.log')).write_text(l)
    (t / 'results.json').write_text(json.dumps({'jobs': {**{n: [] for n in res}, 'later': []}, 'results': res}))
    out = io.StringIO()
    with contextlib.redirect_stdout(out): X.main([str(t), '--h1', str(t / 'plan.md'), '--rulings', str(t / 'rulings.tsv'), '--json'])
    d = json.loads(out.getvalue())
    got = {r['suite']: r['cls'] for r in d['rows']}
    assert got == {'host': 'UNVERIFIED_HOST', 'cut-job': 'RULED_BASELINE', 'tool': 'TOOL_MISSING', 'closure-c2': 'HOST_TIMEOUT',
                   'exec-selfelf-package': 'HOST_TIMEOUT', 'slow-unlisted': 'NEEDS_RULING', 'wrong': 'NEEDS_RULING', 'wrong-host': 'RULED_BASELINE', 'new-sig': 'NEEDS_RULING', 'plan-only': 'NEEDS_RULING', 'later': 'PENDING'}, got
    assert d['pass'] == 1 and d['jobs'] == 12, d   # a ruled red is never PASS; a new signature or a plan-only name needs a ruling
print('exittable: UNVERIFIED/INTERRUPTED/tool/H1 timeout/unlisted/wrong-output/pending classes and PASS separation pass')
