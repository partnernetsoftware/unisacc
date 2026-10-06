#!/usr/bin/env python3
"""N22: the seed, the bootstrap fixed point, and the record of both.

    comboot.py stage N FILE     record FILE as stage N (its sha256, and the
                                seed it came from when N > 1)
    comboot.py cmp A B          the fixed point: A and B must be byte-equal
    comboot.py shard NAME       one com-comboot gate job: seed | stage2 |
                                stage3 | fixedpoint, each doing ONLY its own
                                step and checking its prerequisites first
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

THE GATE DOES NOT BUILD ITS OWN INPUTS (ruling, cc-unisacc 2026-09-30).  These
four jobs used to build whatever they needed, which made "four job names" and
"every step inside the watchdog" impossible to hold at once: a first run in an
empty SEED_DIR cost 79 s against a 55 s bound.  Now the release flow builds the
seed before the queue (`make seed-com SEED_DIR=...`) and the gate only checks
what it is handed:

    seed        the seed exists, its sha256 matches its own build.json, and it
                runs a program
    stage2      the seed exists; build stage 2 FROM it, one bounded make a step
    stage3      stage 2 exists; build stage 3 from it the same way
    fixedpoint  both stages exist; compare them byte for byte

A shard whose prerequisite is absent prints ONE `skipped:` line and exits 0, so
an empty SEED_DIR is a green checklist rather than a failure -- the pipeline
that was supposed to produce the seed has simply not run yet.  `STRICT=1` turns
those skips into failures, for a caller that needs the whole bootstrap proven.
Every `make` stays on its own `tests/bound.py 55`.
"""
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / 'out' / 'comboot'
REC = OUT / 'stages.json'
SEED = pathlib.Path(os.environ.get('SEED_DIR', '/tmp/unisacc-seed-com')) / 'unisacc-seed.com'
STAGE2 = ROOT / 'unisacc.com'
# 0.0.30 R1: a shard stops at a step boundary once this many seconds have gone and exits 75
# (pending); the driver re-runs the same shard, and the step markers resume it.  The 0.0.29
# release instead had stage2/3 killed by the outer 55 s watchdog (rc=142) twice each.
T0 = time.monotonic()
BUDGET = float(os.environ.get('COMBOOT_BUDGET', '40'))
STAGE3 = SEED.parent / 'stage3' / 'unisacc.com'
# One buildcompiler step per bounded call: shared, the six targets, then pack.
# `all` in one invocation is what does NOT fit the watchdog.  Measured on the
# development machine: shared 17-18 s, each target 3-5 s, pack 37-39 s (after K2
# the model construction alone passed 55 s, so it is split into pack-prep-1..3,
# each filling the build's own model cache; pack-models then only packages).  Every
# step is individually wrapped by `run`, which is the invariant the ruling
# states; the sum is deliberately not what the watchdog is asked to hold.
STEP_LIST = ['shared', 'lnx/arm64', 'lnx/x86_64', 'osx/arm64', 'osx/x86_64',
             'win/arm64', 'win/x86_64', 'pack-prep-1', 'pack-prep-2', 'pack-prep-3',
             'pack-models', 'pack-driver']
COMB_BUILD = SEED.parent / 'comb-build'
HELLO = ROOT / 'examples' / 'hello.c'
# The one probe every seed must pass: a compiler that cannot run a program is
# not a seed, whatever its hash says.
SEED_PROBE = 'hello from C99'
# A caller that needs the bootstrap actually proven sets this and gets a
# failure instead of a skip.
STRICT = os.environ.get('STRICT', '0') == '1'


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


