#!/usr/bin/env python3
"""stagesum: summarise a stagelog event log (0.0.38 P6).   usage: stagesum.py [--log L] [--run R] [--json]

Pairs begin/end and wait-begin/wait-end by id.  A duplicate id, an end without a begin, a field
outside the whitelist or a value that looks like a path or a credential rejects the log (rc 1).
A begin without an end is INCOMPLETE; a pair whose ends lie on different boots has no wall time
(the monotonic clocks are not comparable) and is INCOMPLETE too.  Per phase: total wall is the
union of its command intervals (overlap is not added twice), waits likewise; attempts, rc and
acceptance statuses are listed apart -- rc 0 is not PASS, a ruling is not PASS.  Unmeasured CPU
and RSS stay null.  Subphases (window-N folded to window) get their own union and count beside the
phase total.  Scope of the queue segments: prologue starts inside gatequeue (term.sh, release.sh's
checks and warm-up are only in the enclosing window); segments are written when a window ends
normally, so a window killed from outside has no segment events -- its split is unknown, never
inferred from the parent's missing end.
"""
import argparse, json, pathlib, re, sys

FIELDS = {
    'begin': {'event', 'id', 'run', 'phase', 'subphase', 'attempt', 'parent_id', 'source_commit', 'mono', 'boot_id', 'utc'},
    'end': {'event', 'id', 'rc', 'acceptance_status', 'execution_status', 'authorization_ref', 'cpu_user', 'cpu_sys', 'rss_kib', 'mono', 'boot_id', 'utc'},
    'wait-begin': {'event', 'id', 'run', 'phase', 'reason', 'mono', 'boot_id', 'utc'},
    'wait-end': {'event', 'id', 'run', 'phase', 'reason', 'mono', 'boot_id', 'utc'},
}
PHASES = ('code', 'matrix', 'freeze', 'candidate', 'fixedpoint', 'accept17', 'seal', 'queue', 'draft', 'public')
ACCEPT = ('PASS', 'FAILED', 'UNVERIFIED', 'ACCEPTED_BY_RULING')
REQUIRED = {'begin': {'event', 'id', 'run', 'phase', 'mono', 'boot_id'}, 'end': {'event', 'id', 'mono', 'boot_id', 'acceptance_status'},
            'wait-begin': {'event', 'id', 'run', 'mono', 'boot_id'}, 'wait-end': {'event', 'id', 'run', 'mono', 'boot_id'}}
SENSITIVE = re.compile(r'(/home/|/Users/|/tmp/|/var/folders/|[A-Za-z]:\\\\|token|password|secret|ghp_|gho_)', re.I)


def load(path):
    events = []
    for k, line in enumerate(pathlib.Path(path).read_text().splitlines(), 1):
        if not line.strip(): continue
        try: r = json.loads(line)
        except json.JSONDecodeError: raise SystemExit('stagesum: line %d is not JSON' % k)
        kind = r.get('event')
        if kind not in FIELDS: raise SystemExit('stagesum: line %d: unknown event %r' % (k, kind))
        extra = set(r) - FIELDS[kind]
        if extra: raise SystemExit('stagesum: line %d: fields outside the whitelist: %s' % (k, ','.join(sorted(extra))))
        missing = REQUIRED[kind] - set(r)
        if missing: raise SystemExit('stagesum: line %d: missing fields %s' % (k, ','.join(sorted(missing))))
        m = r.get('mono')
        if not isinstance(m, (int, float)) or isinstance(m, bool) or m != m or m in (float('inf'), float('-inf')):
            raise SystemExit('stagesum: line %d: mono must be a finite number' % k)
        if r.get('phase') is not None and r['phase'] not in PHASES:
            raise SystemExit('stagesum: line %d: unknown phase %r' % (k, r['phase']))
        if kind == 'end' and r.get('acceptance_status') not in ACCEPT:
            raise SystemExit('stagesum: line %d: unknown acceptance status %r' % (k, r.get('acceptance_status')))
        for f in ('cpu_user', 'cpu_sys', 'rss_kib'):
            v = r.get(f)
            if v is not None and (not isinstance(v, (int, float)) or isinstance(v, bool) or v != v or v < 0 or v == float('inf')):
                raise SystemExit('stagesum: line %d: %s must be null or a finite non-negative number' % (k, f))
        for f, v in r.items():
            if isinstance(v, str) and f != 'utc' and SENSITIVE.search(v):
                raise SystemExit('stagesum: line %d: field %s looks like a path or credential' % (k, f))
        events.append(r)
    return events


def union(spans):
    """Sum of per-(run, boot) unions: monotonic times are comparable only on one boot, and two runs
    of one phase are two stretches of work, not one overlapping interval."""
    groups = {}
    for key, a, b in spans: groups.setdefault(key, []).append((a, b))
    return round(sum(union1(v) for v in groups.values()), 3)


def union1(spans):
    total, end = 0.0, None
    for a, b in sorted(spans):
        if end is None or a > end: total += b - a; end = b
        elif b > end: total += b - end; end = b
    return round(total, 3)


