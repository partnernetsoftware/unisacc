#!/usr/bin/env python3
"""draftguard (0.0.39 R3): run release-smoke.yml's own first-step bash guard (taken from the workflow file, not a
copy) on every all/none/partial combination of release_id, asset_id, tag_commit and on bad shas: only all-empty, or
all-set with a 40-hex tag_commit, pass.  The Defender workflow's pwsh twin is not executed here (no pwsh)."""
import itertools, os, pathlib, subprocess, yaml
ROOT = pathlib.Path(__file__).resolve().parents[1]
wf = yaml.safe_load((ROOT / '.github/workflows/release-smoke.yml').read_text())
step = next(s for s in wf['jobs']['smoke']['steps'] if s.get('name') == 'draft inputs are all-or-none (R3=A)')
def ok(rid, aid, tc):
    env = dict(os.environ, RID=rid, AID=aid, TC=tc)
    return subprocess.run(['bash', '-eo', 'pipefail', '-c', step['run']], env=env, capture_output=True).returncode == 0
good = 'a' * 40
for rid, aid, tc in itertools.product(['', '7'], ['', '11'], ['', good]):
    want = (rid, aid, tc) in (('', '', ''), ('7', '11', good))
    assert ok(rid, aid, tc) == want, (rid, aid, tc)
for bad in ('abc', 'A' * 40, 'a' * 39, 'a' * 41, 'g' * 40):
    assert not ok('7', '11', bad), bad
print('draftguard  release-smoke guard: only all-empty or all-set with 40-hex tag_commit pass (8 combinations, 5 bad shas); pwsh twin not executed')
