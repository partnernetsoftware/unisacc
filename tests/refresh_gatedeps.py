#!/usr/bin/env python3
"""Recompute the reviewed-tree stamps in tests/gatedeps.json from HEAD (R14-2 (4)).

gatequeue.py fingerprints each reviewed tree as sha256 over {path: [st_mode,
sha256]}.  Computing that in the shared checkout is wrong twice over: a
teammate's uncommitted file lands in the stamp, and so do the checkout's own
permission bits (0711/0600 from a teammate's umask, where git checks out
0755/0644) -- 0.0.13's gate-infra stayed red through two such "refreshes".
So the stamps are computed in a temporary extraction of HEAD (`git archive`),
which has exactly git's content and git's modes, and only the json is written
back.  Run it as the LAST commit before a release queue: any later exec/ or
src/ change makes the stamp stale again.   usage: make gatedeps
"""
import hashlib, io, json, os, pathlib, subprocess, sys, tarfile, tempfile
R = pathlib.Path(__file__).resolve().parents[1]

def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    return [os.stat(path).st_mode, h.hexdigest()]

def stamp(v): return hashlib.sha256(json.dumps(v, sort_keys=True).encode()).hexdigest()

def main():
    p = R / 'tests/gatedeps.json'; d = json.loads(p.read_text())
    trees = sorted({t for e in d['families'].values() for t in (e.get('reviewed_trees') or {})})
    with tempfile.TemporaryDirectory(prefix='unisacc-gatedeps-') as tmp:
        blob = subprocess.run(['git', '-c', 'tar.umask=022', 'archive', '--format=tar', 'HEAD'] + trees,   # git's default archive umask is 002 (0664 files); a checkout has 0644/0755
                           cwd=R, check=True, capture_output=True, timeout=60).stdout
        with tarfile.open(fileobj=io.BytesIO(blob)) as tf: tf.extractall(tmp, filter='fully_trusted')
        changed = []
        for fam, entry in d['families'].items():
            rt = entry.get('reviewed_trees')
            if not rt: continue
            for tree in rt:
                base = pathlib.Path(tmp)
                paths = sorted(q for q in (base / tree).rglob('*') if q.is_file()
                               and (tree in entry['all_files_trees'] or q.suffix in entry['inventory_suffixes'])
                               and not any((base / x) == q or (base / x) in q.parents for x in entry.get('excluded_dirs', [])))
                s = stamp({str(q.relative_to(base)): digest(q) for q in paths})
                if rt[tree] != s: changed.append((fam, tree)); rt[tree] = s
    p.write_text(json.dumps(d, indent=2) + '\n')
    dirty = subprocess.run(['git', 'status', '--porcelain', '--'] + trees, cwd=R, capture_output=True, text=True).stdout.strip()
    print('gatedeps: reviewed stamps from HEAD; changed %s' % (changed or 'none'))
    if dirty: print('gatedeps: note -- uncommitted changes under reviewed trees are NOT in the stamp:\n' + dirty)
    return 0

if __name__ == '__main__':
    sys.exit(main())
