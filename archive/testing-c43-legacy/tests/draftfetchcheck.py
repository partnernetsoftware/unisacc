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
assert D.check(R, A, 'v9', 'abc', 7, 11) == []
assert D.check({**R, 'id': 8}, A, 'v9', 'abc', 7, 11)                       # API answered another release id
assert D.check(R, {'id': 12, 'name': 'unisacc.com'}, 'v9', 'abc', 7, 11)      # another asset of the same release
args = ['--repo', 'o/r', '--release-id', '7', '--asset-id', '11', '--tag', 'v9', '--commit', 'abc', '--sha256', '0' * 64]
env = os.environ.pop('GITHUB_TOKEN', None)
with tempfile.TemporaryDirectory() as d, contextlib.redirect_stdout(io.StringIO()):
    out = pathlib.Path(d) / 'u'
    assert D.main(args + ['--out', str(out)]) == 1                  # no token: refused, no anonymous/public fallback
    out.write_bytes(b'signed'); h = D.sha(out)
    assert D.main(args[:-1] + [h, '--out', str(out), '--verify-again']) == 0
    out.write_bytes(b'modified by the test')
    assert D.main(args[:-1] + [h, '--out', str(out), '--verify-again']) == 1   # bytes changed during the court
# call level, with a fake API (no network): API failure, download failure, wrong bytes, id mismatch -> rc 1;
# the token never appears in the output
import json as _j
TOKEN = 'ghs_SECRETTOKENVALUE'
def run_main(release, asset, body, fail=None):
    def fake(url, token, accept='application/vnd.github+json'):
        assert token == TOKEN
        if fail == 'api' and accept != 'application/octet-stream': raise OSError('down')
        if accept == 'application/octet-stream':
            if fail == 'download': raise OSError('down')
            return body
        return _j.dumps(release if '/releases/assets/' not in url else asset).encode()
    D.api = fake; os.environ['GITHUB_TOKEN'] = TOKEN
    with tempfile.TemporaryDirectory() as d:
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf): rc = D.main(['--repo', 'o/r', '--release-id', '7', '--asset-id', '11', '--tag', 'v9', '--commit', 'abc',
                                                           '--sha256', D.hashlib.sha256(b'signed').hexdigest(), '--out', str(pathlib.Path(d) / 'u')])
    assert TOKEN not in buf.getvalue(), buf.getvalue()
    return rc
assert run_main(R, A, b'signed') == 0
assert run_main(R, A, b'signed', 'api') == 1 and run_main(R, A, b'signed', 'download') == 1
class E404(Exception): code = 404
def fake404(url, token, accept='application/vnd.github+json'): raise E404()
D.api = fake404; os.environ['GITHUB_TOKEN'] = TOKEN; buf = io.StringIO()
with tempfile.TemporaryDirectory() as d, contextlib.redirect_stdout(buf):
    assert D.main(['--repo', 'o/r', '--release-id', '7', '--asset-id', '11', '--tag', 'v9', '--commit', 'abc', '--sha256', '0' * 64, '--out', str(pathlib.Path(d) / 'u')]) == 1
assert 'E404 404' in buf.getvalue() and TOKEN not in buf.getvalue(), buf.getvalue()   # the HTTP status is reported, the token never
assert run_main(R, A, b'other bytes') == 1
assert run_main({**R, 'id': 8}, A, b'signed') == 1
assert run_main(R, {'id': 12, 'name': 'unisacc.com'}, b'signed') == 1
os.environ.pop('GITHUB_TOKEN', None)
if env is not None: os.environ['GITHUB_TOKEN'] = env
print('draftfetch  requested ids bound; call level: API/download failure, wrong bytes, other release/asset id -> 1, token never printed; identity: other-release asset/wrong tag/wrong commit/already public/wrong name refused; no token fails without fallback; changed bytes fail')
