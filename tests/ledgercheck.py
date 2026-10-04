#!/usr/bin/env python3
"""ledgercheck: before the version commit every plan row must say how it ends (0.0.25 P8).

0.0.25 sealed its candidate at 21:49 and recorded T2's second deferral at 21:53.  Since then precheck
runs this on plans/v<next version>.md: every row of the item tables must carry a status marker
〔...〕 whose text says 已完成, 顺延 or 跨版进行 (or 砍掉).  A row without one fails.
  tests/ledgercheck.py [PLAN]      default: the highest plans/v0.0.N.md
"""
import pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
def main():
    if len(sys.argv) > 1: plan = pathlib.Path(sys.argv[1])
    else:   # the plan being released: the highest plans/v0.0.N.md (precheck runs before the version commit)
        plans = sorted(ROOT.joinpath('plans').glob('v0.0.*.md'), key=lambda q: int(q.stem.split('.')[-1]))
        if not plans: print('ledgercheck: no version plan'); return 0
        plan = plans[-1]
    try: text = plan.read_text()
    except FileNotFoundError:
        print('ledgercheck: no %s (nothing to settle)' % plan.name); return 0
    bad = []
    for line in text.splitlines():
        if not line.startswith('| ') or line.startswith('| 项') or line.startswith('|---'): continue
        cells = line.split(' | ')
        if len(cells) < 3: continue
        m = re.search(r'〔([^〕]*)〕', line)
        if not m or not re.search(r'已完成|顺延|跨版进行|砍掉', m.group(1)): bad.append(cells[0].lstrip('| ').strip())
    for b in bad: print('  UNSETTLED %s' % b)
    print('ledgercheck  %s   unsettled rows %d' % (plan.name, len(bad)))
    return 1 if bad else 0
if __name__ == '__main__':
    sys.exit(main())
