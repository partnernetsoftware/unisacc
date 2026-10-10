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
            if att is None and ev == 'DONE' and rc == '1' and secs == 0.0 and re.match(r'\s*predecessor( \S+)+ failed\s*$', rest):   # gatequeue's exact BLOCKED line
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


def family(name):
    """Shard family: a trailing -N shard number dropped (csmithdiff-27 -> csmithdiff)."""
    return re.sub(r'-\d+$', '', name)


def tail_risk(chains):
    """WF2 (after the fact: reads the whole log, so siblings may have run later -- a hindsight risk hint, not
    evidence that was available before the tail was admitted): for each tail attempt, the successful seconds of *other* suites in its family next to the tail
    limit -- a risk hint only, never this suite's lower bound; successful tails are counted beside it."""
    ok = collections.defaultdict(list)
    for name, seq in chains.items():
        for a in seq:
            if a['outcome'] == 'DONE' and a['rc'] == '0': ok[family(name)].append((name, a['seconds']))
    rows, tail_ok, tail_defer = [], 0, 0
    for name, seq in chains.items():
        for a in seq:
            if a['kind'] != 'tail': continue
            if a['outcome'] == 'DONE' and a['rc'] == '0': tail_ok += 1
            if a['outcome'] != 'DEFER': continue
            tail_defer += 1
            sib = [s for n, s in ok[family(name)] if n != name]
            rows.append({'suite': name, 'limit': a['limit'], 'deferred_after': a['seconds'],
                         'siblings_ok': len(sib), 'sibling_max_s': max(sib) if sib else None,
                         'hint': 'UNKNOWN (no sibling evidence)' if not sib else
                                 'risk: a sibling needed more than this tail budget' if max(sib) > a['limit'] else 'siblings fit this tail budget'})
    return {'tail_success': tail_ok, 'tail_deferred': tail_defer, 'rows': rows}


def conserve(summary, exit_json):
    """WF5: rc-class conservation against exittable --json, which lists every non-PASS suite once and counts
    PASS: listed suites are never rc 0, unlisted finals are exactly the rc 0 ones, and PASS + listed = suites."""
    rows = exit_json.get('rows', []); names = [r['suite'] for r in rows]; fin = summary['final_rc']
    unlisted = set(fin) - set(names)
    out = {'suites': len(fin), 'listed': len(names), 'pass': exit_json.get('pass'),
           'duplicates': sorted({n for n in names if names.count(n) > 1}),
           'foreign': sorted(set(names) - set(fin)),
           'listed_rc0': sorted(n for n in names if fin.get(n) == '0'),          # a PASS hidden in a class
           'unlisted_not_rc0': sorted(n for n in unlisted if fin[n] != '0')}    # a red with no class
    hist = collections.Counter(r['cls'] for r in rows)
    out['rc_mismatch'] = sorted(r['suite'] for r in rows if r['suite'] in fin and str(r.get('rc')) != str(fin[r['suite']]))
    out['class_histogram_mismatch'] = {k: [hist.get(k, 0), v] for k, v in exit_json.get('classes', {}).items() if hist.get(k, 0) != v}
    out['class_histogram_mismatch'].update({k: [v, 0] for k, v in hist.items() if k not in exit_json.get('classes', {})})
    out['ok'] = (not (out['duplicates'] or out['foreign'] or out['listed_rc0'] or out['unlisted_not_rc0'] or out['rc_mismatch'] or out['class_histogram_mismatch'])
                 and exit_json.get('pass') == len(unlisted) and exit_json.get('jobs') == len(fin)
                 and sum(exit_json.get('classes', {}).values()) == len(names))
    return out


WINDOW = re.compile(r'window (\d+) rc=(\S+) ')


