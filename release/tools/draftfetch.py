#!/usr/bin/env python3
"""draftfetch.py --repo O/R --release-id N --asset-id N --tag T --commit SHA --sha256 H --out FILE (0.0.39 R3=A).

Fetch the signed unisacc.com from a still-DRAFT release by fixed ids, for courts that must run before
publishing.  Before any byte is used it checks, via the GitHub API with the workflow's read token:
the release id has this tag, is a draft, and targets the expected commit; the asset id belongs to that
release and is named unisacc.com.  The downloaded bytes must equal the signed sha256.  It never falls
back to the public URL, a cache or "latest"; it never uploads, deletes or publishes.  `--verify-again`
re-hashes the file after a test so a court cannot run on bytes it modified.  The token is read from
GITHUB_TOKEN and never printed."""
import argparse, hashlib, json, os, sys, urllib.request


def check(release, asset, tag, commit):
    """Pure identity check (testable offline): a list of reasons, empty when the draft is the one named."""
    why = []
    if release.get('tag_name') != tag: why.append('release %s has tag %r, not %r' % (release.get('id'), release.get('tag_name'), tag))
    if release.get('draft') is not True: why.append('release %s is not a draft (already published?)' % release.get('id'))
    if release.get('target_commitish') != commit: why.append('release targets %r, not %r' % (release.get('target_commitish'), commit))
    if asset.get('id') not in [a.get('id') for a in release.get('assets', [])]: why.append('asset %s does not belong to release %s' % (asset.get('id'), release.get('id')))
    if asset.get('name') != 'unisacc.com': why.append('asset %s is %r, not unisacc.com' % (asset.get('id'), asset.get('name')))
    return why


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def api(url, token, accept='application/vnd.github+json'):
    req = urllib.request.Request(url, headers={'Authorization': 'Bearer ' + token, 'Accept': accept, 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(req, timeout=60) as r: return r.read()


def main(argv=None):
    ap = argparse.ArgumentParser()
    for k in ('repo', 'release-id', 'asset-id', 'tag', 'commit', 'sha256', 'out'): ap.add_argument('--' + k, required=True)
    ap.add_argument('--verify-again', action='store_true', help='only re-hash --out against --sha256')
    a = ap.parse_args(argv)
    if a.verify_again:
        got = sha(a.out); print('draft bytes after the test %s' % got)
        if got != a.sha256: print('draftfetch: BYTES CHANGED during the test'); return 1
        return 0
    token = os.environ.get('GITHUB_TOKEN')
    if not token: print('draftfetch: GITHUB_TOKEN missing (a draft is not readable anonymously)'); return 1
    base = 'https://api.github.com/repos/%s' % a.repo
    try:
        release = json.loads(api('%s/releases/%s' % (base, a.release_id), token))
        asset = json.loads(api('%s/releases/assets/%s' % (base, a.asset_id), token))
    except Exception as e:
        print('draftfetch: API read failed: %s' % type(e).__name__); return 1
    why = check(release, asset, a.tag, a.commit)
    if why:
        for w in why: print('draftfetch: REFUSED ' + w)
        return 1
    try: data = api('%s/releases/assets/%s' % (base, a.asset_id), token, 'application/octet-stream')
    except Exception as e:
        print('draftfetch: download failed: %s' % type(e).__name__); return 1
    open(a.out, 'wb').write(data)
    got = sha(a.out)
    print('source=authenticated-draft release_id=%s asset_id=%s tag=%s commit=%s sha256 %s' % (a.release_id, a.asset_id, a.tag, a.commit, got))
    if got != a.sha256: print('draftfetch: DRAFT BYTES DIFFER from the signed hash'); return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