def summarise(events, run=None):
    begins, ends, wb, we = {}, {}, {}, {}
    for r in events:
        kind, i = r['event'], r['id']
        table = {'begin': begins, 'end': ends, 'wait-begin': wb, 'wait-end': we}[kind]
        if i in table: raise SystemExit('stagesum: duplicate %s id %s' % (kind, i))
        table[i] = r
    for i in ends:
        if i not in begins: raise SystemExit('stagesum: end without begin: %s' % i)
    for i in we:
        if i not in wb: raise SystemExit('stagesum: wait end without begin: %s' % i)
        a, z = wb[i], we[i]
        if (a.get('run'), a.get('phase')) != (z.get('run'), z.get('phase')):
            raise SystemExit('stagesum: wait %s ends in another run/phase' % i)
        if a.get('boot_id') == z.get('boot_id') and z['mono'] < a['mono']:
            raise SystemExit('stagesum: wait end before begin: %s' % i)
    if set(begins) & set(wb): raise SystemExit('stagesum: a command and a wait share an id')
    for i, b in begins.items():
        if b.get('parent_id') and b['parent_id'] not in begins: raise SystemExit('stagesum: unknown parent %s' % b['parent_id'])
    phases = {}
    for i, b in begins.items():
        if run and b['run'] != run: continue
        p = phases.setdefault(b['phase'], {'spans': [], 'waits': [], 'commands': 0, 'attempts': set(), 'incomplete': [],
                                            'rc': [], 'acceptance': [], 'cpu': [], 'rss': []})
        p['commands'] += 1; p['attempts'].add(b.get('attempt', 1))
        e = ends.get(i)
        if e is None: p['incomplete'].append({'id': i, 'why': 'no end'}); continue
        p['rc'].append(e.get('rc')); p['acceptance'].append(e.get('acceptance_status'))
        if b.get('boot_id') is None or b.get('boot_id') != e.get('boot_id'):
            p['incomplete'].append({'id': i, 'why': 'begin and end on different boots'}); continue
        if e['mono'] < b['mono']: raise SystemExit('stagesum: end before begin: %s' % i)
        p['spans'].append(((b['run'], b['boot_id']), b['mono'], e['mono']))
        sub = re.sub(r'-\d+$', '', b.get('subphase') or '(whole)')     # window-7 -> window
        p.setdefault('subs', {}).setdefault(sub, []).append(((b['run'], b['boot_id']), b['mono'], e['mono']))
        cpu = None if e.get('cpu_user') is None or e.get('cpu_sys') is None else round(e['cpu_user'] + e['cpu_sys'], 3)
        p['cpu'].append(cpu); p['rss'].append(e.get('rss_kib'))
    for i, b in wb.items():
        if run and b['run'] != run: continue
        e = we.get(i)
        p = phases.setdefault(b.get('phase') or '(between phases)', {'spans': [], 'waits': [], 'commands': 0, 'attempts': set(),
                                                                       'incomplete': [], 'rc': [], 'acceptance': [], 'cpu': [], 'rss': []})
        if e is None or e.get('boot_id') != b.get('boot_id') or b.get('boot_id') is None:
            p['incomplete'].append({'id': i, 'why': 'wait without end' if e is None else 'wait across boots'}); continue
        p['waits'].append(((b['run'], b['boot_id']), b['mono'], e['mono']))
    out = {}
    for name, p in phases.items():
        out[name] = {
            'wall_s': union(p['spans']) if p['spans'] else None,
            'wait_s': union(p['waits']) if p['waits'] else None,
            'commands': p['commands'], 'attempts': sorted(p['attempts']),
            'rc': p['rc'], 'acceptance': p['acceptance'],
            'cpu_s': p['cpu'], 'rss_kib_per_command': p['rss'],
            # per subphase: union and count, apart from the phase total (a parent and its parts are not added)
            'subphases': {k: {'wall_s': union(v), 'count': len(v)} for k, v in sorted(p.get('subs', {}).items())},
            'status': 'INCOMPLETE' if p['incomplete'] else 'COMPLETE',
            'incomplete': p['incomplete'],
        }
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--log', default=str(pathlib.Path.home() / '.unisacc' / 'stagelog' / 'events.jsonl'))
    ap.add_argument('--run'); ap.add_argument('--json', action='store_true')
    a = ap.parse_args(argv)
    if not pathlib.Path(a.log).is_file(): print('stagesum: no event log at %s' % a.log, file=sys.stderr); return 2
    s = summarise(load(a.log), a.run)
    if a.json: print(json.dumps(s, indent=2, sort_keys=True)); return 0
    print('%-11s %9s %9s %4s %-10s %s' % ('phase', 'wall_s', 'wait_s', 'cmd', 'status', 'acceptance'))
    for name, r in s.items():
        f = lambda v: '-' if v is None else '%.1f' % v
        print('%-11s %9s %9s %4d %-10s %s' % (name, f(r['wall_s']), f(r['wait_s']), r['commands'], r['status'],
                                              ','.join(str(x) for x in r['acceptance']) or '-'))
        for sub, v in r['subphases'].items():
            print('  %-9s %9s %9s %4d' % (sub, f(v['wall_s']), '', v['count']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