def windows(lines):
    """WF5: per queue window (closed by gatequeue's `window N rc=R hh:mm:ss` line) the attempts it ended, each
    kept as (suite, kind, outcome, rc): new finals, passes, deferrals, job-seconds.  Kept apart from wall clock."""
    out, cur, kinds, secs = [], [], {}, 0.0
    for line in lines:
        m = START.match(line)
        if m: kinds[m.group(1)] = m.group(3) or 'unknown'; continue
        m = WINDOW.match(line)
        if m:
            c = collections.Counter(a[2] for a in cur)
            out.append({'window': int(m.group(1)), 'rc': m.group(2), 'ends': len(cur), 'final': c['DONE'], 'deferred': c['DEFER'],
                        'passed': sum(1 for a in cur if a[2] == 'DONE' and a[3] == '0'), 'job_seconds': round(secs, 1), 'attempts': cur})
            cur, secs = [], 0.0
            continue
        m = END.match(line)
        if m:
            cur.append((m.group(2), kinds.pop(m.group(2), 'unstarted'), m.group(1), m.group(3))); secs += float(m.group(4))
    return out


def stage_windows(events, run):
    """Wall seconds of each window-N segment of RUN from a stagelog events.jsonl.  UNKNOWN (None, never
    inferred) unless: exactly one begin for that window and one end for that id, both carry the same
    non-empty boot_id, and end - begin is finite and >= 0.  stagelog end events carry no run/phase (schema),
    so the pairing identity is the id alone: an id begun by any other event, in any run, makes the window UNKNOWN."""
    import math
    begins, ends, per_window = collections.defaultdict(list), collections.defaultdict(list), collections.defaultdict(list)
    for line in events:
        d = json.loads(line)
        if d.get('event') == 'begin':
            begins[d['id']].append(d)    # every run: stagelog ends carry no run, so an id begun twice anywhere is ambiguous
            if d.get('run') == run and str(d.get('subphase') or '').startswith('window-'):
                per_window[int(d['subphase'].split('-')[1])].append(d['id'])
        elif d.get('event') == 'end':
            ends[d['id']].append(d)
    out = {}
    for n, ids in per_window.items():
        out[n] = None
        if len(ids) != 1 or len(begins[ids[0]]) != 1 or len(ends[ids[0]]) != 1: continue
        b, e = begins[ids[0]][0], ends[ids[0]][0]
        if not b.get('boot_id') or b.get('boot_id') != e.get('boot_id'): continue
        try: w = float(e['mono']) - float(b['mono'])
        except (KeyError, TypeError, ValueError): continue
        if math.isfinite(w) and w >= 0: out[n] = round(w, 2)
    return out


def join_windows(wins, walls, total_ends=None):
    rows = [{**w, 'wall_s': walls.get(w['window'])} for w in wins]
    nums = [r['window'] for r in rows]
    j = {'ends_in_windows': sum(r['ends'] for r in rows), 'windows': len(rows), 'stagelog_windows': len(walls),
         'duplicate_windows': sorted({n for n in nums if nums.count(n) > 1}),
         'stagelog_only': sorted(set(walls) - set(nums)), 'unknown_wall': sorted(r['window'] for r in rows if r['wall_s'] is None),
         'no_ends': [r['window'] for r in rows if not r['ends']],            # nothing ended in the window
         'no_done': [r['window'] for r in rows if not r['final']],           # no DONE line (not: no new valid final)
         'wall_s_sum': round(sum(r['wall_s'] or 0 for r in rows), 1), 'rows': rows}
    if total_ends is not None: j['ends_outside_windows'] = total_ends - j['ends_in_windows']
    j['ok'] = not (j['duplicate_windows'] or j['stagelog_only'] or j['unknown_wall'] or j.get('ends_outside_windows'))
    return j


def reconcile(summary, results):
    res = results.get('results', {})
    diff = [n for n, rc in summary['final_rc'].items() if n in res and str(res[n].get('rc')) != str(rc)]
    return {'results_suites': len(res), 'replayed_final': summary['final'],
            'missing_from_replay': sorted(set(res) - set(summary['final_rc'])),
            'extra_in_replay': sorted(set(summary['final_rc']) - set(res)), 'rc_mismatch': sorted(diff)}


def strict_rc(s):
    r = s.get('reconcile', {})
    bad = s['pending'] or s['unknown'] or s['evidence_gaps'] or r.get('missing_from_replay') or r.get('extra_in_replay') or r.get('rc_mismatch')
    if 'conserve' in s and not s['conserve']['ok']: bad = True
    return 1 if bad else 0


