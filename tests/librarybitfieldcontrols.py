#!/usr/bin/env python3
"""Independent true-C carrier controls; not model/public bitfield qualification.
Each ISA is a separate <=55s invocation. Qualified provider required.
"""
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'exec/c'))
from buildlibrary import provider_flags

CASES = {'b4', 'u4', 'b8', 'cross8', 'u8', 'b16', 'bd16', 'db16',
         'u16', 'a16', 'ad16', 'uf16', 'bf8', 'cross16'}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--target', choices=['osx/arm64', 'osx/x86_64'], required=True)
    ap.add_argument('--provider', type=Path, required=True)
    ap.add_argument('--output', type=Path)
    a = ap.parse_args()
    includes, links, provider = provider_flags(a.provider, a.target)
    arch = a.target.split('/')[1]
    fixture = ROOT / 'tests/libraryabi/bitfield-controls.c'
    records = []
    def command(argv):
        bounded = [sys.executable, str(ROOT / 'tests/bound.py'), '20', *argv]
        p = subprocess.run(bounded, capture_output=True, text=True, timeout=23)
        records.append({'command': bounded, 'rc': p.returncode,
                        'stdout': p.stdout, 'stderr': p.stderr})
        if p.returncode:
            raise RuntimeError(json.dumps(records[-1]))
        return p.stdout
    with tempfile.TemporaryDirectory(prefix='r10-bitfield-controls-') as tmp:
        exe = str(Path(tmp) / 'controls')
        command(['cc', '-arch', arch, '-std=c99', '-O2',
                 '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                 *includes, str(fixture), *links, '-o', exe])
        raw = command(['arch', '-' + arch, exe])
    rows = [json.loads(line) for line in raw.splitlines()]
    assert len(rows) == 43, ('short/extra output', len(rows))
    assert rows[0]['bitfield_offsetof_used'] is False
    actual = [(r['case'], r['pressure']) for r in rows[1:]]
    assert len(set(actual)) == 42
    assert set(actual) == {(name, pressure) for name in CASES for pressure in range(3)}
    for i, row in enumerate(rows[1:], 1):
        assert row['rc'] == 0
        assert row['native_calls'] == i * 100 and row['closure_calls'] == i * 100
        assert row['size'] in (4, 8, 16) and row['alignment'] in (4, 8)
    result = {'schema': 1, 'status': 'passed', 'target': a.target,
              'scope': 'Independent declared-carrier ABI controls; no model/public support proof',
              'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
              'provider': provider, 'records': records,
              'variants': 42, 'native_calls': 4200, 'closure_calls': 4200,
              'padding': 'observed storage preservation, not a portable C return requirement'}
    if a.output:
        a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('status', 'target', 'scope', 'variants', 'native_calls', 'closure_calls')}))

if __name__ == '__main__':
    main()
