#!/usr/bin/env python3
"""exittable: one release exit table from a queue state (0.0.38 P1).

  exittable.py STATE_DIR [--h1 plans/v0.0.38.md] [--json]

Sorts every non-PASS result of STATE_DIR/results.json into the pre-authorised classes of
release/preauth.tsv: UNVERIFIED_HOST (rc 77), OUTER_INTERRUPTED (status INTERRUPTED),
TOOL_MISSING (the log names a missing tool/headers), HOST_TIMEOUT (rc 142 and the suite is named
in the plan's H1 row), NEEDS_RULING (all else).  Pending suites are listed as PENDING.  PASS, the
counts and the classes stay separate: an accepted class is not a pass.  Read-only.
"""
import argparse, json, pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = re.compile(r'(not found|no such file or directory: .*(csmith|clang|zig|lld))', re.I)


def classes():
    out = {}
    for line in (ROOT / 'release/preauth.tsv').read_text().splitlines():
        if line and not line.startswith('#'):
            c, m, d = line.split('\t'); out[c] = d
    return out


def h1_names(plan, item='H1'):
    try: text = pathlib.Path(plan).read_text()
    except FileNotFoundError: return set()
    row = next((l for l in text.splitlines() if l.startswith('| %s ' % item)), '')
    names = set(re.findall(r'[a-z][a-z0-9]*(?:-[a-z0-9]+)+', row))
    for stem, lo, hi in re.findall(r'([a-z][a-z0-9-]*?)(\d+)\.\.(\d+)', row):     # closure-c1..4
        names |= {'%s%d' % (stem, k) for k in range(int(lo), int(hi) + 1)}
    for base in re.findall(r'([a-z][a-z0-9-]*[a-z0-9])\(\+package\)', row):        # selfelf(+package)
        names |= {base, base + '-package'}
    return names | {'exec-' + n for n in names if not n.startswith(('exec-', 'lib-'))}


def rulings(path):
    out = {}
    try: text = pathlib.Path(path).read_text()
    except FileNotFoundError: return out
    for line in text.splitlines():
        if line and not line.startswith('#'):
            suite, kind, sig, base, ref = (line.split('\t') + [''] * 5)[:5]
            out[suite] = (kind, sig, base, ref)
    return out


def ruled_match(name, r, log, rules):
    """The red is exactly what was ruled: same suite, same kind, same first-error signature."""
    if name not in rules: return None
    kind, sig, base, ref = rules[name]
    got = 'INTERRUPTED' if r.get('status') == 'INTERRUPTED' or r.get('rc') in (None, 142) else 'FAILED'
    if got != kind: return None
    if sig and sig not in log[-4000:]: return None
    return base


def classify(name, r, log, h1, rules=None):
    if r.get('status') == 'UNVERIFIED' or r.get('rc') == 77: return 'UNVERIFIED_HOST'
    if rules and ruled_match(name, r, log, rules): return 'RULED_BASELINE'   # exactly as ruled: still not PASS
    if r.get('status') == 'INTERRUPTED': return 'OUTER_INTERRUPTED'
    tail = '\n'.join(log.splitlines()[-5:])
    if TOOL.search(tail) and r.get('rc') not in (0, 142): return 'TOOL_MISSING'
    if r.get('rc') == 142 and name in h1:
        return 'HOST_TIMEOUT'
    return 'NEEDS_RULING'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('state'); ap.add_argument('--h1', default=str(ROOT / 'plans/v0.0.38.md')); ap.add_argument('--json', action='store_true')
    ap.add_argument('--rulings', default=str(ROOT / 'release/rulings.tsv'))
    a = ap.parse_args(argv)
    st = pathlib.Path(a.state); data = json.loads((st / 'results.json').read_text())
    disp, h1, rules = classes(), h1_names(a.h1), rulings(a.rulings)
    rows, passed = [], 0
    for name in data['jobs']:
        r = data['results'].get(name)
        if r is None: rows.append((name, 'PENDING', None, '')); continue
        if r.get('rc') == 0 and not r.get('status'): passed += 1; continue
        try: log = (st / (name + '.log')).read_text(errors='replace')
        except FileNotFoundError: log = ''
        c = classify(name, r, log, h1, rules)
        last = (log.strip().splitlines() or [''])[-1][:120]
        rows.append((name, c, r.get('rc'), last))
    counts = {}
    for _, c, _, _ in rows: counts[c] = counts.get(c, 0) + 1
    if a.json:
        print(json.dumps({'jobs': len(data['jobs']), 'pass': passed, 'classes': counts,
                          'rows': [dict(suite=n, cls=c, rc=rc, last=l) for n, c, rc, l in rows]}, indent=2)); return 0
    print('exit table: %d suites, %d PASS; %s' % (len(data['jobs']), passed, ', '.join('%s %d' % kv for kv in sorted(counts.items()))))
    for c in ['NEEDS_RULING', 'OUTER_INTERRUPTED', 'TOOL_MISSING', 'HOST_TIMEOUT', 'RULED_BASELINE', 'UNVERIFIED_HOST', 'PENDING']:
        sel = [r for r in rows if r[1] == c]
        if not sel: continue
        print('\n[%s] %s' % (c, disp.get(c, 'not yet run')))
        for n, _, rc, l in sel: print('  %-34s rc=%s  %s' % (n, rc, l))
    return 0


if __name__ == '__main__':
    try: sys.exit(main())
    except BrokenPipeError: sys.exit(0)
