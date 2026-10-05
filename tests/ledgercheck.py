#!/usr/bin/env python3
"""ledgercheck: before the version commit every plan row must say how it ends (0.0.25 P8).

0.0.25 sealed its candidate at 21:49 and recorded T2's second deferral at 21:53.  Since then precheck
runs this on plans/v<next version>.md: every row of the item tables must carry a status marker
〔...〕 whose text says 已完成, 顺延 or 跨版进行 (or 砍掉).  A row without one fails.
  tests/ledgercheck.py [--final] [PLAN]   default: plans/v0.0.<version.h + 1>.md; --final (precheck) rejects 本版做/进行中
"""
import pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
FINAL = r'已完成|顺延|跨版进行|砍掉'   # --final (precheck): '本版做' and '进行中' are not endings

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if args: plan = pathlib.Path(args[0])
    else:   # the plan being released: one past src/version.h (precheck runs before the version commit)
        ver = re.search(r'"0\.0\.(\d+)"', (ROOT / 'src/version.h').read_text()).group(1)
        plan = ROOT / 'plans' / ('v0.0.%d.md' % (int(ver) + 1))
    try: text = plan.read_text()
    except FileNotFoundError:
        print('ledgercheck: no %s (nothing to settle)' % plan.name); return 0
    bad = []
    for line in text.splitlines():
        if not line.startswith('| ') or line.startswith('| 项') or line.startswith('|---'): continue
        cells = line.split(' | ')
        if len(cells) < 3: continue
        if re.match(r'\| 0\.\d+\.\d+ ', line) or line.startswith('| 版本'): continue   # roadmap rows, not items
        m = re.search(r'〔([^〕]*)〕', line)
        if not m or not re.search(FINAL if '--final' in sys.argv else r'已完成|顺延|跨版进行|砍掉|本版做|进行中', m.group(1)): bad.append(cells[0].lstrip('| ').strip())
    for b in bad: print('  UNSETTLED %s' % b)
    print('ledgercheck  %s   unsettled rows %d' % (plan.name, len(bad)))
    return 1 if bad else 0
if __name__ == '__main__':
    sys.exit(main())
