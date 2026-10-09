#!/usr/bin/env python3
"""stagelog/stagesum controls (0.0.38 P6): pairing, whitelist, clock domains, union of intervals,
null for unmeasured values, rc 0 is not PASS; every malformed log is rejected."""
import importlib.util, json, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m
S = load('stagesum_under_test', ROOT / 'release/tools/stagesum.py')
LOG = ROOT / 'release/tools/stagelog.py'

def ev(kind, i, mono, boot='b1', **kw):
    r = {'event': kind, 'id': i, 'mono': mono, 'boot_id': boot, 'utc': '2026-10-09T00:00:00Z'}
    if kind == 'begin': r.update(run='r1', phase=kw.pop('phase', 'queue'), subphase=None, attempt=kw.pop('attempt', 1), parent_id=None, source_commit=None)
    if kind == 'end': r.update(rc=kw.pop('rc', 0), acceptance_status=kw.pop('acc', 'UNVERIFIED'), execution_status=None,
                               authorization_ref=None, cpu_user=kw.pop('cpu_user', None), cpu_sys=kw.pop('cpu_sys', None), rss_kib=kw.pop('rss', None))
    if kind.startswith('wait'): r.update(run='r1', phase=kw.pop('phase', 'queue'), reason='ruling')
    r.update(kw); return r

def rejects(events, what):
    try: S.summarise(events)
    except SystemExit as e: return
    raise AssertionError('accepted: ' + what)

def bad_line(rec, what):
    with tempfile.TemporaryDirectory() as td:
        p = pathlib.Path(td) / 'l.jsonl'; p.write_text(json.dumps(rec) + '\n')
        try: S.load(p)
        except SystemExit: return
    raise AssertionError('accepted: ' + what)

# overlap is a union (10..30 and 20..40 -> 30 s, not 40); waits apart; rc 0 with FAILED acceptance stays FAILED
s = S.summarise([ev('begin', 'a', 10), ev('begin', 'b', 20), ev('end', 'a', 30, rc=0, acc='FAILED'), ev('end', 'b', 40, rc=0),
                 ev('wait-begin', 'w', 40), ev('wait-end', 'w', 100)])['queue']
assert s['wall_s'] == 30.0 and s['wait_s'] == 60.0 and s['status'] == 'COMPLETE', s
assert s['acceptance'] == ['FAILED', 'UNVERIFIED'] and s['rc'] == [0, 0], 'rc 0 must not become PASS'
assert s['cpu_s'] == [None, None] and s['rss_kib_per_command'] == [None, None], 'unmeasured must be null, not 0'
# missing end, cross-boot pair: INCOMPLETE, no invented wall time; several attempts of one phase are listed
s = S.summarise([ev('begin', 'a', 1), ev('begin', 'c', 5, attempt=2), ev('end', 'c', 9, boot='b2')])['queue']
assert s['status'] == 'INCOMPLETE' and s['wall_s'] is None and s['attempts'] == [1, 2], s
assert {x['why'] for x in s['incomplete']} == {'no end', 'begin and end on different boots'}
# failure then success are two commands, both kept
s = S.summarise([ev('begin', 'a', 0), ev('end', 'a', 5, rc=1, acc='FAILED'), ev('begin', 'b', 6, attempt=2), ev('end', 'b', 8, acc='PASS')])['queue']
assert s['rc'] == [1, 0] and s['wall_s'] == 7.0
rejects([ev('end', 'x', 1)], 'orphan end')
rejects([ev('begin', 'a', 1), ev('begin', 'a', 2)], 'duplicate id')
rejects([ev('begin', 'a', 5), ev('end', 'a', 1)], 'end before begin')
# two runs (or two boots) of one phase are summed per clock line, never unioned across them
s = S.summarise([ev('begin', 'a', 10), ev('end', 'a', 20), dict(ev('begin', 'b', 10), run='r2'), dict(ev('end', 'b', 20)),
                 ev('begin', 'c', 10, boot='b2'), ev('end', 'c', 20, boot='b2')])['queue']
