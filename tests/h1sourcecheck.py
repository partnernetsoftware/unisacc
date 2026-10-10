#!/usr/bin/env python3
"""h1sourcecheck: build_candidate settles its H1 table before its first write or make (0.0.40, 机房主任 22:45 A/C).

Scratch roots only; the real build_candidate.sh runs with ROOT=scratch and a stub tests/bound that counts calls
and stops at the first one, so a positive case reaches the stub and nothing is built."""
import hashlib, os, pathlib, shutil, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
BC = ROOT / 'release/tools/build_candidate.sh'
OLD = '| H1 | 超时：closure-c1..2、tools-2 | cc | x | 0 |\n'


def scratch(t, version, plan, old=OLD):
    r = t / version; (r / 'src').mkdir(parents=True); (r / 'plans').mkdir(); (r / 'archive/plans').mkdir(parents=True); (r / 'tests').mkdir()
    (r / 'src/version.h').write_text('#define UNISACC_VERSION "%s"\n' % version)
    (r / 'archive/plans/v0.0.38.md').write_text(old)
    if plan is not None: (r / ('plans/v%s.md' % version)).write_text(plan)
    stub = r / 'tests/bound'
    stub.write_text('#!/bin/sh\necho call >> "$(dirname "$0")/../bound.calls"\necho "stub bound $*"\nexit 1\n'); stub.chmod(0o755)
    return r


def run(r, script=BC):
    d = r / 'cand'
    p = subprocess.run(['bash', str(script), str(d), '/nonexistent-ua'], cwd=r, capture_output=True, text=True, timeout=30,
                       env=dict(os.environ, ROOT=str(r), SEED_C='0'))
    calls = len((r / 'bound.calls').read_text().splitlines()) if (r / 'bound.calls').exists() else 0
    return p.returncode, calls, d.exists(), p.stdout + p.stderr


def inherit(sha, path='archive/plans/v0.0.38.md'):
    return '# plan\nH1-INHERIT: %s sha256=%s host=linux-x86_64-8core-xeon-cloud\n' % (path, sha)


t = pathlib.Path(tempfile.mkdtemp(prefix='h1src.'))
try:
    good = hashlib.sha256(OLD.encode()).hexdigest()
    # positive first: the stub is reachable, after the h1source line (order), and the candidate dir only then exists
    for ver, plan, kind in (('0.0.40', inherit(good), 'INHERIT'), ('0.0.41', '| H1 | 超时：tools-2 | cc | x | 0 |\n', 'OWN'),
                            ('0.0.42', '| H1 | EMPTY |\n', 'EMPTY')):
        rc, calls, made, out = run(scratch(t, ver, plan))
        assert rc == 1 and calls == 1 and made, (ver, rc, calls, made, out)
        assert out.index('h1source: ' + kind) < out.index('step shared rc=1'), out   # the stub's output goes to the step log
    # negatives: rc 2, no make, no candidate directory
    bad = {
        'no declaration (version differs, nothing says which table)': ('0.0.40', '# plan without H1\n', OLD),
        'no plan for the version': ('0.0.40', None, OLD),
        'wrong sha': ('0.0.40', inherit('0' * 64), OLD),
        'missing target': ('0.0.40', inherit(good, 'archive/plans/v0.0.37.md'), OLD),
        'target without an H1 row': ('0.0.40', inherit(hashlib.sha256(b'# no row\n').hexdigest()), '# no row\n'),
        'malformed declaration': ('0.0.40', '# plan\nH1-INHERIT: archive/plans/v0.0.38.md\n', OLD),
        'own row naming no suite (not empty)': ('0.0.40', '| H1 | 无 | cc | x | 0 |\n', OLD),
        'own row without cell structure': ('0.0.40', '| H1 nonsense fake-suite\n', OLD),
        'EMPTY beside a suite name': ('0.0.40', '| H1 | EMPTY | fake-suite |\n', OLD),
        'own row and declaration': ('0.0.40', '| H1 | 超时：tools-2 |\n' + inherit(good), OLD),
    }
    for why, (ver, plan, old) in bad.items():
        u = t / 'neg'; shutil.rmtree(u, ignore_errors=True); u.mkdir()
        rc, calls, made, out = run(scratch(u, ver, plan, old))
        assert rc == 2 and calls == 0 and not made and 'h1source: REFUSED' in out, (why, rc, calls, made, out)
    # mutation: the same negative through a build_candidate without the h1source line reaches make -- the line is the gate
    m = t / 'mut.sh'; m.write_text(''.join(l for l in BC.read_text().splitlines(True) if 'h1source.py' not in l))
    u = t / 'mutroot'; u.mkdir()
    rc, calls, made, out = run(scratch(u, '0.0.40', '# plan without H1\n'), m)
    assert calls >= 1, ('mutant did not reach make: the negative proves nothing', rc, calls, out)
finally:
    shutil.rmtree(t, ignore_errors=True)
print('h1source  positives (inherit+sha, own row, explicit EMPTY) reach the make stub after the H1 line; no declaration/'
      'no plan/wrong sha/missing target/target without row/malformed/nameless row/structureless row/EMPTY beside a name/both refused rc 2 with 0 make calls and no '
      'candidate dir; without the gate line the same negative reaches make')
