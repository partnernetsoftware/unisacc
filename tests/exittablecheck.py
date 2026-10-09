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
    (t / 'rulings.tsv').write_text('wrong-host\tFAILED\tmincore\tH2\tr\nwrong-host\tFAILED\tTimeoutExpired\tH1\tr2\nstale-word\tFAILED\tTimeoutExpired\tH1\tr\ncut-job\tTIMEOUT\t\tH1\tr\ncut-job\tINTERRUPTED\t\tH1\tr\nsecond-rule\tFAILED\tmincore\tH2\tr\nsecond-rule\tFAILED\tTimeoutExpired\tH1\tr2\nown-limit\tINTERRUPTED\t\tH1\tr\nnew-sig\tFAILED\tmincore\tH2\tr\n')
    res = {'ok': {'rc': 0}, 'host': {'rc': 77, 'status': 'UNVERIFIED'}, 'cut-job': {'rc': None, 'status': 'INTERRUPTED'},
           'tool': {'rc': 1}, 'closure-c2': {'rc': 142}, 'exec-selfelf-package': {'rc': 142},
           'slow-unlisted': {'rc': 142}, 'wrong': {'rc': 1}, 'wrong-host': {'rc': 1}, 'new-sig': {'rc': 1}, 'plan-only': {'rc': 1}, 'second-rule': {'rc': 1}, 'stale-word': {'rc': 1}, 'own-limit': {'rc': 142}, 'mem-gap': {'rc': 2}, 'plain-two': {'rc': 2}, 'shard-1': {'rc': 1, 'seconds': 0, 'limit': 0}}
    logs = {'tool': 'csmithdiff: csmith not found (brew install csmith)\n', 'wrong': 'differ 3\n', 'wrong-host': 'error: implicit declaration of mincore\n', 'new-sig': 'output mismatch\n', 'stale-word': 'note: earlier TimeoutExpired was retried fine\nok 3\nwrong output: differ 1\n'}
    for n, l in logs.items(): (t / (n + '.log')).write_text(l)
    (t / 'mem-gap.log').write_text('UNKNOWN: required=memory observed=unknown missing=memory-evidence reason=unmeasured\n')
    (t / 'plain-two.log').write_text('usage: x\nUNKNOWN: required=memory but then\nreal error: bad option\n')
    (t / 'second-rule.log').write_text('subprocess.TimeoutExpired: 40 s\n')
    (t / 'results.json').write_text(json.dumps({'jobs': {**{n: [] for n in res}, 'later': []}, 'results': res}))
    out = io.StringIO()
    with contextlib.redirect_stdout(out): X.main([str(t), '--h1', str(t / 'plan.md'), '--rulings', str(t / 'rulings.tsv'), '--json'])
    d = json.loads(out.getvalue())
    got = {r['suite']: r['cls'] for r in d['rows']}
    assert got == {'host': 'UNVERIFIED_HOST', 'cut-job': 'RULED_BASELINE', 'tool': 'TOOL_MISSING', 'closure-c2': 'HOST_TIMEOUT',
                   'exec-selfelf-package': 'HOST_TIMEOUT', 'slow-unlisted': 'NEEDS_RULING', 'wrong': 'NEEDS_RULING', 'wrong-host': 'RULED_BASELINE', 'new-sig': 'NEEDS_RULING', 'plan-only': 'NEEDS_RULING', 'second-rule': 'RULED_BASELINE', 'stale-word': 'NEEDS_RULING', 'own-limit': 'NEEDS_RULING', 'mem-gap': 'RESOURCE_UNKNOWN', 'plain-two': 'NEEDS_RULING', 'shard-1': 'BLOCKED', 'later': 'PENDING'}, got
    assert d['pass'] == 1 and d['jobs'] == 18, d   # a ruled red is never PASS; a new signature or a plan-only name needs a ruling
print('exittable: UNVERIFIED/INTERRUPTED/tool/H1 timeout/unlisted/wrong-output/pending classes and PASS separation pass')
