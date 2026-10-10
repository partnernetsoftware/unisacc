#!/usr/bin/env python3
"""draftfetch controls (0.0.39 R3=A): the identity check refuses another release's asset, a wrong tag or
commit, an already-published release and a wrongly named asset; a missing token and changed bytes fail
without any network fallback."""
import contextlib, importlib.util, io, os, pathlib, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('draftfetch_under_test', ROOT / 'release/tools/draftfetch.py')
D = importlib.util.module_from_spec(spec); spec.loader.exec_module(D)
R = {'id': 7, 'tag_name': 'v9', 'draft': True, 'target_commitish': 'abc', 'assets': [{'id': 11}, {'id': 12}]}
A = {'id': 11, 'name': 'unisacc.com'}
assert D.check(R, A, 'v9', 'abc') == []
for bad_r, bad_a, tag, commit in (
        (R, {'id': 99, 'name': 'unisacc.com'}, 'v9', 'abc'),       # asset of another release
        (R, A, 'v8', 'abc'),                                        # wrong tag
        (R, A, 'v9', 'def'),                                        # wrong commit
        ({**R, 'draft': False}, A, 'v9', 'abc'),                    # already public
        (R, {'id': 12, 'name': 'unisacc-unsigned.zip'}, 'v9', 'abc')):   # wrong asset name
    assert D.check(bad_r, bad_a, tag, commit), (bad_r, bad_a, tag, commit)
args = ['--repo', 'o/r', '--release-id', '7', '--asset-id', '11', '--tag', 'v9', '--commit', 'abc', '--sha256', '0' * 64]
env = os.environ.pop('GITHUB_TOKEN', None)
with tempfile.TemporaryDirectory() as d, contextlib.redirect_stdout(io.StringIO()):
    out = pathlib.Path(d) / 'u'
    assert D.main(args + ['--out', str(out)]) == 1                  # no token: refused, no anonymous/public fallback
    out.write_bytes(b'signed'); h = D.sha(out)
    assert D.main(args[:-1] + [h, '--out', str(out), '--verify-again']) == 0
    out.write_bytes(b'modified by the test')
    assert D.main(args[:-1] + [h, '--out', str(out), '--verify-again']) == 1   # bytes changed during the court
if env is not None: os.environ['GITHUB_TOKEN'] = env
print('draftfetch  identity: other-release asset/wrong tag/wrong commit/already public/wrong name refused; no token fails without fallback; changed bytes fail')
