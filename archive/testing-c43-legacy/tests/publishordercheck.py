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
    (bin_dir / 'gh').write_text(r'''#!/usr/bin/env python3
import os,sys
from pathlib import Path
a=sys.argv[1:]
with Path(os.environ['MOCK_LOG']).open('a') as f:f.write('gh '+ ' '.join(a)+'\n')
if a[:2]==['release','view']:
 q=a[-1]
 assets=os.environ.get('MOCK_ASSETS','unisacc.com unsigned.zip unisacc-macos-universal.dmg').split()
 if q=='.assets[].name':print('\n'.join(assets))
 elif 'select(' in q:print('https://example.invalid/unisacc.com')
 elif 'join(" ")' in q:print(' '.join(assets))
 elif 'join(", ")' in q:print(', '.join(assets))
if a[:2]==['release','download']:
 d=Path(a[a.index('-D')+1]); d.mkdir(parents=True,exist_ok=True); (d/'unisacc.com').write_bytes(b'signed asset')
''')
    (bin_dir / 'curl').write_text(r'''#!/usr/bin/env python3
import os,sys
from pathlib import Path
a=sys.argv[1:]
with Path(os.environ['MOCK_LOG']).open('a') as f:f.write('curl\n')
Path(a[a.index('-o')+1]).write_bytes(b'signed asset')
''')
    for path in bin_dir.iterdir():
        path.chmod(0o755)
    env = dict(os.environ, PATH=str(bin_dir) + os.pathsep + os.environ['PATH'], MOCK_LOG=str(log))
    expected = hashlib.sha256(b'signed asset').hexdigest()

    import json
    def acceptance(eligible, signed, **drop):
        rec = {'schema': 1, 'version': '-mock', 'release_eligible': eligible == 'true',
               'windows': {'after_sha256': signed},
               'published': {'public_assets': ['unisacc.com','unisacc-macos-universal.dmg']},
               'courts': {'six-native-cells-final-bytes': {'run_id': 1, 'conclusion': 'success', 'cells_public_sha256': signed, 'cells': 6},
                          'windows-defender-final-bytes': {'run_id': 2, 'conclusion': 'success', 'final_sha256': signed, 'cells': ['windows-latest', 'windows-11-arm']},
                          'owner-promotion': {'authority': 'mock owner'}}}
        for key, value in drop.items():
            if key == 'assets': rec['published']['public_assets'] = value
            elif key == 'version': rec['version'] = value
            elif key == 'pending': rec['pending'] = value
            else: rec['courts'][key.replace('_', '-')] = value
        path = work / ('acc-%d.json' % len(list(work.glob('acc-*.json'))))
        path.write_text(json.dumps(rec))
        return str(path)

    def run(want_hash, acc=None, assets=None):
        log.write_text('')
        acc = acc or acceptance('true', want_hash)
        result = subprocess.run(['bash', str(ROOT / 'release/tools/publish.sh'), 'v-mock', want_hash, acc],
                                cwd=ROOT, env=dict(env, **({'MOCK_ASSETS': assets} if assets is not None else {})), capture_output=True, text=True, timeout=10)
        return result, log.read_text().splitlines()

    # 0.0.39 WF3 negative: not eligible, other bytes or no receipt -> refused before any gh call
    minimal = work / 'minimal.json'; minimal.write_text('{"release_eligible": true, "windows": {"after_sha256": "%s"}}' % expected)
    for bad in (acceptance('false', expected), acceptance('true', '1' * 64), str(work / 'missing.json'), str(minimal),
                acceptance('true', expected, version='9.9.9'),
                acceptance('true', expected, windows_defender_final_bytes={'run_id': 2, 'conclusion': 'failure'}),
                acceptance('true', expected, six_native_cells_final_bytes={'run_id': 1, 'conclusion': 'success', 'cells_public_sha256': '2' * 64}),
                acceptance('true', expected, owner_promotion={}),
                acceptance('true', expected, pending=[{'item': 'gate-infra', 'state': 'PENDING'}]),
                acceptance('true', expected, six_native_cells_final_bytes={'run_id': 1, 'conclusion': 'success', 'cells': 6}),
                acceptance('true', expected, six_native_cells_final_bytes={'run_id': 1, 'conclusion': 'success', 'cells_public_sha256': expected, 'cells': 5}),
                acceptance('true', expected, windows_defender_final_bytes={'run_id': 2, 'conclusion': 'success', 'final_sha256': '3' * 64, 'cells': ['w']}),
                acceptance('true', expected, windows_defender_final_bytes={'run_id': 2, 'conclusion': 'success', 'cells': ['w']})):
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
    assert not any('delete-asset v-mock unisacc-macos-universal.dmg' in x for x in rows), rows
    for bad_assets in ([], ['other.txt'], ['unisacc.com', 'unisacc.com'], ['unisacc.com', '../bad'], 'unisacc.com', None):
        result, rows = run(expected, acceptance('true', expected, assets=bad_assets))
        assert result.returncode != 0 and not rows, (bad_assets, rows)
    com_only = acceptance('true', expected, assets=['unisacc.com'])
    result, rows = run(expected, com_only, assets='unisacc.com unsigned.zip')
    assert result.returncode == 0, (result.stdout, result.stderr)
    result, rows = run(expected, com_only, assets='unisacc.com unisacc-macos-universal.dmg unsigned.zip')
    assert result.returncode == 0 and any('delete-asset v-mock unisacc-macos-universal.dmg' in x for x in rows), rows
    result, rows = run(expected, acceptance('true', expected), assets='unisacc.com unsigned.zip')
    assert result.returncode != 0 and not any('release download' in x or 'delete-asset' in x or 'release edit' in x for x in rows), rows
    result, rows = run(expected, acceptance('true', expected, assets=['unisacc.com', 'notes.txt']), assets='unisacc.com notes.txt unsigned.zip')
    assert result.returncode == 0 and not any('delete-asset v-mock notes.txt' in x for x in rows), rows
    assert any('delete-asset v-mock unsigned.zip' in x for x in rows), rows
    result, rows = run(expected, com_only, assets='unsigned.zip')
    assert result.returncode != 0 and not any('release download' in x or 'delete-asset' in x or 'release edit' in x for x in rows), rows
print('publish order: R2 receipt-defined assets/no-DMG/missing-declared/malformed/preserve-listed;  ineligible/other-bytes/missing/minimal/wrong-tag/failed-court/other-court-bytes/unauthorised-promotion/pending-item/no-six-sha/five-cells/defender-other-or-no-sha acceptance touches nothing; wrong hash keeps draft and assets; correct hash verifies before removal and publication')
