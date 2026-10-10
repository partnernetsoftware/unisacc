#!/usr/bin/env python3
"""attemptchain.py RELEASE_QUEUE_LOG [--results results.json] [--json] (0.0.39 WF5).

Offline replay of a queue log into per-suite attempt chains; never runs or resumes a queue.
- cumulative cost (job-seconds by kind and outcome) and the *current* pending set stay apart:
  a deferral keeps its cost after a later success, but the suite leaves the pending set;
- a START with no DONE/DEFER after it is UNKNOWN (never a synthesised rc or PASS);
- a DONE without a START is BLOCKED (predecessor failed: no execution, no seconds);
- job-seconds are not wall clock; near-limit passes are listed per kind and limit.
With --results the replay is reconciled against the queue's results.json (suite count and final rc)."""
import argparse, collections, json, re, sys

START = re.compile(r'START (\S+) limit=(\d+) kind=(\w+)')
END = re.compile(r'(DONE|DEFER) (\S+) rc=(\S+) ([\d.]+)s')


def replay(lines):
    chains, open_ = collections.defaultdict(list), {}
    for line in lines:
        m = START.match(line)
        if m:
            name, limit, kind = m.group(1), int(m.group(2)), m.group(3)
            if name in open_:                       # a second START before an end: the first never ended
                chains[name].append({**open_.pop(name), 'outcome': 'UNKNOWN', 'rc': None, 'seconds': None})
            open_[name] = {'kind': kind, 'limit': limit}
            continue
        m = END.match(line)
        if m:
            ev, name, rc, secs = m.group(1), m.group(2), m.group(3), float(m.group(4))
            att = open_.pop(name, None)
            if att is None:
                chains[name].append({'kind': 'blocked', 'limit': None, 'outcome': 'BLOCKED', 'rc': rc, 'seconds': 0.0})
            else:
                chains[name].append({**att, 'outcome': ev, 'rc': rc, 'seconds': secs})
    for name, att in open_.items():
        chains[name].append({**att, 'outcome': 'UNKNOWN', 'rc': None, 'seconds': None})
    return dict(chains)


def summarise(chains, near=2.0):
    cost = collections.Counter(); count = collections.Counter()
    final, pending, unknown, nearlimit = {}, [], [], []
    for name, seq in chains.items():
        for a in seq:
            key = '%s/%s' % (a['kind'], a['outcome'])
            count[key] += 1
            if a['seconds'] is not None: cost[key] += a['seconds']
        last = seq[-1]
        if last['outcome'] in ('DONE', 'BLOCKED'):
            final[name] = last['rc']
            if last['outcome'] == 'DONE' and last['rc'] == '0' and last['limit'] and last['limit'] - last['seconds'] < near:
                nearlimit.append({'suite': name, 'kind': last['kind'], 'limit': last['limit'], 'seconds': last['seconds'],
                                  'margin': round(last['limit'] - last['seconds'], 3)})
        elif last['outcome'] == 'DEFER':
            pending.append(name)
        else:
            unknown.append(name)
    return {'suites': len(chains), 'final': len(final), 'pending': sorted(pending), 'unknown': sorted(unknown),
            'attempts': dict(count), 'job_seconds': {k: round(v, 1) for k, v in cost.items()},
            'near_limit': sorted(nearlimit, key=lambda r: r['margin']), 'final_rc': final}


def reconcile(summary, results):
    res = results.get('results', {})
    diff = [n for n, rc in summary['final_rc'].items() if n in res and str(res[n].get('rc')) != str(rc)]
    return {'results_suites': len(res), 'replayed_final': summary['final'],
            'missing_from_replay': sorted(set(res) - set(summary['final_rc'])), 'rc_mismatch': sorted(diff)}


def main(argv=None):
    ap = argparse.ArgumentParser(); ap.add_argument('log'); ap.add_argument('--results'); ap.add_argument('--json', action='store_true')
    a = ap.parse_args(argv)
    s = summarise(replay(open(a.log).read().splitlines()))
    if a.results: s['reconcile'] = reconcile(s, json.load(open(a.results)))
    if a.json:
        print(json.dumps({k: v for k, v in s.items() if k != 'final_rc'}, indent=1)); return 0
    print('attemptchain  suites %d  final %d  pending %d  unknown %d' % (s['suites'], s['final'], len(s['pending']), len(s['unknown'])))
    for k in sorted(s['attempts']): print('  %-22s %4d attempts %9.1f job-s' % (k, s['attempts'][k], s['job_seconds'].get(k, 0.0)))
    for r in s['near_limit']: print('  near-limit %-40s %-5s limit %3d  %6.2fs  margin %.3f' % (r['suite'], r['kind'], r['limit'], r['seconds'], r['margin']))
    if 'reconcile' in s: print('  reconcile', json.dumps(s['reconcile']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
