#!/usr/bin/env python3
"""csmithdiff: fixed-seed Csmith programs, reference / product / system cc (0.0.22 TDD, T2).

Csmith writes UB-free C99 whose output is one checksum.  Each seed is compiled by cc (the
expected checksum) and run through the reference UA and, when MODEL_COM is set, the product.
A refusal by name ("not covered") is allowed and listed; a different checksum, a crash or a
hang where cc finished is a failure, unless the seed is listed in tests/csmithdiff.knownwrong
(one "seed compiler" per line, removed when fixed -- a listed seed that passes is red).
Seeds: SEEDS=a-b (default 1-40), split by SHARD=k/n.  Needs csmith (Homebrew) on PATH.
"""
import os, pathlib, shutil, subprocess, sys, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
BOUND = [sys.executable, str(ROOT / 'tests/bound.py')]
CSMITH = shutil.which('csmith') or sys.exit('csmithdiff: csmith not found (brew install csmith)')
INC = next(iter(sorted(pathlib.Path(CSMITH).resolve().parents[1].glob('include/csmith-*'))), None) or sys.exit('csmithdiff: csmith headers not found')
OPTS = ['--no-packed-struct', '--no-bitfields', '--no-volatiles', '--max-funcs', '4', '--max-block-depth', '3']

def run(cmd, timeout):
    r = subprocess.run(BOUND + [str(timeout)] + cmd, capture_output=True)
    return r.returncode, r.stdout.decode(errors='replace'), r.stderr.decode(errors='replace')

def launch(comp): return ['sh', comp] if comp.endswith('.com') else [comp]

ua = os.environ.get('UA')
if not ua:
    subprocess.run(['bash', '-c', '. "$0" && ua_ready', str(ROOT / 'tests/lib.sh')], cwd=ROOT, check=True, timeout=45)
    ua = '/tmp/ua_ref'
product = os.environ.get('MODEL_COM')
lo, hi = (int(x) for x in os.environ.get('SEEDS', '1-40').split('-'))
k, n = (int(x) for x in os.environ.get('SHARD', '1/1').split('/'))
known = set()
kw = ROOT / 'tests/csmithdiff.knownwrong'
if kw.is_file(): known = {tuple(l.split()[:2]) for l in kw.read_text().splitlines() if l.strip() and not l.startswith('#')}
ok = bad = refused = skipped = knownc = revived = 0
with tempfile.TemporaryDirectory(prefix='unisacc-csmith-') as td:
    d = pathlib.Path(td)
    for seed in range(lo, hi + 1)[k - 1::n]:
        src = d / ('s%d.c' % seed)
        rc, out, _ = run([CSMITH, '--seed', str(seed)] + OPTS, 20)
        if rc: print('csmithdiff FAIL generate', seed); bad += 1; continue
        src.write_text(out)
        rc, _, err = run(['cc', '-std=c99', '-w', '-I' + str(INC), '-o', str(d / 'ref'), str(src)], 30)
        if rc: print('csmithdiff FAIL cc', seed, err[:120]); bad += 1; continue
        want = run([str(d / 'ref')], 5)
        if want[0] != 0: skipped += 1; continue          # cc's build did not finish in 5 s: no verdict
        for label, comp in [('reference', ua)] + ([('product', product)] if product else []):
            got = run(launch(comp) + ['-I' + str(ROOT / 'include'), '-I' + str(INC), '-run', str(src)], 20)
            listed = (str(seed), label) in known
            if got[:2] == want[:2]:
                if listed: revived += 1; print('  REVIVED  seed %d %s (remove it from csmithdiff.knownwrong)' % (seed, label))
                else: ok += 1
            elif got[0] != 0 and 'not covered' in got[2] and not got[1]:
                refused += 1; print('  refused  seed %d %-9s %s' % (seed, label, got[2].strip().splitlines()[-3][:90]))
            elif listed: knownc += 1
            else: bad += 1; print('  FAIL     seed %d %-9s rc %d want %s got %s %s' % (seed, label, got[0], want[1].strip(), got[1].strip()[:40], got[2].strip()[:100]))
print('csmithdiff  ok %d   wrong %d   refused %d   known %d   revived %d   no-verdict %d   (seeds %d-%d shard %d/%d)' % (ok, bad, refused, knownc, revived, skipped, lo, hi, k, n))
sys.exit(1 if bad or revived or not (ok + refused + knownc) else 0)
