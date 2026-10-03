#!/usr/bin/env python3
"""decision-ledger: build-time control written in Python instead of a table (0.0.23 I).

The shipped compiler runs networks plus a generic executor; the networks come from finite
transition systems that exec/*/ constructors assemble at build time, mostly from .tsv
declarations.  What remains as Python control is counted here, per stage: the call sites
that create transitions directly -- `.on(`, `.branch(`, `.goto(` -- and the graph edits (state-table
mutations: renames, hooks, aliases) outside finite_rules.py,
the generic declaration expander.  Lines are not counted (formatting would game them).
The counts may only fall: research/decision-ledger.json is the baseline; a rise fails unless
tests/decisionledger.allow has `rise STAGE reason`; every file still counted needs `keep FILE CATEGORY reason`.  `--update` rewrites the baseline
(only after a reviewed decrease, or with an allow line).
"""
import ast, json, pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = ROOT / 'research/decision-ledger.json'
ALLOW = ROOT / 'tests/decisionledger.allow'
CALLS = ('on', 'branch', 'goto', 'put')   # put: lex Delta.put writes a transition directly
# attributes of a graph object that hold transitions or states: any write to them is a graph edit
GRAPH_ATTRS = ('st', 'els', 'r', 'seqs', 'states', 'rows')

def count():
    stages = {}
    for p in sorted((ROOT / 'exec').rglob('*.py')):
        rel = p.relative_to(ROOT / 'exec')
        if rel.parts[0] in ('build', '__pycache__') or p.name == 'finite_rules.py' or '__pycache__' in rel.parts: continue
        stage = rel.parts[0] if len(rel.parts) > 1 else '(top)'
        tree = ast.parse(p.read_text(), str(p))
        n = sum(1 for node in ast.walk(tree)
                if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and node.func.attr in CALLS)
        # 0.0.23 K (owner): graph edits written in Python count too -- renames/hooks/aliases done by
        # mutating the state table (`X.st.pop(...)`, `X.st[...] = ...`, `del X.st[...]`)
        def is_st(e): return isinstance(e, ast.Attribute) and e.attr in GRAPH_ATTRS
        n += sum(1 for node in ast.walk(tree)
                 if (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and node.func.attr in ('pop', 'setdefault', 'update', 'append', 'extend', 'insert', 'clear') and is_st(node.func.value))
                 or (isinstance(node, (ast.Assign, ast.Delete)) and any(isinstance(t, ast.Subscript) and is_st(t.value) for t in node.targets))
                 or (isinstance(node, ast.Assign) and any(isinstance(t, ast.Subscript) and isinstance(t.value, ast.Subscript) and is_st(t.value.value) for t in node.targets))
                 or (isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Subscript) and is_st(node.target.value)))
        # edge rewrites inside a loop over the graph (`for ... in X.st.values(): row[key] = ...`)
        def walks_graph(it): return any(is_st(x) for x in ast.walk(it))
        for loop in [x for x in ast.walk(tree) if isinstance(x, ast.For) and walks_graph(x.iter)]:
            n += sum(1 for x in ast.walk(loop) if isinstance(x, ast.Assign)
                     and any(isinstance(t, ast.Subscript) and not is_st(t.value) for t in x.targets))
        if n: stages.setdefault(stage, {})[str(rel)] = n
    return {s: {'total': sum(v.values()), 'files': v} for s, v in sorted(stages.items())}

# 0.0.23 K2 seedpy: seed-layer Python is only the generic tsv->weights converter and the seed build.
SEED_WHITELIST = ('exec/finite_rules.py', 'exec/assemble.py', 'exec/facts/export.py', 'exec/facts/load.py',
                  'exec/c/', 'exec/pipeline/', 'exec/build/',
                  'exec/lex/tbl.py', 'exec/lex/net.py', 'exec/lex/stage.py')   # executors / converters
# check and reference-simulator tools: to move to the tests/ side, not counted
SEED_TOOLS = re.compile(r'(check|sim|compare|roundtrip|cut)\.py$')

def seedpy():
    counts, tools = {}, []
    for f in sorted(ROOT.joinpath('exec').rglob('*.py')):
        rel = f.relative_to(ROOT).as_posix()
        if rel.startswith(SEED_WHITELIST): continue
        if SEED_TOOLS.search(rel): tools.append(rel); continue
        d = rel.split('/')[1] if rel.count('/') > 1 else '.'
        counts[d] = counts.get(d, 0) + 1
        if '--list' in sys.argv: print('  stage    %s' % rel)
    for t in tools: print('  tool     %s' % t)
    for d, n in sorted(counts.items()): print('  %-10s %4d' % (d, n))
    print('seedpy  stage-specific .py under exec/ %d   (check/sim tools %d, not counted; report only)'
          % (sum(counts.values()), len(tools)))
    return 0

def main():
    if '--seedpy' in sys.argv: return seedpy()
    now = count()
    total = sum(v['total'] for v in now.values())
    if '--update' in sys.argv:
        BASE.write_text(json.dumps({'note': 'direct transition call sites in exec/*.py (outside finite_rules.py), per stage; may only fall', 'stages': now, 'total': total}, indent=1) + '\n')
        print('decision-ledger: baseline written, total %d' % total); return 0
    base = json.loads(BASE.read_text())['stages']
    lines = [l for l in (ALLOW.read_text().splitlines() if ALLOW.is_file() else []) if l.strip() and not l.startswith('#')]
    allow = {l.split()[1] for l in lines if l.split()[0] == 'rise'}
    keep = {l.split()[1]: l for l in lines if l.split()[0] == 'keep'}
    # every file that still writes the graph in Python must say why (0.0.23 K: a reason per remaining site)
    missing = sorted(f for v in now.values() for f in v['files'] if f not in keep)
    for f in missing: print('  NO REASON  %s (add `keep %s CATEGORY reason` to tests/decisionledger.allow)' % (f, f))
    bad = 0
    for s, v in now.items():
        was = base.get(s, {}).get('total', 0)
        mark = 'ok' if v['total'] <= was else ('allowed' if s in allow else 'ROSE')
        if mark == 'ROSE': bad += 1
        print('  %-10s %4d  (baseline %4d) %s' % (s, v['total'], was, mark))
    print('decision-ledger  stages %d   direct transition sites %d   (baseline %d)   rose %d'
          % (len(now), total, sum(v['total'] for v in base.values()), bad))
    return 1 if bad or missing else 0

if __name__ == '__main__':
    sys.exit(main())
