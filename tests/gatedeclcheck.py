#!/usr/bin/env python3
"""Shared gate declaration semantics (tests/gatedecl.py) against the readings they replace.

1. The queue reading: effective_entry equals the inline family substitution gatequeue used, for every manifest suite
   (the fingerprint input is unchanged: sha256 over all effective entries is compared).
2. The selection reading: declared_reasons equals affected's former reasons(), path by path, on sample paths."""
import hashlib, json, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
import gatedecl

manifest = json.loads((ROOT / 'tests/gatedeps.json').read_text(encoding='utf-8'))
fails = 0
def check(cond, msg):
    global fails
    print(('ok   ' if cond else 'FAIL ') + msg)
    fails += 0 if cond else 1

old, new = {}, {}
for name in manifest.get('suites', {}):
    entry = manifest['suites'][name]
    if entry and 'family' in entry:   # the former inline substitution (gatequeue before tests/gatedecl.py)
        entry = dict(manifest['families'][entry['family']], command=entry['command'], family=entry['family'])
    old[name] = entry
    new[name] = gatedecl.effective_entry(manifest, name)
h = lambda d: hashlib.sha256(json.dumps(d, sort_keys=True).encode()).hexdigest()
check(h(old) == h(new), 'effective entries of all %d manifest suites are byte-identical to the former inline substitution' % len(old))

# the former affected reasons(): the same logic, kept here as the reference
PRODUCT = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/', 'seed/')
def former_reasons(e, fams, path):
    out = []
    if path in (e.get('files') or []): out.append('files')
    if any(gatedecl.in_tree(path, t) for t in (e.get('trees') or [])): out.append('trees')
    if path in (e.get('guards') or {}): out.append('guard')
    if e.get('executable_inputs') and (path.startswith(PRODUCT) or path in gatedecl.PRODUCT_FILES): out.append('product')
    fam = fams.get(e.get('family')) if e.get('family') else None
    if fam and gatedecl.family_hit(fam, path): out.append('family')
    return out
fams = manifest.get('families', {})
samples = ['exec/c/compiler.c', 'tests/gatedecl.py', 'src/version.h', 'unisacc.c', 'README.md', 'ujs/cloudflare', 'tests/gatedeps.json']
same = all(former_reasons(manifest['suites'][n], fams, p) == gatedecl.declared_reasons(manifest['suites'][n], fams, p)
           for n in list(manifest['suites'])[:200] for p in samples)
check(same, 'declared_reasons equals the former affected reasons on sample paths for 200 suites')
print('gatedecl check: PASS' if fails == 0 else 'gatedecl check: FAIL %d' % fails)
sys.exit(1 if fails else 0)
