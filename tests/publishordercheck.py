#!/usr/bin/env python3
"""Publish only after the signed release asset has passed its hash check."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='unisacc-publish-order-') as directory:
    work = Path(directory)
    log = work / 'commands'
    bin_dir = work / 'bin'
    bin_dir.mkdir()
    (bin_dir / 'gh').write_text('''#!/usr/bin/env python3
import os,sys
from pathlib import Path
a=sys.argv[1:]
with Path(os.environ['MOCK_LOG']).open('a') as f:f.write('gh '+ ' '.join(a)+'\\n')
if a[:2]==['release','view']:
 q=a[-1]
 if q=='.assets[].name':print('unisacc.com\\nunsigned.zip\\nunisacc-macos-universal.dmg')
 elif 'select(' in q:print('https://example.invalid/unisacc.com')
 elif 'join(" ")' in q:print('unisacc.com unsigned.zip unisacc-macos-universal.dmg')
 elif 'join(", ")' in q:print('unisacc.com, unisacc-macos-universal.dmg')
if a[:2]==['release','download']:
 d=Path(a[a.index('-D')+1]); d.mkdir(parents=True,exist_ok=True); (d/'unisacc.com').write_bytes(b'signed asset')
''')
    (bin_dir / 'curl').write_text('''#!/usr/bin/env python3
import os,sys
from pathlib import Path
a=sys.argv[1:]
with Path(os.environ['MOCK_LOG']).open('a') as f:f.write('curl\\n')
Path(a[a.index('-o')+1]).write_bytes(b'signed asset')
''')
    for path in bin_dir.iterdir():
        path.chmod(0o755)
    env = dict(os.environ, PATH=str(bin_dir) + os.pathsep + os.environ['PATH'], MOCK_LOG=str(log))
    expected = hashlib.sha256(b'signed asset').hexdigest()

    def acceptance(eligible, signed):
        path = work / ('acc-%s-%s.json' % (eligible, signed[:4]))
        path.write_text('{"release_eligible": %s, "windows": {"after_sha256": "%s"}}' % (eligible, signed))
        return str(path)

    def run(want_hash, acc=None):
        log.write_text('')
        acc = acc or acceptance('true', want_hash)
        result = subprocess.run(['bash', str(ROOT / 'release/tools/publish.sh'), 'v-mock', want_hash, acc],
                                cwd=ROOT, env=env, capture_output=True, text=True, timeout=10)
        return result, log.read_text().splitlines()

    # 0.0.39 WF3 negative: not eligible, other bytes or no receipt -> refused before any gh call
    for bad in (acceptance('false', expected), acceptance('true', '1' * 64), str(work / 'missing.json')):
        result, rows = run(expected, bad)
        assert result.returncode != 0 and 'refused' in result.stdout, (bad, result.stdout)
        assert not rows, (bad, rows)
    result, rows = run('0' * 64)
    assert result.returncode != 0, result.stdout
    assert any('release download' in row for row in rows), rows   # 0.0.21: a draft asset is fetched with the authenticated client
    assert not any('delete-asset' in row or 'release edit' in row for row in rows), rows
    result, rows = run(expected)
    assert result.returncode == 0, (result.stdout, result.stderr)
    checked = next(i for i, row in enumerate(rows) if 'release download' in row)
    deleted = next(i for i, row in enumerate(rows) if 'delete-asset' in row)
    published = next(i for i, row in enumerate(rows) if 'release edit' in row)
    assert checked < deleted < published, rows
print('publish order: ineligible/other-bytes/missing acceptance touches nothing; wrong hash keeps draft and assets; correct hash verifies before removal and publication')
