#!/usr/bin/env python3
"""whyrerun.py snapshot OUT.json [--com] | explain A.json B.json [--json] (0.0.39 invalidation explainer).

Explains, offline, why a queue result could not be reused between two input states.  `snapshot` records
every suite's identity *by label* from tests/gatequeue.py's own fingerprint (the same parts that make its
stamp -- nothing is recomputed differently), keeping one digest per label and per key, never file bytes.
`explain` diffs two snapshots and gives each changed suite its reasons:

  product    a declared input under the product closure (exec/ unisa/ src/ kernel/ include/ weights/), the
             candidate/UA executables or their settings
  contract   the suite's own command, or a declared test/tool input (tests/, release/, ...)
  host       the shared environment (platform, interpreter, tools, gate runner) or the family toolchain
  (a suite gets every category that applies -- categories overlap and per-category counts do not add up)
  global     an unaudited suite: its identity holds every tracked file, so ANY change reruns it -- reported
             as UNKNOWN-cause, never narrowed (conservative, matching gatequeue's global fallback)
  location   the checkout root of a location-bound suite

It only explains; it never changes what gatequeue reuses."""
import argparse, hashlib, importlib.util, json, os, pathlib, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
PRODUCT = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/')
COMMON_KEYS = ('provider_inputs', 'seed_inputs', 'environment', 'platform', 'machine', 'python', 'python_path',
               'tools', 'queue_contract', 'gate_runner')


def h(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, default=str).encode()).hexdigest()[:16]


def flatten(label, value):
    """One digest per key for dict-valued parts (so changed files/keys can be named), else one digest."""
    if label == 'common' and isinstance(value, list):
        return {k: h(v) for k, v in zip(COMMON_KEYS, value)}
    if isinstance(value, dict):
        return {str(k): h(v) for k, v in value.items()}
    return h(value)


def snapshot(com):
    os.chdir(ROOT); sys.path.insert(0, str(ROOT / 'tests'))
    spec = importlib.util.spec_from_file_location('gatequeue', ROOT / 'tests/gatequeue.py')
    gq = importlib.util.module_from_spec(spec); spec.loader.exec_module(gq)
    jobs = gq.plan(com); parts = {}
    stamps = gq.fingerprint(jobs, parts)
    return {'schema': 1, 'com': com, 'stamps': stamps,
            'suites': {n: {'audited': p['audited'], **{k: flatten(k, v) for k, v in p.items() if k != 'audited'}}
                       for n, p in parts.items()}}


def classify_inputs(changed):
    return 'product' if any(f.startswith(PRODUCT) for f in changed) else 'contract'


def explain(a, b):
    rows, counts = {}, {}
    for name in sorted(set(a['suites']) | set(b['suites'])):
        x, y = a['suites'].get(name), b['suites'].get(name)
        if x is None or y is None:
            rows[name] = {'reasons': ['new suite' if x is None else 'removed suite'], 'category': ['contract']}
        elif a['stamps'].get(name) == b['stamps'].get(name):
            continue
        else:
            reasons, cats = [], set()
            for label in sorted(set(x) | set(y)):
                if label == 'audited' or x.get(label) == y.get(label): continue
                vx, vy = x.get(label), y.get(label)
                keys = sorted(k for k in set(vx or {}) | set(vy or {}) if (vx or {}).get(k) != (vy or {}).get(k)) \
                    if isinstance(vx, dict) or isinstance(vy, dict) else []
                if label == 'global_inputs':
                    cats.add('global'); reasons.append('global fallback (unaudited): %d tracked files changed, cause UNKNOWN' % len(keys))
                elif label == 'inputs':   # per file: product and test inputs both named when both changed
                    cats.update('product' if f.startswith(PRODUCT) else 'contract' for f in keys)
                    reasons.append('declared inputs: ' + ', '.join(keys[:5]) + (' ...' if len(keys) > 5 else ''))
                elif label in ('settings', 'executables'):
                    cats.add('product'); reasons.append('%s: %s' % (label, ', '.join(keys) or 'changed'))
                elif label == 'command':
                    cats.add('contract'); reasons.append('command changed')
                elif label == 'common':   # per key: the queue contract is contract, every other shared key is host
                    cats.update('contract' if k == 'queue_contract' else 'host' for k in keys); reasons.append('shared: ' + ', '.join(keys))
                elif label == 'inventory':   # reviewed trees: product when the tree is under the product closure
                    cats.add('product' if any(k.startswith(PRODUCT) or k.rstrip('/') + '/' in PRODUCT for k in keys) else 'contract')
                    reasons.append('reviewed trees: ' + ', '.join(keys))
                elif label == 'toolchain':
                    cats.add('host'); reasons.append('family toolchain changed')
                elif label == 'location':
                    cats.add('location'); reasons.append('checkout root changed')
                else:
                    cats.add('UNKNOWN'); reasons.append(label + ' changed')
            if x['audited'] != y['audited']:
                cats.add('global'); reasons.append('audit state %s -> %s' % (x['audited'], y['audited']))
            rows[name] = {'reasons': reasons or ['stamp differs, no labelled part differs: UNKNOWN'], 'category': sorted(cats) or ['UNKNOWN']}
        for c in rows[name]['category']: counts[c] = counts.get(c, 0) + 1
    return {'suites': len(set(a['suites']) | set(b['suites'])), 'rerun': len(rows), 'by_category': counts, 'rows': rows}


def main(argv=None):
    ap = argparse.ArgumentParser(); sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('snapshot'); s.add_argument('out'); s.add_argument('--com', action='store_true')
    e = sub.add_parser('explain'); e.add_argument('a'); e.add_argument('b'); e.add_argument('--json', action='store_true')
    a = ap.parse_args(argv)
    if a.cmd == 'snapshot':
        snap = snapshot(a.com); pathlib.Path(a.out).write_text(json.dumps(snap, sort_keys=True) + '\n')
        print('whyrerun snapshot  %d suites (%d audited)' % (len(snap['suites']), sum(v['audited'] for v in snap['suites'].values())))
        return 0
    r = explain(json.load(open(a.a)), json.load(open(a.b)))
    if a.json: print(json.dumps(r, indent=1)); return 0
    print('whyrerun  %d suites, %d not reusable  %s' % (r['suites'], r['rerun'], json.dumps(r['by_category'], sort_keys=True)))
    for n, row in r['rows'].items(): print('  %-40s %-18s %s' % (n, '/'.join(row['category']), '; '.join(row['reasons'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