def capture(cmd, secs=55):
    """One bounded step; `tests/bound.py` is the project's watchdog.

    Returns (rc, tail) rather than just the code, because the callers here
    judge a compiler by what it prints as well as by what it exits with.
    """
    r = subprocess.run([sys.executable, str(ROOT / 'tests' / 'bound.py'), str(secs)] + cmd,
                       cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = r.stdout.decode('utf-8', 'replace')
    return r.returncode, text


def run(cmd, secs=55):
    return capture(cmd, secs)[0]


def shown(p):
    """A path for humans and logs: repository-relative when it is inside one.

    Absolute host paths in gate output put the machine's user name into every
    transcript and log, which is a leak rather than a detail.  The private
    seed directory lives outside the tree, so it keeps its own absolute form --
    that one is the caller's own input and carries no user name of ours.
    """
    try:
        return str(pathlib.Path(p).resolve().relative_to(ROOT))
    except ValueError:
        return str(p)


def skipped(what, how):
    """A prerequisite the gate is not allowed to build for itself.

    One line, and exit 0: the pipeline that owns that artifact has not run, and
    an unrun pipeline is not a defect in the compiler.  `STRICT=1` is for the
    caller who needs the opposite answer.
    """
    print('skipped: %s not built (%s)' % (what, how))
    return 1 if STRICT else 0


def step_name(st):
    """`lnx/arm64` is the build's name; `lnx-arm64` is its directory."""
    return st.replace('/', '-')


def source_digest():
    """One digest over everything the pipeline READS to make the model.

    It is `exec/c/provenance.py:source_digest` (models.closure plus the .S
    kernels) -- the value the product's build.json records as `sources_sha256` -- so a step marker and the
    candidate's build.json can never disagree about which sources count.  The
    earlier private list (exec/pipeline, exec/parse, exec/parse2, exec/pp,
    unisa, include, weights) left out exec/c, src and kernel: a change there
    kept the marker valid while buildcompiler.sh called the manifest stale
    (0.0.13, R14-2 (1)).
    """
    import importlib.util, sys
    for d in (ROOT / 'exec' / 'pipeline', ROOT / 'exec' / 'c'):
        if str(d) not in sys.path: sys.path.insert(0, str(d))
    spec = importlib.util.spec_from_file_location('unisacc_provenance', ROOT / 'exec' / 'c' / 'provenance.py')
    prov = importlib.util.module_from_spec(spec); spec.loader.exec_module(prov)
    return prov.source_digest()   # closure + exec/c/asm/*.S, exactly what build.json records


def step_manifest(model_dir, st):
    """The file that proves a step ran: each target step writes <step>/manifest.json;
    pack writes no such file -- its proof is the artifact's build.json sidecar."""
    if st in ('pack', 'pack-driver'):
        return model_dir / 'unisacc-next.com.build.json'
    if st.startswith('pack-prep-'):
        return model_dir / (st + '.done')
    if st == 'pack-models':
        return model_dir / 'compiler.pkg'
    return model_dir / step_name(st) / 'manifest.json'


def install(model_dir, dest):
    """Put the stage's artifact where the plan says it lives (ROOT/unisacc.com for
    stage 2, SEED_DIR/stage3/unisacc.com for stage 3), with its build.json sidecar,
    by tmp + rename so a reader sees the old file or the new one, never half."""
    src = model_dir / 'unisacc-next.com'
    if not src.exists():
        raise SystemExit('comboot: no %s after the build' % src)
    dest = pathlib.Path(dest); dest.parent.mkdir(parents=True, exist_ok=True)
    for a, b in ((src, dest), (pathlib.Path(str(src) + '.build.json'), pathlib.Path(str(dest) + '.build.json'))):
        tmp = pathlib.Path(str(b) + '.tmp.%d' % os.getpid())
        tmp.write_bytes(a.read_bytes()); tmp.chmod(0o755 if a is src else 0o644); os.replace(tmp, b)
    return dest


def step_done(model_dir, st):
    """Whether this step's artifacts are present AND from the current sources."""
    manifest = step_manifest(model_dir, st)
    marker = model_dir / ('step-' + st.replace('/', '_'))
    if not manifest.exists() or not marker.exists():
        return False
    try:
        return marker.read_text().strip() == '%s %s' % (source_digest(), sha(manifest))
    except (OSError, ValueError):
        return False


def mark_done(model_dir, st):
    manifest = step_manifest(model_dir, st)
    if not manifest.exists():
        raise SystemExit(
            'comboot: step %s reported success but wrote no %s; refusing to '
            'record it as done' % (st, manifest))
    m = model_dir / ('step-' + st.replace('/', '_'))
    m.parent.mkdir(parents=True, exist_ok=True)
    m.write_text('%s %s\n' % (source_digest(), sha(manifest)))


def build_step(st, model_dir):
    """ONE bounded buildcompiler step, and the only place `make` is called.

    The whole watchdog contract of this file lives here: every `make` gets its
    own `tests/bound.py 55`, so no single unit of work is ever unbounded, and
    the step markers make a re-run pick up where the last one stopped instead
    of starting over.
    """
    if step_done(model_dir, st):
        return 0
    cmd = ['make', 'model-com', 'MODEL_DIR=%s' % model_dir, 'MODEL_STEP=%s' % st]
    rc, text = capture(cmd, 55)
    if rc != 0:
        # Name the cause: three 0.0.13 failures read only "model step pack failed"
        # and each needed a hand re-run to see why (R14-2 (3)).
        tail = '\n'.join(text.rstrip().splitlines()[-20:])
        raise SystemExit('comboot: model step %s failed (rc=%s); last output:\n%s' % (st, rc, tail))
    mark_done(model_dir, st)
    return 0


def build_model(model_dir):
    """Every step of one bootstrapped stage, one bounded `make` each."""
    if model_dir.exists() and step_done(model_dir, STEP_LIST[-1]) \
            and (model_dir / 'unisacc-next.com').exists():
        return 0
    for st in STEP_LIST:
        if not step_done(model_dir, st) and time.monotonic() - T0 > BUDGET:
            print('comboot: %.0f s used, step %s pending; re-run the same shard (exit 75)'
                  % (time.monotonic() - T0, st))
            raise SystemExit(75)
        build_step(st, model_dir)
    return 0


def stage_dir(n):
    """Where stage N's build state lives: its OWN directory, never shared.

    stage 2 and stage 3 are the same build with a different `--via`, so giving
    them one MODEL_DIR would have both write the same `shared/*.net` files --
    whichever ran second would find a manifest that does not match what it
    needs.  Separate directories make each stage's step markers meaningful.
    """
    if n == 2:
        return COMB_BUILD / 'stage2'
    return COMB_BUILD / 'stage3'


def lock(path, name):
    """A mkdir lock: atomic on every filesystem the gate runs on.

    `mkdir` either creates the directory or fails, with no window between the
    two, so it is the whole primitive -- no lockfile format, no stale-lock
    timeout to get wrong.
    """
    lk = pathlib.Path(str(path) + '.lock')
    lk.parent.mkdir(parents=True, exist_ok=True)
    try:
        lk.mkdir()
        (lk / 'pid').write_text(str(os.getpid()))
        return lk
    except FileExistsError:
        # 0.0.24 X6: a watchdog kill skips `finally`, so the directory outlives its owner.  The
        # owner's pid is inside; when that process is gone the lock is stale and is taken over.
        try:
            owner = int((lk / 'pid').read_text())
            os.kill(owner, 0)
        except (FileNotFoundError, ValueError, ProcessLookupError):
            for f in lk.iterdir(): f.unlink()
            lk.rmdir()
            return lock(path, name)
        except PermissionError:
            pass
        raise SystemExit(
            'comboot: %s is being built by another run (%s); '
            'invoke this shard again once it is done' % (name, lk))


def seed_ready():
    """The seed's sidecar, its hash, and a program it has to run."""
    side = pathlib.Path(str(SEED) + '.build.json')
    if not side.exists():
        raise SystemExit(
            'comboot: %s has no build.json sidecar; rebuild the seed with '
            '`make seed-com SEED_DIR=%s`' % (SEED, SEED.parent))
    want = json.loads(side.read_text()).get('artifact_sha256')
    got = sha(SEED)
    if want != got:
        raise SystemExit(
            'comboot: seed sha256 %s does not match its sidecar %s (%s); '
            'the seed is not the artifact it claims to be' % (got, side, want))
    rc, text = capture(['/bin/sh', str(SEED), '-run', str(HELLO)], 55)
    # An APE is also a shell script, so /bin/sh is how it is started; exec'ing
    # the file directly is the one thing that does not work.
    if rc != 0 or SEED_PROBE not in text:
        raise SystemExit(
            'comboot: the seed did not run %s (rc=%d)\n%s'
            % (HELLO.name, rc, text.strip()))
    print('comboot seed ok  sha256 %s   runs %s' % (got, HELLO.name))
    return 0


def shard(name):
    """One com-comboot gate job: its OWN step, and nothing else's.

    The four names are the four the plan and README list, and they do not
    change.  What changed is that a shard no longer builds its prerequisites:
    it checks for them and says `skipped:` when they are absent, so an empty
    SEED_DIR is a green checklist instead of a 79 s build that overruns the
    watchdog.  The seed is the release flow's job.
    """
    if name == 'seed':
        if not SEED.exists():
            return skipped('seed', 'make seed-com SEED_DIR=%s' % SEED.parent)
        rc = seed_ready()
        d = load()
        have = d.get('stage1', {}).get('sha256')
        now = sha(SEED)
        if have != now:
            # Keep the record honest about which bytes are in play, without
            # printing the same line twice when the release flow already did.
            stage(1, SEED)
        return rc

    if name == 'stage2':
        if not SEED.exists():
            return skipped('stage2', 'the seed it is built from: '
                           'make seed-com SEED_DIR=%s' % SEED.parent)
        model_dir = stage_dir(2)
        lk = lock(model_dir, 'stage 2')
        try:
            os.environ['UA'] = str(SEED)          # stage 2 is the seed compiling the source
            build_model(model_dir)
            install(model_dir, STAGE2)
        finally:
            for f in lk.iterdir(): f.unlink()
            lk.rmdir()
        if not STAGE2.exists():
            raise SystemExit('comboot-shard stage2: no %s after the build' % STAGE2)
        stage(2, STAGE2)
        d = load()
        if d.get('stage2', {}).get('built_by_sha256') != d.get('stage1', {}).get('sha256'):
            raise SystemExit('comboot-shard stage2: sidecar seed sha256 does not match stage 1')
        print('comboot-shard stage2: sidecar seed sha256 matches stage 1')
        return 0

    if name == 'stage3':
        if not SEED.exists():
            return skipped('stage3', 'the seed it is built from: '
                           'make seed-com SEED_DIR=%s' % SEED.parent)
        if not STAGE2.exists():
            # stage 3 is built BY stage 2, so with stage 2 absent this shard has
            # nothing to do: say so and return 0.  Failing here instead would be
            # wrong -- a pipeline that has not run yet is not a defect.
            return skipped('stage3', 'stage 2 (%s): run the stage2 shard' % shown(STAGE2))
        # 0.0.24 X6: the main checkout's unisacc.com exists before stage 2 runs (it is the previous
        # release), so existence is not enough -- stage 2 must be the one this seed built.
        d = load()
        if d.get('stage2', {}).get('built_by_sha256') != d.get('stage1', {}).get('sha256'):
            return skipped('stage3', 'a stage 2 built by this seed (%s is not; run the stage2 shard)' % shown(STAGE2))
        model_dir = stage_dir(3)
        lk = lock(model_dir, 'stage 3')
        try:
            os.environ['UA'] = str(STAGE2)        # stage 3 is stage 2 compiling the same source
            build_model(model_dir)
            install(model_dir, STAGE3)
        finally:
            for f in lk.iterdir(): f.unlink()
            lk.rmdir()
        if not STAGE3.exists():
            raise SystemExit('comboot-shard stage3: no %s after the build' % STAGE3)
        stage(3, STAGE3)
        return 0

    if name == 'fixedpoint':
        if not STAGE2.exists() or not STAGE3.exists():
            return skipped('fixedpoint', 'stage 2 and stage 3 (%s, %s)' % (shown(STAGE2), shown(STAGE3)))
        return cmp2(str(STAGE2), str(STAGE3))

    raise SystemExit('comboot: unknown shard %r' % name)


def report():
    d = load()
    for k in ('stage1', 'stage2', 'stage3'):
        print('%-7s %s' % (k, d.get(k, {}).get('sha256', '(unrecorded)')))
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
