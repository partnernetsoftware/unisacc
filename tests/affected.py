#!/usr/bin/env python3
"""Read-only: which gate suites a set of changed paths can affect, by the gatedeps declarations.

usage: affected.py (--rev A..B | --rev C | PATH...)     (0.0.43 step 6: a recording tool, not a gate)
Declared suites: a path affects a suite if it is in its files, under its trees, a guard key, or
(executable_inputs) inside the product closure; family suites use the family's reviewed trees.
Undeclared gate jobs use the queue's global inputs, so they count as affected by any change.
Reports paths that no declaration claims (the risk: a missed suite), and never edits anything.
"""
import json, pathlib, subprocess, sys

import os
R = pathlib.Path(os.environ.get("AFFECTED_ROOT") or pathlib.Path(__file__).resolve().parents[1])   # AFFECTED_ROOT: the replays point it at the checkout
PRODUCT = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/', 'seed/')
PRODUCT_FILES = ('unisacc.c',)

def in_tree(path, tree):
    tree = tree.rstrip('/')
    return path == tree or path.startswith(tree + '/')

def family_hit(fam, path):
    trees = fam.get('reviewed_trees') or {}
    for t in trees:
        if not in_tree(path, t):
            continue
        if path in (fam.get('files') or []):
            return True
        suffix = pathlib.PurePosixPath(path).suffix
        if t in (fam.get('all_files_trees') or []) or suffix in (fam.get('inventory_suffixes') or []):
            if not any(in_tree(path, x) for x in (fam.get('excluded_dirs') or [])):
                return True
    return path in (fam.get('guards') or {}) or path in (fam.get('files') or [])

def reasons(name, e, fams, path):
    out = []
    if path in (e.get('files') or []): out.append('files')
    if any(in_tree(path, t) for t in (e.get('trees') or [])): out.append('trees')
    if path in (e.get('guards') or {}): out.append('guard')
    if e.get('executable_inputs') and (path.startswith(PRODUCT) or path in PRODUCT_FILES): out.append('product')
    fam = fams.get(e.get('family')) if e.get('family') else None
    if fam and family_hit(fam, path): out.append('family')
    return out

def changed(args):
    if args and args[0] == '--rev':
        spec = args[1]
        if '..' in spec:
            a, b = spec.split('..', 1)
        else:
            a, b = spec + '^', spec
        return subprocess.run(['git', 'diff', '--name-only', a, b], cwd=R, capture_output=True, text=True, check=True).stdout.split()
    return args

def main(argv):
    paths = changed(argv)
    d = json.loads((R / 'tests/gatedeps.json').read_text())
    suites, fams = d['suites'], d['families']
    jobs = subprocess.run([str(R / 'tests/gate.sh'), '--list', '--com'], cwd=R, capture_output=True, text=True).stdout.split()
    undeclared = [j for j in jobs if j not in suites]
    affected, unclaimed = {}, []
    for p in paths:
        claimed = False
        for n, e in suites.items():
            why = reasons(n, e, fams, p)
            if why:
                claimed = True
                affected.setdefault(n, set()).update(why)
        if not claimed and not p.startswith(('.git/',)):
            unclaimed.append(p)
    # the queue's global inputs: every tracked or untracked file under these, for undeclared jobs
    GLOBAL = ('src', 'exec', 'tests', 'include', 'kernel', 'weights', 'unisa', 'examples', 'iterate', 'release', 'scripts', 'unisacc.c', 'README.md', 'ARCHITECTURE.md', 'AGENTS.md', 'Makefile')
    global_hit = any(p.split('/')[0] in GLOBAL or p in GLOBAL for p in paths)
    job_set = set(jobs)
    declared_jobs = {n for n in affected if n in job_set}
    undeclared_hit = set(undeclared) if (global_hit and paths) else set()
    print('changed paths: %d' % len(paths))
    print('gate jobs affected: %d of %d  (declared %d, undeclared %d)' % (
        len(declared_jobs | undeclared_hit), len(jobs), len(declared_jobs), len(undeclared_hit)))
    print('declared-only suites not in gate list (affected, informational): %d' % len({n for n in affected if n not in job_set}))
    print('paths no declaration claims: %d' % len(unclaimed))
    for p in unclaimed[:40]: print('  UNCLAIMED', p)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
