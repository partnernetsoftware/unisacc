#!/usr/bin/env python3
"""ledgercheck: before the version commit every plan row must say how it ends (0.0.25 P8).

0.0.25 sealed its candidate at 21:49 and recorded T2's second deferral at 21:53.  Since then precheck
runs this on plans/v<next version>.md: every row of the item tables must carry a status marker
〔...〕 whose text says 已完成, 顺延 or 跨版进行 (or 砍掉).  A row without one fails.
  tests/ledgercheck.py [--final] [PLAN]   default: plans/v0.0.<version.h>.md until archived, then <version.h + 1>; --final (precheck) rejects 本版做/进行中
"""
import pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
FINAL = r'已完成|顺延|跨版进行|砍掉'   # --final (precheck): '本版做' and '进行中' are not endings

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if args: plan = pathlib.Path(args[0])
    else:   # the plan being released: src/version.h's own plan while it is not yet archived
        # (between the version commit and publishing), otherwise the next one (0.0.27 queue red)
        ver = int(re.search(r'"0\.0\.(\d+)"', (ROOT / 'src/version.h').read_text()).group(1))
        plan = ROOT / 'plans' / ('v0.0.%d.md' % ver)
        if not plan.is_file(): plan = ROOT / 'plans' / ('v0.0.%d.md' % (ver + 1))
    try: text = plan.read_text()
    except FileNotFoundError:
        print('ledgercheck: no %s (nothing to settle)' % plan.name); return 0
    bad = []
    items = False   # only tables headed '| 项 |' hold items (0.0.32: the waiver ledger is not one)
    for line in text.splitlines():
        if line.startswith('| 项'): items = True; continue
        if not line.startswith('|'): items = False; continue
        if not items or line.startswith('|---'): continue
        cells = line.split(' | ')
        if len(cells) < 3: continue
        if re.match(r'\| 0\.\d+\.\d+ ', line) or line.startswith('| 版本'): continue   # roadmap rows, not items
        m = re.search(r'〔([^〕]*)〕', line)
        if not m or not re.search(FINAL if '--final' in sys.argv else r'已完成|顺延|跨版进行|砍掉|本版做|进行中', m.group(1)): bad.append(cells[0].lstrip('| ').strip())
    for b in bad: print('  UNSETTLED %s' % b)
    print('ledgercheck  %s   unsettled rows %d' % (plan.name, len(bad)))
    carry = carry_check(plan)
    return 1 if bad or carry else 0

def norm(item):
    """Row id as plans write it: first word, ASCII primes folded (receipts say T3', plans T3′)."""
    return item.split()[0].replace("'''", '‴').replace("''", '″').replace("'", '′') if item.split() else ''

def plan_file(ver):
    for d in ('plans', 'archive/plans'):
        f = ROOT / d / ('v%s.md' % ver)
        if f.is_file(): return f
    return None

def rows(text):
    out, items = {}, False
    for line in text.splitlines():
        if line.startswith('| 项'): items = True; continue
        if not line.startswith('|'): items = False; continue
        if items and not line.startswith('|---'):
            out[line.split(' | ')[0].lstrip('| ').strip()] = line
    return out

def carry_check(plan):
    """0.0.32 (commissar 10-06): deferrals must land.  (a) every 顺延 target named in this plan or the
    previous one has a plan file; (b) every row the previous plan defers to this version, and every
    deferral in the previous release receipt, is a row here."""
    m = re.match(r'v0\.0\.(\d+)\.md$', plan.name)
    if not m: return 0
    n = int(m.group(1)); here = rows(plan.read_text()); bad = []
    prev = plan_file('0.0.%d' % (n - 1))
    for src in [plan] + ([prev] if prev else []):
        for rid, line in rows(src.read_text()).items():
            marks = re.findall(r'〔([^〕]*)〕', line)
            for t in re.findall(r'顺延[^〕]*?(?:→\s*|顺延\s+)(0\.\d+\.\d+)(?:\s+([A-Za-z][\w′″]*))?', ' '.join('〔%s〕' % k for k in marks[:1])):
                ver, renamed = t
                if plan_file(ver) is None: bad.append('%s: %s defers to v%s, which has no plan file' % (src.name, rid, ver))
                elif src is prev and ver == '0.0.%d' % n and (renamed or rid) not in here:
                    bad.append('%s defers %s to v%s, but %s has no row %s' % (src.name, rid, ver, plan.name, renamed or rid))
                elif src is prev and ver == '0.0.%d' % n:
                    # E42 (unisa coordinator 10-06): the count carried here must equal the count the
                    # previous plan's marker states (`顺延 3 → ...` lands as a row whose last cell is 3)
                    said = re.match(r'顺延\s*(\d+)', marks[0])
                    got = re.match(r'\s*(\d+)', here[renamed or rid].rstrip().rstrip('|').split(' | ')[-1])
                    if said and (not got or got.group(1) != said.group(1)):
                        bad.append('%s defers %s %s times, but %s row %s counts %s' % (src.name, rid, said.group(1), plan.name, renamed or rid, got.group(1) if got else 'nothing'))
    receipt = ROOT / 'research' / ('r%d-release-acceptance.json' % (n - 1))
    if receipt.is_file():
        import json
        for d in json.loads(receipt.read_text()).get('deferrals', []):
            if d.get('to') == '0.0.%d' % n and norm(d.get('item', '')) not in here:
                bad.append('%s defers %s to v0.0.%d, but %s has no row %s' % (receipt.name, d.get('item'), n, plan.name, norm(d.get('item', ''))))
    for b in bad: print('  CARRY %s' % b)
    print('ledgercheck  carry into %s   mismatches %d' % (plan.name, len(bad)))
    return len(bad)
if __name__ == '__main__':
    sys.exit(main())
