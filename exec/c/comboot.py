#!/usr/bin/env python3
"""N22: the seed, the bootstrap fixed point, and the record of both.

    comboot.py stage N FILE     record FILE as stage N (its sha256, and the
                                seed it came from when N > 1)
    comboot.py cmp A B          the fixed point: A and B must be byte-equal
    comboot.py shard NAME       one bounded piece of the `comboot` gate
    comboot.py report           print the three sha256 values

What bootstrapping means here is one substitution.  `com` ends at

    python3 -m unisa ape exec/c/asmcompiler.c --via "$UA" ...

so stage 2 is that same build with `--via unisacc-seed.com`, and stage 3 is
the same again with `--via unisacc.com`.  Nothing about the six slices, the
pack or the provenance check changes, which is the point: if the fixed point
holds, the shipped compiler reproduces itself.

What this does NOT establish, and the distinction matters more than the
mechanism: the seed is still produced by a host-cc `UA`, so the trust root is
unchanged -- what changes is that the SHIPPED .com is built by our own
compiler.  `nativeboot` proves N1=N2=N3 for the classic image, a different
claim about a different artifact.
"""
import hashlib
import json
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / 'out' / 'comboot'
REC = OUT / 'stages.json'
SEED = pathlib.Path(os.environ.get('SEED_DIR', '/tmp/unisacc-seed-com')) / 'unisacc-seed.com'
STAGE2 = ROOT / 'unisacc.com'
STAGE3 = SEED.parent / 'stage3' / 'unisacc.com'


def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()


def load():
    if REC.exists():
        try:
            return json.loads(REC.read_text())
        except ValueError:
            pass
    return {}


def save(d):
    OUT.mkdir(parents=True, exist_ok=True)
    REC.write_text(json.dumps(d, indent=2, sort_keys=True) + '\n')


def stage(n, path):
    p = pathlib.Path(path)
    if not p.exists() or p.stat().st_size == 0:
        raise SystemExit('comboot: stage %d artifact missing: %s' % (n, path))
    d = load()
    d['stage%d' % n] = {'path': str(path), 'sha256': sha(p), 'bytes': p.stat().st_size}
    if n > 1:
        prev = d.get('stage%d' % (n - 1))
        if not prev:
            raise SystemExit('comboot: stage %d recorded before stage %d' % (n, n - 1))
        d['stage%d' % n]['built_by_sha256'] = prev['sha256']
    save(d)
    print('comboot stage %d  %s  sha256 %s' % (n, path, d['stage%d' % n]['sha256']))
    return 0


def cmp2(a, b):
    pa, pb = pathlib.Path(a), pathlib.Path(b)
    for p in (pa, pb):
        if not p.exists():
            raise SystemExit('comboot: missing %s' % p)
    if pa.read_bytes() != pb.read_bytes():
        # Not a warning: a fixed point that does not hold is a defect.  The
        # plan is explicit that a UA-built artifact may not be passed off as
        # the fixed point, so this exits non-zero and names both hashes.
        raise SystemExit(
            'comboot: NOT a fixed point\n  %s  %s\n  %s  %s\n'
            % (pa, sha(pa), pb, sha(pb)))
    print('comboot fixed point holds: %s == %s (sha256 %s)' % (a, b, sha(pa)))
    return 0


def run(cmd, secs=55):
    """One bounded step; `tests/bound.py` is the project's watchdog."""
    r = subprocess.run([sys.executable, str(ROOT / 'tests' / 'bound.py'), str(secs)] + cmd,
                       cwd=str(ROOT))
    return r.returncode


def shard(name):
    d = load()
    if name == 'seed':
        # stage 1 already exists if `make seed-com` ran; build it here so the
        # shard is self-contained, and record it either way
        if not SEED.exists():
            if run(['make', 'seed-com', 'COMB=0'], 55) != 0:
                raise SystemExit('comboot-shard seed: make seed-com failed')
        if not d.get('stage1') or d['stage1']['sha256'] != sha(SEED):
            stage(1, SEED)
        return 0
    if name == 'stage2':
        if not SEED.exists():
            raise SystemExit('comboot-shard stage2: run the seed shard first')
        if run(['make', 'com', 'COMB=1', 'COM_OUT=unisacc.com'], 55) != 0:
            raise SystemExit('comboot-shard stage2: make com COMB=1 failed')
        stage(2, STAGE2)
        # the seed the sidecar was built from must be the one on disk
        if d.get('stage2', {}).get('built_by_sha256') == d.get('stage1', {}).get('sha256'):
            print('comboot-shard stage2: sidecar seed sha256 matches stage 1')
        else:
            raise SystemExit('comboot-shard stage2: sidecar seed sha256 does not match stage 1')
        return 0
    if name == 'stage3':
        if not STAGE2.exists():
            raise SystemExit('comboot-shard stage3: run the stage2 shard first')
        if run(['make', 'com', 'COMB=1', 'COM_OUT=%s' % STAGE3], 55) != 0:
            raise SystemExit('comboot-shard stage3: build failed')
        stage(3, STAGE3)
        return 0
    if name == 'fixedpoint':
        return cmp2(str(STAGE2), str(STAGE3))
    raise SystemExit('comboot: unknown shard %r' % name)


def report():
    d = load()
    for k in ('stage1', 'stage2', 'stage3'):
        v = d.get(k)
        print('%-7s %s' % (k, v['sha256'] if v else '(not recorded)'))
    return 0


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    what = sys.argv[1]
    if what == 'stage':
        return stage(int(sys.argv[2]), sys.argv[3])
    if what == 'cmp':
        return cmp2(sys.argv[2], sys.argv[3])
    if what == 'shard':
        return shard(sys.argv[2])
    if what == 'report':
        return report()
    raise SystemExit('comboot: unknown command %r' % what)


if __name__ == '__main__':
    sys.exit(main() or 0)