def main(argv=None):
    ap = argparse.ArgumentParser(); ap.add_argument('log'); ap.add_argument('--results'); ap.add_argument('--json', action='store_true')
    ap.add_argument('--exittable', help='WF5: exittable --json output to check rc-class conservation against')
    ap.add_argument('--stagelog', help='WF5: stagelog events.jsonl for the per-window wall-clock join')
    ap.add_argument('--run', help='stagelog run id (e.g. q-ba3f40cb4fba)')
    ap.add_argument('--tail-risk', action='store_true', help='WF2: sibling-shard risk hints for tail deferrals')
    ap.add_argument('--strict', action='store_true', help='exit 1 on pending, unknown, evidence gaps or any reconcile difference')
    a = ap.parse_args(argv)
    chains = replay(open(a.log).read().splitlines())
    if a.stagelog:
        if not a.run or a.tail_risk or a.strict or a.results or a.exittable: print('attemptchain: --stagelog needs --run and runs alone', file=sys.stderr); return 2
        total = sum(1 for l in open(a.log) if END.match(l))
        j = join_windows(windows(open(a.log).read().splitlines()), stage_windows(open(a.stagelog), a.run), total)
        if a.json: print(json.dumps(j, indent=1)); return 0 if j['ok'] else 1
        if j['ends_in_windows'] != total: print('attemptchain: %d ends outside any closed window (of %d) -- UNKNOWN window' % (total - j['ends_in_windows'], total))
        print('windows %d  stagelog windows %d  unknown wall %s  no ends %s  no DONE %d  window wall sum %.1fs (wall, not job-seconds)'
              % (j['windows'], j['stagelog_windows'], j['unknown_wall'] or '-', j['no_ends'] or '-', len(j['no_done']), j['wall_s_sum']))
        for r in j['rows']: print('  w%-4d rc=%-3s wall %7s  ends %3d  final %3d  passed %3d  deferred %3d  job %7.1fs'
              % (r['window'], r['rc'], r['wall_s'], r.get('ends', 0), r.get('final', 0), r.get('passed', 0), r.get('deferred', 0), r['job_seconds']))
        return 0 if j['ok'] else 1
    s = summarise(chains)
    if a.tail_risk and (a.strict or a.results or a.json or a.exittable):
        print('attemptchain: --tail-risk is a separate hint report; run --strict/--results/--json on their own', file=sys.stderr); return 2
    if a.tail_risk:
        t = tail_risk(chains)
        print('tail-risk  tail successes %d  tail deferrals %d  (hints only; sibling seconds are not this suite\'s lower bound)' % (t['tail_success'], t['tail_deferred']))
        for r in sorted(t['rows'], key=lambda r: r['suite']): print('  %-40s limit %3s  deferred after %6.2fs  siblings ok %2d  max %s  %s' % (r['suite'], r['limit'], r['deferred_after'], r['siblings_ok'], r['sibling_max_s'], r['hint']))
        return 0
    if a.results: s['reconcile'] = reconcile(s, json.load(open(a.results)))
    if a.exittable: s['conserve'] = conserve(s, json.load(open(a.exittable)))
    if a.json:
        print(json.dumps({k: v for k, v in s.items() if k != 'final_rc'}, indent=1)); return strict_rc(s) if a.strict else 0
    print('attemptchain  suites %d  final %d  pending %d  unknown %d  evidence-gap suites %d' % (s['suites'], s['final'], len(s['pending']), len(s['unknown']), len(s['evidence_gaps'])))
    for k in sorted(s['attempts']): print('  %-22s %4d attempts %9.1f job-s' % (k, s['attempts'][k], s['job_seconds'].get(k, 0.0)))
    for r in s['near_limit']: print('  near-limit %-40s %-5s limit %3d  %6.2fs  margin %.3f' % (r['suite'], r['kind'], r['limit'], r['seconds'], r['margin']))
    if 'reconcile' in s: print('  reconcile', json.dumps(s['reconcile']))
    if 'conserve' in s: print('  conserve', json.dumps(s['conserve']))
    return strict_rc(s) if a.strict else 0


if __name__ == '__main__':
    sys.exit(main())
