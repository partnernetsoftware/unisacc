#!/usr/bin/env python3
"""Rolling parallel gate, bounded windows, durable exact per-suite results.

Run through tests/term.sh. Exit 75 means pending work: invoke the SAME command
again. No test result is cached across an input change. Model preparation may
be shared separately. Each window is <=55 s, leaving cleanup inside the 60 s
outer watchdog. Long jobs go first; shorter jobs fill remaining slots.
"""
import argparse, fcntl, hashlib, json, os, pathlib, shutil, stat, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[1]

def atomic(path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, sort_keys=True) + '\n')
    tmp.replace(path)

def plan(com):
    out = subprocess.check_output(['sh', 'tests/gate.sh', '--plan'] + (['--com'] if com else []),
                                  env=dict(os.environ, TERM_SH='0'), timeout=10)
    words = out.decode().split('\0'); assert words.pop() == ''
    jobs = {}; i = 0
    while i < len(words):
        name, n = words[i:i+2]; n = int(n); i += 2
        assert n > 0 and name not in jobs and i + n <= len(words)
        jobs[name] = words[i:i+n]; i += n
    assert jobs, 'empty gate plan'
    return jobs

def execution_settings():
    return {k:os.environ[k] for k in ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA','CC','CFLAGS','TARGET','DRIVE','NETWORK',
                'EXEC_CC','PAR','STRICT','SHARD','CHAINKEEP','E3KEEP','E4STRICT') if k in os.environ}

def executable_inputs(settings):
    # These selectors are one quoted executable argument, never shell commands.
    # MODEL_COM is a file path; the other selectors also permit a PATH command.
    # Empty UA-family selectors retain their existing fallback semantics.
    inputs = {}
    for key in ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA'):
        if key not in settings or (not settings[key] and key != 'MODEL_COM'): continue
        value = settings[key]
        try:
            if not value: raise ValueError('empty candidate path')
            name = value if key == 'MODEL_COM' or '/' in value else shutil.which(value)
            if name is None: raise ValueError('executable name not found in PATH')
            path = pathlib.Path(name).resolve(strict=True)
            if not stat.S_ISREG(path.stat().st_mode) or not os.access(path, os.X_OK):
                raise ValueError('expected an executable regular file')
            with path.open('rb') as f:
                mode = os.fstat(f.fileno()).st_mode
                if not stat.S_ISREG(mode) or not os.access(path, os.X_OK):
                    raise ValueError('expected an executable regular file')
                h = hashlib.sha256()
                for chunk in iter(lambda: f.read(1024*1024), b''): h.update(chunk)
            inputs[key] = {'path':str(path), 'mode':mode, 'sha256':h.hexdigest()}
        except (OSError, ValueError) as error:
            raise SystemExit(f'queue {key}: {error}; use one executable path or wrapper, not a shell command')
    return inputs

