#!/usr/bin/env python3
"""exittable controls (0.0.38 P1): each red lands in exactly one pre-authorised class; PASS,
PENDING and the classes stay apart; an accepted class is never counted as PASS."""
import contextlib, hashlib, importlib.util, io, json, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('exittable_under_test', ROOT / 'release/tools/exittable.py')
X = importlib.util.module_from_spec(spec); spec.loader.exec_module(X)

def _run(argv):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        rc = X.main(argv)
    return rc or 0, out.getvalue(), err.getvalue()

# classification fixture: --root's own H1 row is the table (no --h1 bypass of the declaration)
with tempfile.TemporaryDirectory() as td:
    r = pathlib.Path(td); (r / 'src').mkdir(); (r / 'plans').mkdir(); q = r / 'q'; q.mkdir()
    (r / 'src/version.h').write_text('#define UNISACC_VERSION "0.0.99"\n')
    (r / 'plans/v0.0.99.md').write_text(
        '| H1 | 超时：closure-c1..2、selfelf(+package)、tools-2 | cc | x | 0 |\n'
        '| H2 | 宿主红：wrong-host、cut-job、plan-only | cc | x | 0 |\n')
    (q / 'rulings.tsv').write_text(
        'wrong-host\tFAILED\tmincore\tH2\tr\nwrong-host\tFAILED\tTimeoutExpired\tH1\tr2\n'
        'stale-word\tFAILED\tTimeoutExpired\tH1\tr\ncut-job\tTIMEOUT\t\tH1\tr\n'
        'cut-job\tINTERRUPTED\t\tH1\tr\nsecond-rule\tFAILED\tmincore\tH2\tr\n'
        'second-rule\tFAILED\tTimeoutExpired\tH1\tr2\nown-limit\tINTERRUPTED\t\tH1\tr\n'
        'new-sig\tFAILED\tmincore\tH2\tr\n')
    res = {'ok': {'rc': 0}, 'host': {'rc': 77, 'status': 'UNVERIFIED'}, 'cut-job': {'rc': None, 'status': 'INTERRUPTED'},
           'tool': {'rc': 1}, 'closure-c2': {'rc': 142}, 'exec-selfelf-package': {'rc': 142},
           'slow-unlisted': {'rc': 142}, 'wrong': {'rc': 1}, 'wrong-host': {'rc': 1}, 'new-sig': {'rc': 1},
           'plan-only': {'rc': 1}, 'second-rule': {'rc': 1}, 'stale-word': {'rc': 1}, 'own-limit': {'rc': 142},
           'mem-gap': {'rc': 2}, 'plain-two': {'rc': 2}, 'shard-1': {'rc': 1, 'seconds': 0, 'limit': 0}}
    logs = {'tool': 'csmithdiff: csmith not found (brew install csmith)\n', 'wrong': 'differ 3\n',
            'wrong-host': 'error: implicit declaration of mincore\n', 'new-sig': 'output mismatch\n',
            'stale-word': 'note: earlier TimeoutExpired was retried fine\nok 3\nwrong output: differ 1\n'}
    for n, l in logs.items(): (q / (n + '.log')).write_text(l)
    (q / 'mem-gap.log').write_text('UNKNOWN: required=memory observed=unknown missing=memory-evidence reason=unmeasured\n')
    (q / 'plain-two.log').write_text('usage: x\nUNKNOWN: required=memory but then\nreal error: bad option\n')
    (q / 'second-rule.log').write_text('subprocess.TimeoutExpired: 40 s\n')
    (q / 'results.json').write_text(json.dumps({'jobs': {**{n: [] for n in res}, 'later': []}, 'results': res}))
    rc, out, err = _run([str(q), '--root', str(r), '--rulings', str(q / 'rulings.tsv'), '--json'])
    assert rc == 0, (rc, out, err)
    d = json.loads(out)
    got = {row['suite']: row['cls'] for row in d['rows']}
    assert got == {'host': 'UNVERIFIED_HOST', 'cut-job': 'RULED_BASELINE', 'tool': 'TOOL_MISSING', 'closure-c2': 'HOST_TIMEOUT',
                   'exec-selfelf-package': 'HOST_TIMEOUT', 'slow-unlisted': 'NEEDS_RULING', 'wrong': 'NEEDS_RULING',
                   'wrong-host': 'RULED_BASELINE', 'new-sig': 'NEEDS_RULING', 'plan-only': 'NEEDS_RULING',
                   'second-rule': 'RULED_BASELINE', 'stale-word': 'NEEDS_RULING', 'own-limit': 'NEEDS_RULING',
                   'mem-gap': 'RESOURCE_UNKNOWN', 'plain-two': 'NEEDS_RULING', 'shard-1': 'BLOCKED', 'later': 'PENDING'}, got
    assert d['pass'] == 1 and d['jobs'] == 18, d