assert s['wall_s'] == 30.0, s
rejects([ev('wait-begin', 'w', 9), ev('wait-end', 'w', 3)], 'wait ending before it began')
rejects([ev('wait-begin', 'w', 1), ev('wait-end', 'w', 3, phase='seal')], 'wait ending in another phase')
rejects([ev('begin', 'x', 1), ev('wait-begin', 'x', 2)], 'command and wait sharing an id')
rejects([ev('begin', 'a', 1, parent_id='nope')], 'unknown parent')
bad_line({k: v for k, v in ev('begin', 'a', 1).items() if k != 'phase'}, 'missing phase')
bad_line(ev('begin', 'a', 1, phase='lunch'), 'unknown phase')
bad_line(dict(ev('begin', 'a', 1), mono=float('nan')), 'NaN time')
bad_line(ev('end', 'a', 1, acc='GREEN'), 'unknown acceptance status')
bad_line(ev('end', 'a', 1, rss=-1), 'negative RSS')
bad_line(dict(ev('begin', 'a', 1), argv='make x'), 'field outside whitelist')
bad_line(dict(ev('begin', 'a', 1), run='/home/someone/x'), 'absolute home path')
bad_line(dict(ev('end', 'a', 1), execution_status='ghp_abcdef'), 'credential-looking value')

# the writer: private log outside the repo, whitelisted fields, paired ids, refusals
with tempfile.TemporaryDirectory() as td:
    log = pathlib.Path(td) / 'ev.jsonl'
    run = lambda *a: subprocess.run([sys.executable, str(LOG), '--log', str(log), *a], capture_output=True, text=True, timeout=20)
    r = run('begin', '--run', 'v0.0.38', '--phase', 'queue', '--source-commit', 'abc1234'); assert r.returncode == 0, r.stderr
    eid = r.stdout.strip()
    assert run('end', '--id', eid, '--rc', '0').returncode == 0
    w = run('wait', '--action', 'begin', '--run', 'v0.0.38', '--phase', 'seal', '--reason', 'ruling').stdout.strip()
    assert run('wait', '--action', 'end', '--run', 'v0.0.38', '--phase', 'seal', '--id', w).returncode == 0
    s = S.summarise(S.load(log))
    assert s['queue']['status'] == 'COMPLETE' and s['queue']['acceptance'] == ['UNVERIFIED'], 'default acceptance is UNVERIFIED'
    assert s['seal']['wait_s'] is not None
    assert run('begin', '--run', 'v0.0.38', '--phase', 'nonsense').returncode != 0, 'unknown phase accepted'
    assert run('begin', '--run', '/tmp/x', '--phase', 'queue').returncode != 0, 'path accepted as run name'
    assert subprocess.run([sys.executable, str(LOG), '--log', str(ROOT / 'stagelog.jsonl'), 'begin', '--run', 'r', '--phase', 'queue'],
                          capture_output=True, timeout=20).returncode != 0, 'log inside the repository accepted'
    assert not (ROOT / 'stagelog.jsonl').exists()
    assert run('end', '--id', eid, '--cpu-user', 'nan').returncode != 0, 'NaN CPU accepted'
    link = pathlib.Path(td) / 'link.jsonl'; link.symlink_to(pathlib.Path(td) / 'elsewhere.jsonl')
    assert subprocess.run([sys.executable, str(LOG), '--log', str(link), 'begin', '--run', 'r', '--phase', 'queue'],
                          capture_output=True, timeout=20).returncode != 0, 'symlinked log accepted'
print('stagelog: union of overlaps, waits, null for unmeasured, rc0 != PASS, INCOMPLETE for missing end/cross-boot, '
      'per-run/boot clock lines, wait order/run/phase, shared/unknown ids, missing/unknown fields, NaN, '
      'orphan/duplicate/order/whitelist/path/credential rejections and writer refusals (NaN, symlink, in-repo) pass')