def fingerprint(jobs):
    settings = execution_settings()
    h = hashlib.sha256(json.dumps([jobs, settings, executable_inputs(settings)], sort_keys=True).encode())
    raw = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard',
                                  '--', 'src', 'exec', 'tests', 'include', 'kernel', 'weights', 'unisa', 'examples',
                                  'unisacc.c', 'README.md', 'ARCHITECTURE.md', 'AGENTS.md',
                                  'prd.tree.md', 'prd.map.md', 'research/referee.tsv', 'iterate/kernel/typekw.tsv'])
    names = sorted(set(raw.decode().split('\0')) - {''})
    if pathlib.Path('unisacc.com').is_file(): names.append('unisacc.com')
    for name in names:
        p = pathlib.Path(name)
        h.update(name.encode() + b'\0' + str(p.stat().st_mode).encode())
        with p.open('rb') as f:
            for b in iter(lambda: f.read(1024*1024), b''): h.update(b)
    return h.hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--state', type=pathlib.Path, required=True)
    ap.add_argument('--com', action='store_true')
    ap.add_argument('--jobs', type=int, default=2)
    ap.add_argument('--window', type=int, default=55)
    ap.add_argument('--suite', action='append', default=[])
    ap.add_argument('--exclusive-suite', action='append', default=[],
                    help='selected suite that must run alone (repeatable; prioritized)')
    args = ap.parse_args()
    if not 1 <= args.jobs <= 4 or not 5 <= args.window <= 55: ap.error('jobs 1..4; window 5..55')
    os.chdir(ROOT); jobs = plan(args.com)
    if args.suite:
        if len(set(args.suite)) != len(args.suite) or set(args.suite) - jobs.keys(): ap.error('duplicate/unknown suite')
        jobs = {n: jobs[n] for n in args.suite}
    exclusive = set(args.exclusive_suite)
    if len(exclusive) != len(args.exclusive_suite) or exclusive - jobs.keys():
        ap.error('duplicate/unknown exclusive suite (must belong to selected jobs)')
    state = args.state.resolve(); state.mkdir(parents=True, exist_ok=True)
    lock = (state/'lock').open('a'); fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    stamp = fingerprint(jobs); path = state/'results.json'
    data = json.loads(path.read_text()) if path.is_file() else {'stamp':stamp, 'jobs':jobs, 'exclusive':sorted(exclusive), 'results':{}}
    if data['stamp'] != stamp or data['jobs'] != jobs or data.get('exclusive', []) != sorted(exclusive):
        raise SystemExit('queue input changed: use a new state directory; old results are not reused')
    atomic(path, data)
    # Classic and network drivers, and different concurrency, have different costs.
    profile = json.dumps([str(ROOT), execution_settings(), sorted(exclusive)], sort_keys=True).encode()
    histpath = pathlib.Path(os.environ.get('TMPDIR','/tmp')) / ('unisacc-gate-times-'+hashlib.sha256(profile).hexdigest()[:16]+'.json')
    history = json.loads(histpath.read_text()) if histpath.is_file() else {}
    pending = [n for n in jobs if n not in data['results']]
    active = {}; start = time.monotonic(); deadline = start + args.window
    def estimate(n): return min(args.window-2, max(2, history.get(n, 30)*1.3+1))
    try:
        while pending or active:
            left = deadline-time.monotonic()
            while pending and len(active) < args.jobs and left > 2:
                if exclusive.intersection(active): break
                fits = [n for n in pending if estimate(n) <= left-1]
                if not fits: break
                alone = [n for n in fits if n in exclusive]
                if alone and active: break  # drain ordinary work before the priority job
                n = max(alone or fits, key=estimate); pending.remove(n)
                limit = max(1, int(left)-1)
                log = (state/(n+'.log')).open('wb')
                p = subprocess.Popen(['perl','tests/bound.pl',str(limit),'env','PYTHONUNBUFFERED=1',*jobs[n]],stdout=log,stderr=subprocess.STDOUT)
                active[n] = (p, log, time.monotonic(), limit)
                print('START',n,'limit='+str(limit),flush=True)
                left = deadline-time.monotonic()
            for n, (p, log, t, limit) in list(active.items()):
                rc = p.poll()
                if rc is None: continue
                log.close(); elapsed = time.monotonic()-t
                if rc == 142 and limit < args.window-3:
                    # A late fill used only the window remainder, not a full
                    # attempt. Keep it pending and give it an early slot next.
                    pending.append(n)
                    data.setdefault('deferred', []).append({'name':n,'limit':limit})
                    history[n] = max(elapsed*2, history.get(n,0))
                else:
                    data['results'][n] = {'rc':rc, 'seconds':round(elapsed,3),'limit':limit}
                    history[n] = elapsed
                atomic(path,data); atomic(histpath,history)
                lines = (state/(n+'.log')).read_text(errors='replace').splitlines()
                print('DONE' if n in data['results'] else 'DEFER',n,'rc='+str(rc),'%.2fs'%elapsed,lines[-1] if lines else '(no output)',flush=True)
                del active[n]
            if not active and (not pending or not any(estimate(n) <= deadline-time.monotonic()-1 for n in pending)): break
            if time.monotonic() >= deadline: break
            time.sleep(.05)
    finally:
        # A signal/exception/window ending cannot leave tests behind.
        for p, log, _, _ in active.values(): p.terminate()
        for n, (p, log, t, limit) in active.items():
            p.wait(timeout=2); log.close()
            data['results'][n] = {'rc':142,'seconds':round(time.monotonic()-t,3),'limit':limit}
        atomic(path,data)
    if fingerprint(jobs) != stamp: raise SystemExit('inputs changed during queue: results invalid')
    bad = [n for n,r in data['results'].items() if r['rc'] != 0]
    missing = set(jobs)-data['results'].keys()
    print('queue: %d/%d completed, %d failed, %d pending, window %.2fs; logs %s' %
          (len(data['results']),len(jobs),len(bad),len(missing),time.monotonic()-start,state),flush=True)
    return 75 if missing else (1 if bad else 0)

if __name__ == '__main__':
    sys.exit(main())
