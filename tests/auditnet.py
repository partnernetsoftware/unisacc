#!/usr/bin/env python3
"""auditnet: the release gate re-checks every constructed network of the candidate (0.0.22 P1-C1).

The candidate build retains each distinct (table, network) pair it packed in
<candidate dir>/model-audit/ with models.json (exec/c/compilerpack.py retain_models).
Here each pair's hashes are re-verified and `run --check-net TABLE NET` enumerates the
full domain again; the count goes into the release receipt (r19-r21 had none).
Fails on an empty audit, a hash mismatch, or any check failure: nothing checked is red.
"""
import hashlib, json, os, pathlib, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
product = pathlib.Path(os.environ.get('MODEL_COM') or sys.exit('auditnet: MODEL_COM required'))
audit = pathlib.Path(os.environ.get('AUDIT_DIR', product.parent / 'model-audit'))
pairs = json.loads((audit / 'models.json').read_text())['pairs']
if not pairs: sys.exit('auditnet: empty audit')
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
bound = [sys.executable, str(ROOT / 'tests/bound.py')]
with tempfile.TemporaryDirectory(prefix='unisacc-auditnet-') as td:
    run = pathlib.Path(td) / 'run'
    subprocess.run(bound + ['50', os.environ.get('EXEC_CC', 'cc'), '-O2', '-o', str(run), str(ROOT / 'exec/c/run.c')], check=True)
    bad = 0
    for key, rec in sorted(pairs.items()):
        net, tbl = audit / (key + '.net'), audit / (key + '.tbl')
        if sha(net) != rec['network_sha256'] or sha(tbl) != rec['table_sha256']:
            print('auditnet FAIL hash', key[:16]); bad += 1; continue
        r = subprocess.run(bound + ['30', str(run), '--check-net', str(tbl), str(net)], capture_output=True)
        if r.returncode: print('auditnet FAIL check-net', key[:16], r.stderr.decode()[-200:]); bad += 1
print('auditnet  networks %d   checked %d   failed %d   (%s)' % (len(pairs), len(pairs) - bad, bad, audit))
sys.exit(1 if bad else 0)