# h1_names unit cases (CLI no longer accepts an arbitrary --h1 for these)
with tempfile.TemporaryDirectory() as hn:
    hp = pathlib.Path(hn)
    (hp / 'empty.md').write_text('| H1 | EMPTY |\n')
    assert X.h1_names(hp / 'empty.md') == set()
    for i, text in enumerate(('# no row\n', '| H1 | 无 | cc | x | 0 |\n', '| H1 | a-b |\n| H1 | c-d |\n',
                              '| H1 nonsense fake-suite\n', '| H1 | EMPTY | fake-suite |\n',
                              '| H1 | fake-suite | EMPTY |\n', '| H1 | fake-suite\n', '| H1 | EMPTY\n')):
        f = hp / ('bad%d.md' % i); f.write_text(text)
        try: X.h1_names(f); raise AssertionError('expected H1Error for %r' % text)
        except X.H1Error:
            pass

# 0.0.40-prep / 23:15: missing path and pre-archive default disagree with the settled table -> rc 2 (not a bypass);
# the declared archived table (same path as H1-INHERIT) is accepted; a different old plan with a valid H1 row is refused
with tempfile.TemporaryDirectory() as m:
    mt = pathlib.Path(m); (mt / 'results.json').write_text(json.dumps({'jobs': {}}))
    settled = str((ROOT / 'archive/plans/v0.0.38.md').resolve())
    for bad in (str(mt / 'no-such-plan.md'), str(ROOT / 'plans/v0.0.38.md')):
        rc, out, err = _run([str(mt), '--h1', bad])
        assert rc == 2 and out == '' and 'not a bypass' in err, (bad, rc, out, err)
    rc, out, err = _run([str(mt), '--h1', settled])
    assert rc == 0, (rc, out, err)
    rc, out, err = _run([str(mt), '--h1', str(ROOT / 'archive/plans/v0.0.38.md')])  # relative to cwd form still ok
    assert rc == 0, (rc, out, err)
    # another archived plan that has an H1-shaped row must still be refused when it is not the declaration
    other = ROOT / 'archive/plans/v0.0.37.md'
    if other.is_file() and '| H1 ' in other.read_text():
        rc, out, err = _run([str(mt), '--h1', str(other)])
        assert rc == 2 and out == '' and 'not a bypass' in err, (rc, out, err)

