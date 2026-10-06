#!/usr/bin/env python3
"""List gate suites whose declared inputs (tests/gatedeps.json) cover the given paths.

  python3 tests/affected.py [PATH...]     default: paths changed vs HEAD (git diff + untracked)

Pre-merge check: run the printed suites before committing a change, instead of
waiting for the release queue to find it (0.0.30: a compiler.c change voided a
signed candidate).  Suites with no declared inputs depend on everything and are
not listed; the full queue still covers them.
"""
import json, subprocess, sys

def entry_paths(e):
    files = list(e.get('files', []))
    trees = []
    for k in ('trees', 'code_trees', 'all_files_trees', 'reviewed_trees'):
        trees += e.get(k, [])
    files += [c for c in e.get('command', []) if '/' in c and not c.startswith('-')]
    return [f.removeprefix('./') for f in files], [t.rstrip('/') + '/' for t in trees]

def covers(paths, e):
    files, trees = entry_paths(e)
    return any(p in files or any(p.startswith(t) for t in trees) for p in paths)

def main():
    paths = sys.argv[1:]
    if not paths:
        out = subprocess.run(['git', 'diff', '--name-only', 'HEAD'], capture_output=True, text=True).stdout
        out += subprocess.run(['git', 'ls-files', '--others', '--exclude-standard'], capture_output=True, text=True).stdout
        paths = sorted(set(out.split()))
    d = json.load(open('tests/gatedeps.json'))
    fam = d['families']
    hit = [n for n, e in d['suites'].items() if covers(paths, e) or covers(paths, fam.get(e.get('family'), {}))]
    for n in hit:
        print(n)
    print(f'affected {len(hit)} of {len(d["suites"])} suites for {len(paths)} paths', file=sys.stderr)
    return 0

if __name__ == '__main__':
    sys.exit(main())
