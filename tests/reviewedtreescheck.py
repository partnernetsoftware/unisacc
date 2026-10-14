#!/usr/bin/env python3
"""reviewed_trees check (0.0.43): the compilercheck family's reviewed-tree snapshots equal what its trees hash to.

The snapshot is derived from the tree contents (tests/refresh_gatedeps.py computes the same thing from a HEAD archive).
A commit that changes one of these trees must carry the refreshed snapshot, so the derived-consistency map routes
exec/, src/ and unisa/ changes here (release/derived-map.tsv).  This check is read-only and runs in the directory it
is given (the working tree by default, or a clean export): it recomputes each snapshot with the refresher's rule and
exits 1, naming the tree, when the declared value differs.  Exit 0 when every snapshot matches.
"""
import hashlib, json, os, pathlib, sys

R = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()

def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    return [os.stat(path).st_mode, h.hexdigest()]

def stamp(v): return hashlib.sha256(json.dumps(v, sort_keys=True).encode()).hexdigest()

def main():
    d = json.loads((R / 'tests/gatedeps.json').read_text())
    bad = []
    for fam, entry in d['families'].items():
        rt = entry.get('reviewed_trees') or {}
        for tree in rt:
            base = R
            paths = sorted(q for q in (base / tree).rglob('*') if q.is_file()
                           and (tree in entry['all_files_trees'] or q.suffix in entry['inventory_suffixes'])
                           and not any((base / x) == q or (base / x) in q.parents for x in entry.get('excluded_dirs', [])))
            s = stamp({str(q.relative_to(base)): digest(q) for q in paths})
            if rt[tree] != s: bad.append('%s/%s' % (fam, tree))
    if bad:
        print('reviewed_trees: stale snapshot(s), refresh with tests/refresh_gatedeps.py in the same commit: ' + ', '.join(bad))
        return 1
    print('reviewed_trees: every snapshot matches its tree (%d families)' % len(d['families']))
    return 0

if __name__ == '__main__':
    sys.exit(main())
