#!/usr/bin/env python3
"""exittable: one release exit table from a queue state (0.0.38 P1).

  exittable.py STATE_DIR [--h1 archive/plans/v0.0.38.md] [--json]

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


class H1Error(Exception): pass


def h1_names(plan, item='H1'):
    text = pathlib.Path(plan).read_text()   # 0.0.40-prep: a missing H1 table is an error, never an empty set
    # 0.0.40 (机房主任 22:45 C): a missing or nameless row is unknown, never an empty list; empty is '| H1 | EMPTY |'
    rows = [l for l in text.splitlines() if l.startswith('| %s ' % item)]
    if len(rows) != 1: raise H1Error('%s: want exactly one "| %s |" row, found %d' % (plan, item, len(rows)))
    row = rows[0]
    if not (row.startswith('| %s |' % item) and row.rstrip().endswith('|')): raise H1Error('%s: malformed %s row (want "| %s | ... |", closing pipe included): %r' % (plan, item, item, row[:60]))
    cells = [c.strip() for c in row.split('|')[2:]]
    if 'EMPTY' in cells:   # an empty table is the whole row, never EMPTY beside names or other cells
        if [c for c in cells if c] != ['EMPTY']: raise H1Error('%s: EMPTY must stand alone in the %s row: %r' % (plan, item, row[:60]))
        return set()
    row = '|'.join(cells)
    names = set(re.findall(r'[a-z][a-z0-9]*(?:-[a-z0-9]+)+', row))
    for stem, lo, hi in re.findall(r'([a-z][a-z0-9-]*?)(\d+)\.\.(\d+)', row):     # closure-c1..4
        names |= {'%s%d' % (stem, k) for k in range(int(lo), int(hi) + 1)}
    for base in re.findall(r'([a-z][a-z0-9-]*[a-z0-9])\(\+package\)', row):        # selfelf(+package)
        names |= {base, base + '-package'}
    if not names: raise H1Error('%s: the %s row names no suite (write "| %s | EMPTY |" for an empty table)' % (plan, item, item))
    return names | {'exec-' + n for n in names if not n.startswith(('exec-', 'lib-'))}


def rulings(path):
    out = {}
    try: text = pathlib.Path(path).read_text()
    except FileNotFoundError: return out
    for line in text.splitlines():
        if line and not line.startswith('#'):
            suite, kind, sig, base, ref = (line.split('\t') + [''] * 5)[:5]
            out.setdefault(suite, []).append((kind, sig, base, ref))   # several rulings per suite add up
    return out


ERRLINE = re.compile(r'(?i)(^|[\s=])error:')   # a diagnostic line (compiler, sanitizer); words elsewhere don't count


def failure_lines(log):
    """The real failure: the first diagnostic `error:` line and the log's last line (the exception or
    verdict) -- not a word that happens to sit somewhere in its tail."""
    lines = [l for l in log.splitlines() if l.strip()]
    first = next((l for l in lines if ERRLINE.search(l)), '')
    return [first, lines[-1] if lines else '']


def ruled_match(name, r, log, rules):
    """The red is exactly what was ruled: same suite, same kind, same first-error signature."""
    got = ('INTERRUPTED' if r.get('status') == 'INTERRUPTED' or r.get('rc') is None else
           'TIMEOUT' if r.get('rc') == 142 else 'FAILED')       # its own limit is not an outside kill
    where = failure_lines(log)
    for kind, sig, base, ref in rules.get(name, []):
        if kind == got and (not sig or any(sig in l for l in where)): return base
    return None


def classify(name, r, log, h1, rules=None):
    if r.get('status') == 'UNVERIFIED' or r.get('rc') == 77: return 'UNVERIFIED_HOST'
    # 0.0.38: the resource admission refused to start (rc 2, its own UNKNOWN line as the last line):
    # missing evidence, not a product error -- any other rc 2 stays an ordinary failure
    lines = [l for l in log.splitlines() if l.strip()]
    if r.get('rc') == 2 and lines and lines[-1].startswith('UNKNOWN: required=memory'): return 'RESOURCE_UNKNOWN'
    if rules and ruled_match(name, r, log, rules): return 'RULED_BASELINE'   # exactly as ruled: still not PASS
    if r.get('status') == 'INTERRUPTED': return 'OUTER_INTERRUPTED'
    tail = '\n'.join(log.splitlines()[-5:])
    if TOOL.search(tail) and r.get('rc') not in (0, 142): return 'TOOL_MISSING'
    if r.get('rc') == 142 and name in h1:
        return 'HOST_TIMEOUT'
    return 'NEEDS_RULING'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('state'); ap.add_argument('--h1', default=str(ROOT / 'archive/plans/v0.0.38.md')); ap.add_argument('--json', action='store_true')
    ap.add_argument('--rulings', default=str(ROOT / 'release/rulings.tsv'))
    a = ap.parse_args(argv)
    if not pathlib.Path(a.h1).is_file():   # 0.0.40-prep (机房主任 18:33): fail closed before reading any state
        print('exittable: H1 table %s does not exist (archived plans live under archive/plans/)' % a.h1, file=sys.stderr); return 2
    st = pathlib.Path(a.state); data = json.loads((st / 'results.json').read_text())
    try: h1 = h1_names(a.h1)
    except H1Error as e: print('exittable: %s' % e, file=sys.stderr); return 2
    disp, rules = classes(), rulings(a.rulings)
    rows, passed = [], 0
    for name in data['jobs']:
        r = data['results'].get(name)
        if r is None: rows.append((name, 'PENDING', None, '')); continue
        if r.get('rc') == 0 and not r.get('status'): passed += 1; continue
        try: log = (st / (name + '.log')).read_text(errors='replace')
        except FileNotFoundError: log = ''
        # 0.0.38: a job never run because a predecessor failed is not its own red -- it hangs under its root
        if r.get('rc') == 1 and r.get('seconds') == 0 and r.get('limit') == 0:
            rows.append((name, 'BLOCKED', 1, 'not run: predecessor failed')); continue
        c = classify(name, r, log, h1, rules)
        last = (log.strip().splitlines() or [''])[-1][:120]
        rows.append((name, c, r.get('rc'), last))
    # root of each blocked job: follow the declared predecessors (gatequeue's PREDS) to a red that ran
    root_of = {}
    try:
        sys.path.insert(0, str(ROOT / 'tests')); import gatequeue as _gq
        preds = getattr(_gq, 'PREDS', {})
    except Exception: preds = {}
    ran_red = {n for n, k, _, _ in rows if k not in ('BLOCKED', 'PENDING')}
    for n, k, _, _ in rows:
        if k != 'BLOCKED': continue
        seen, todo, root = set(), list(preds.get(n, ())), None
        while todo:
            p = todo.pop(0)
            if p in seen: continue
            seen.add(p)
            if p in ran_red: root = p; break
            todo += list(preds.get(p, ()))
        root_of[n] = root or '(unknown root)'
    counts = {}
    for _, c, _, _ in rows: counts[c] = counts.get(c, 0) + 1
    if a.json:
        print(json.dumps({'jobs': len(data['jobs']), 'pass': passed, 'classes': counts,
                          'rows': [dict(suite=n, cls=c, rc=rc, last=l) for n, c, rc, l in rows]}, indent=2)); return 0
    print('exit table: %d suites, %d PASS; %s' % (len(data['jobs']), passed, ', '.join('%s %d' % kv for kv in sorted(counts.items()))))
    for c in ['NEEDS_RULING', 'RESOURCE_UNKNOWN', 'OUTER_INTERRUPTED', 'TOOL_MISSING', 'HOST_TIMEOUT', 'RULED_BASELINE', 'UNVERIFIED_HOST', 'PENDING']:
        sel = [r for r in rows if r[1] == c]
        if not sel: continue
        print('\n[%s] %s' % (c, disp.get(c, 'not yet run')))
        for n, _, rc, l in sel:
            if c == 'BLOCKED': continue
            under = [b for b, k, _, _ in rows if k == 'BLOCKED' and root_of.get(b) == n]
            print('  %-34s rc=%s  %s%s' % (n, rc, l, ('  [+%d not run downstream]' % len(under)) if under else ''))
    blocked = [r for r in rows if r[1] == 'BLOCKED']
    if blocked:
        print('\n[BLOCKED] %d obligations not executed because a predecessor failed (counted once under their root above; '
              'never PASS, no execution time)' % len(blocked))
    return 0


if __name__ == '__main__':
    try: sys.exit(main())
    except BrokenPipeError: sys.exit(0)
