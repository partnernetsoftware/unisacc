#!/usr/bin/env python3
"""0.0.35 P11 regression: a scheduling-only edit to gatequeue.py keeps its contract digest; an
execution edit (outside SCHEDULING regions) changes it."""
import pathlib, sys, tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gatequeue as q
src = pathlib.Path(__file__).resolve().parent.joinpath('gatequeue.py').read_text()
base = q.contract_digest(str(pathlib.Path(__file__).resolve().parent/'gatequeue.py'))
def d(text):
    with tempfile.NamedTemporaryFile('w', suffix='.py', delete=False) as f: f.write(text)
    return q.contract_digest(f.name)
a = 'return v[len(v)//2] if len(v) >= 5 else 30'; assert src.count(a) == 1
b = "'python3','tests/bound.py',str(limit)"; assert src.count(b) == 1
ok = d(src) == base and d(src.replace(a, a.replace('30', '25'))) == base \
     and d(src.replace(b, b.replace('str(limit)', 'str(limit+1)'))) != base
print('gatequeue identity: scheduling edit kept, execution edit changed' if ok else 'gatequeue identity: FAIL')
sys.exit(0 if ok else 1)