# 0.0.40 (机房主任 22:53 E): with no --h1 the table is the one this version's plan settles;
# 0.0.40 (机房主任 23:15): --h1 that disagrees with own row / H1-INHERIT is rc 2 (including pointing at an old plan)
with tempfile.TemporaryDirectory() as m:
    r = pathlib.Path(m); (r / 'src').mkdir(); (r / 'plans').mkdir(); (r / 'archive/plans').mkdir(parents=True); (r / 'q').mkdir()
    (r / 'q/results.json').write_text(json.dumps({'jobs': {'tools-2': 1}, 'results': {'tools-2': {'rc': 142, 'seconds': 40, 'limit': 40}}}))
    (r / 'q/tools-2.log').write_text('timeout\n')
    old = '| H1 | 超时：tools-2 | cc | x | 0 |\n'; (r / 'archive/plans/v0.0.38.md').write_text(old)
    decoy = '| H1 | 超时：other-suite | cc | x | 0 |\n'; (r / 'archive/plans/v0.0.37.md').write_text(decoy)
    def run(version, plan, extra=()):
        (r / 'src/version.h').write_text('#define UNISACC_VERSION "%s"\n' % version)
        for f in (r / 'plans').iterdir(): f.unlink()
        if plan is not None: (r / ('plans/v%s.md' % version)).write_text(plan)
        return _run([str(r / 'q'), '--root', str(r), '--json', *extra])
    sha = hashlib.sha256(old.encode()).hexdigest()
    inherit = 'H1-INHERIT: archive/plans/v0.0.38.md sha256=%s host=h\n' % sha
    rc, out, err = run('0.0.40', inherit)
    assert rc == 0 and json.loads(out)['rows'][0]['cls'] == 'HOST_TIMEOUT', (rc, out, err)
    # matching --h1 (absolute or relative-to-root form) accepted
    rc, out, err = run('0.0.40', inherit, ('--h1', str(r / 'archive/plans/v0.0.38.md')))
    assert rc == 0 and json.loads(out)['rows'][0]['cls'] == 'HOST_TIMEOUT', (rc, out, err)
    # disagreeing --h1: old/decoy plan, or the version plan itself when H1 is inherited -> not a bypass
    for h1 in (str(r / 'archive/plans/v0.0.37.md'), str(r / 'plans/v0.0.40.md')):
        rc, out, err = run('0.0.40', inherit, ('--h1', h1))
        assert rc == 2 and out == '' and 'not a bypass' in err, (h1, rc, out, err)
    # broken declaration cannot be rescued by --h1 pointing at a valid old table
    rc, out, err = run('0.0.40', '# no declaration\n', ('--h1', str(r / 'archive/plans/v0.0.38.md')))
    assert rc == 2 and out == '' and 'no H1 table for this version' in err, (rc, out, err)
    rc, out, err = run('0.0.41', '| H1 | EMPTY |\n')
    assert rc == 0 and json.loads(out)['rows'][0]['cls'] == 'NEEDS_RULING', (rc, out, err)
    # OWN row: --h1 must be the plan itself; pointing at the old inherit target is refused
    own = '| H1 | 超时：tools-2 | cc | x | 0 |\n'
    rc, out, err = run('0.0.41', own, ('--h1', str(r / 'plans/v0.0.41.md')))
    assert rc == 0 and json.loads(out)['rows'][0]['cls'] == 'HOST_TIMEOUT', (rc, out, err)
    rc, out, err = run('0.0.41', own, ('--h1', str(r / 'archive/plans/v0.0.38.md')))
    assert rc == 2 and out == '' and 'not a bypass' in err, (rc, out, err)
    for plan in ('# no declaration\n', None, 'H1-INHERIT: archive/plans/v0.0.38.md sha256=%s host=h\n' % ('0' * 64), '| H1 | tools-2\n'):
        rc, out, err = run('0.0.40', plan)
        assert rc == 2 and out == '' and 'no H1 table for this version' in err, (plan, rc, out, err)

print("exittable: UNVERIFIED/INTERRUPTED/tool/H1 timeout/unlisted/wrong-output/pending classes and PASS separation pass; "
      "missing/disagreeing --h1 (incl. old plan) rc 2 'not a bypass', settled archived/own table accepted; "
      "h1_names missing/nameless/duplicate/malformed/unclosed/EMPTY-beside rc raises, only '| H1 | EMPTY |' is empty; "
      "no --h1 = this version's own row or H1-INHERIT, none/broken rc 2 (cannot rescue with --h1)")
