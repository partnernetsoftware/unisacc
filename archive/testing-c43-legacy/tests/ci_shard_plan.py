#!/usr/bin/env python3
"""Check CI partitions without constructing or running any compiler."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def call(script, args, shard=None, good=True):
    env = dict(os.environ)
    if shard is not None:
        env['SHARD'] = shard
    else:
        env.pop('SHARD', None)
    p = subprocess.run(['bash', str(ROOT/script), *args], cwd=ROOT, env=env,
                       capture_output=True, text=True, timeout=5)
    if good:
        assert p.returncode == 0, (script, args, shard, p.returncode, p.stderr)
    else:
        assert p.returncode != 0, (script, args, shard, 'accepted invalid selection')
    return p.stdout.splitlines()

expected = (ROOT/'tests/tools.baseline.list').read_text().splitlines()
full = call('tests/tools.sh', ['--list'])
assert len(full) == 11 and len(set(full)) == 11 and sorted(full) == sorted(expected)
for n in (2, 4, 11):
    parts = [call('tests/tools.sh', ['--list'], f'{k}/{n}') for k in range(1, n+1)]
    flattened = [name for part in parts for name in part]
    assert all(parts) and sorted(flattened) == sorted(full)
    assert len(flattened) == len(set(flattened)), (n, 'overlapping shards')
for bad in ('', '1', '0/4', '5/4', '1/12', 'x/4', '1/2/3'):
    # Empty environment selects the documented default, not an invalid shard.
    if bad:
        call('tests/tools.sh', ['--list'], bad, good=False)
call('tests/tools.sh', ['--bad'], good=False)
targets = call('tests/bigclosure.sh', ['--list-targets'])
assert targets == ['lnx/x86_64', 'lnx/arm64', 'osx/x86_64', 'osx/arm64',
                   'win/x86_64', 'win/arm64']
assert len(set(targets)) == 6
for bad in ('', 'host', 'lnx/mips'):
    call('tests/bigclosure.sh', ['--target', bad], good=False)
call('tests/bigclosure.sh', ['--target'], good=False)
for script in ('tests/bigclosure.sh', 'tests/bootstrap.sh', 'tests/tools.sh'):
    subprocess.run(['bash', '-n', str(ROOT/script)], check=True, timeout=5)
print('CI shard plan: 11 tools exactly partitioned at 2/4/11; six targets; invalid selections rejected')
