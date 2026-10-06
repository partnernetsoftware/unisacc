#!/usr/bin/env python3
"""freezecheck: after the version commit, only red fixes may touch the product closure (0.0.25 P1).

0.0.24 took kill(), ttyname() and a reference-side extern fix in the ten minutes before sealing; they
forced three seals, broke the first queue at 66/568 and voided the first Windows signing.  The rule
since then: once src/version.h carries the release number, a commit that changes the product closure
(exec/ unisa/ src/ kernel/ include/ weights/, as exec/pipeline/models.py:closure hashes them) must say
`fix:` at the start of its subject.  Anything else is a new feature and waits for the next version.
  tests/freezecheck.py [REPO]     exit 1 and name each offending commit

A red fix already pushed without the prefix (history is not rewritten once an rc tag points past it)
is waived by a line `SHA8<TAB>red suite it fixed` in release/freeze-waivers.tsv; the waiver is read
from HEAD, so it is reviewed like any commit.
"""
import subprocess, sys
CLOSURE = ('exec/', 'unisa/', 'src/', 'kernel/', 'include/', 'weights/', 'seed/')
EXT = ('.py', '.c', '.h', '.inc', '.tsv', '.json', '.sh')

def git(repo, *a):
    return subprocess.run(['git', '-C', repo, *a], capture_output=True, text=True, check=True).stdout

def product(path):
    if not path.startswith(CLOSURE) or not path.endswith(EXT): return False
    parts = path.split('/')
    if parts[0] == 'seed' and parts[-1] not in ('tbl.c', 'net.c', 'json.h', 'gen.c', 'facts.h', 'ident.c'): return False   # the seed tools the build runs (0.0.32 B5: gen.c)
    if parts[0] == 'exec' and path.endswith(('check.sh', 'check.py')): return False   # 0.0.32 X31: verifiers, not inputs
    return not (parts[:2] == ['exec', 'build'] and (len(parts) > 3 or not path.endswith('.py')))

def main(repo):
    v = git(repo, 'log', '-1', '--format=%H', '--', 'src/version.h').strip()
    if not v: print('freezecheck: no version commit'); return 0
    ver = __import__('re').search(r'"([0-9.]+)"', git(repo, 'show', 'HEAD:src/version.h')).group(1)
    if git(repo, 'tag', '-l', 'v' + ver).strip():
        print('freezecheck: v%s is already released; not in a freeze window' % ver); return 0
    try:
        waived = {l.split('\t')[0] for l in git(repo, 'show', 'HEAD:release/freeze-waivers.tsv').splitlines() if l and not l.startswith('#')}
    except subprocess.CalledProcessError:
        waived = set()
    bad = []
    for line in git(repo, 'log', '--format=%H %s', v + '..HEAD').splitlines():
        sha, subject = line.split(' ', 1)
        files = git(repo, 'diff-tree', '--no-commit-id', '--name-only', '-r', sha).split()
        hit = [f for f in files if product(f)]
        if hit and not subject.startswith('fix:') and sha[:8] not in waived:
            bad.append((sha[:8], subject[:70], hit[:3]))
    for sha, subject, hit in bad:
        print('  FREEZE %s %s  (touches %s)' % (sha, subject, ', '.join(hit)))
    print('freezecheck  version commit %s   closure commits without fix: %d' % (v[:8], len(bad)))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else '.'))
