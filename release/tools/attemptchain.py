#!/usr/bin/env python3
"""attemptchain.py RELEASE_QUEUE_LOG [--results results.json] [--json] (0.0.39 WF5).

Offline replay of a queue log into per-suite attempt chains; never runs or resumes a queue.
- cumulative cost (job-seconds by kind and outcome) and the *current* pending set stay apart:
  a deferral keeps its cost after a later success, but the suite leaves the pending set;
- a START with no DONE/DEFER after it is UNKNOWN (never a synthesised rc or PASS);
- a DONE without a START is BLOCKED only when it says rc=1, 0.00s, predecessor failed; any other end without
  its START is UNSTARTED and keeps its observed seconds (an evidence gap, never a synthesised BLOCKED);
- --strict exits 1 on pending, unknown, evidence gaps or any reconcile difference;
- job-seconds are not wall clock; near-limit passes are listed per kind and limit.
With --results the replay is reconciled against the queue's results.json (suite count and final rc)."""
import argparse, collections, json, re, sys

START = re.compile(r'START (\S+) limit=(\d+)(?: kind=(\w+))?')        # pre-0.0.38 logs have no kind
END = re.compile(r'(DONE|DEFER) (\S+) rc=(\S+) ([\d.]+)s(.*)')


def replay(lines):
    chains, open_ = collections.defaultdict(list), {}
    for line in lines:
        m = START.match(line)
        if m:
            name, limit, kind = m.group(1), int(m.group(2)), m.group(3) or 'unknown'
            if name in open_:                       # a second START before an end: the first never ended
                chains[name].append({**open_.pop(name), 'outcome': 'UNKNOWN', 'rc': None, 'seconds': None})
            open_[name] = {'kind': kind, 'limit': limit}
            continue
        m = END.match(line)
        if m:
            ev, name, rc, secs, rest = m.group(1), m.group(2), m.group(3), float(m.group(4)), m.group(5)
            att = open_.pop(name, None)
            if att is None and ev == 'DONE' and rc == '1' and secs == 0.0 and 'predecessor' in rest:
                chains[name].append({'kind': 'blocked', 'limit': None, 'outcome': 'BLOCKED', 'rc': rc, 'seconds': 0.0})
            elif att is None:   # an end without its START: execution happened but the start was not logged
                chains[name].append({'kind': 'unstarted', 'limit': None, 'outcome': 'UNSTARTED-' + ev, 'rc': rc, 'seconds': secs})
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
        if last['outcome'] in ('DONE', 'BLOCKED', 'UNSTARTED-DONE'):
            final[name] = last['rc']
            if last['outcome'] == 'DONE' and last['rc'] == '0' and last['limit'] and last['limit'] - last['seconds'] < near:
                nearlimit.append({'suite': name, 'kind': last['kind'], 'limit': last['limit'], 'seconds': last['seconds'],
                                  'margin': round(last['limit'] - last['seconds'], 3)})
        elif last['outcome'] in ('DEFER', 'UNSTARTED-DEFER'):
            pending.append(name)
        else:
            unknown.append(name)
    hist = sorted(n for n, seq in chains.items() if any(a['outcome'] == 'UNKNOWN' or a['outcome'].startswith('UNSTARTED') for a in seq))
    return {'suites': len(chains), 'final': len(final), 'pending': sorted(pending), 'unknown': sorted(unknown),
            'evidence_gaps': hist,   # any UNKNOWN/UNSTARTED attempt in the history, even if the suite later finished
            'attempts': dict(count), 'job_seconds': {k: round(v, 1) for k, v in cost.items()},
            'near_limit': sorted(nearlimit, key=lambda r: r['margin']), 'final_rc': final}


def reconcile(summary, results):
    res = results.get('results', {})
    diff = [n for n, rc in summary['final_rc'].items() if n in res and str(res[n].get('rc')) != str(rc)]
    return {'results_suites': len(res), 'replayed_final': summary['final'],
            'missing_from_replay': sorted(set(res) - set(summary['final_rc'])),
            'extra_in_replay': sorted(set(summary['final_rc']) - set(res)), 'rc_mismatch': sorted(diff)}


def strict_rc(s):
    r = s.get('reconcile', {})
    bad = s['pending'] or s['unknown'] or s['evidence_gaps'] or r.get('missing_from_replay') or r.get('extra_in_replay') or r.get('rc_mismatch')
    return 1 if bad else 0


def main(argv=None):
    ap = argparse.ArgumentParser(); ap.add_argument('log'); ap.add_argument('--results'); ap.add_argument('--json', action='store_true')
    ap.add_argument('--strict', action='store_true', help='exit 1 on pending, unknown, evidence gaps or any reconcile difference')
    a = ap.parse_args(argv)
    s = summarise(replay(open(a.log).read().splitlines()))
    if a.results: s['reconcile'] = reconcile(s, json.load(open(a.results)))
    if a.json:
        print(json.dumps({k: v for k, v in s.items() if k != 'final_rc'}, indent=1)); return strict_rc(s) if a.strict else 0
    print('attemptchain  suites %d  final %d  pending %d  unknown %d  evidence-gap suites %d' % (s['suites'], s['final'], len(s['pending']), len(s['unknown']), len(s['evidence_gaps'])))
    for k in sorted(s['attempts']): print('  %-22s %4d attempts %9.1f job-s' % (k, s['attempts'][k], s['job_seconds'].get(k, 0.0)))
    for r in s['near_limit']: print('  near-limit %-40s %-5s limit %3d  %6.2fs  margin %.3f' % (r['suite'], r['kind'], r['limit'], r['seconds'], r['margin']))
    if 'reconcile' in s: print('  reconcile', json.dumps(s['reconcile']))
    return strict_rc(s) if a.strict else 0


if __name__ == '__main__':
    sys.exit(main())
