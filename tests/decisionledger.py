#!/usr/bin/env python3
"""decision-ledger: build-time control written in Python instead of a table (0.0.23 I).

The shipped compiler runs networks plus a generic executor; the networks come from finite
transition systems that exec/*/ constructors assemble at build time, mostly from .tsv
declarations.  What remains as Python control is counted here, per stage: the call sites
that create transitions directly -- `.on(`, `.branch(`, `.goto(` -- outside finite_rules.py,
the generic declaration expander.  Lines are not counted (formatting would game them).
The counts may only fall: research/decision-ledger.json is the baseline; a rise fails unless
tests/decisionledger.allow names the stage with a reason.  `--update` rewrites the baseline
(only after a reviewed decrease, or with an allow line).
"""
import ast, json, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = ROOT / 'research/decision-ledger.json'
ALLOW = ROOT / 'tests/decisionledger.allow'
CALLS = ('on', 'branch', 'goto')

def count():
    stages = {}
    for p in sorted((ROOT / 'exec').rglob('*.py')):
        rel = p.relative_to(ROOT / 'exec')
        if rel.parts[0] in ('build', '__pycache__') or p.name == 'finite_rules.py' or '__pycache__' in rel.parts: continue
        stage = rel.parts[0] if len(rel.parts) > 1 else '(top)'
        n = sum(1 for node in ast.walk(ast.parse(p.read_text(), str(p)))
                if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and node.func.attr in CALLS)
        if n: stages.setdefault(stage, {})[str(rel)] = n
    return {s: {'total': sum(v.values()), 'files': v} for s, v in sorted(stages.items())}

def main():
    now = count()
    total = sum(v['total'] for v in now.values())
    if '--update' in sys.argv:
        BASE.write_text(json.dumps({'note': 'direct transition call sites in exec/*.py (outside finite_rules.py), per stage; may only fall', 'stages': now, 'total': total}, indent=1) + '\n')
        print('decision-ledger: baseline written, total %d' % total); return 0
    base = json.loads(BASE.read_text())['stages']
    allow = {l.split()[0] for l in (ALLOW.read_text().splitlines() if ALLOW.is_file() else []) if l.strip() and not l.startswith('#')}
    bad = 0
    for s, v in now.items():
        was = base.get(s, {}).get('total', 0)
        mark = 'ok' if v['total'] <= was else ('allowed' if s in allow else 'ROSE')
        if mark == 'ROSE': bad += 1
        print('  %-10s %4d  (baseline %4d) %s' % (s, v['total'], was, mark))
    print('decision-ledger  stages %d   direct transition sites %d   (baseline %d)   rose %d'
          % (len(now), total, sum(v['total'] for v in base.values()), bad))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())
