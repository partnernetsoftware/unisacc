#!/usr/bin/env python3
"""Rolling parallel gate, bounded windows, durable exact per-suite results.

Run through tests/term.sh. Exit 75 means pending work: invoke the SAME command
again. Audited input closures permit per-suite reuse; unknown suites retain global
input invalidation. Model preparation may
be shared separately. Each window is <=55 s, leaving cleanup inside the 60 s
outer watchdog. Long jobs go first; shorter jobs fill remaining slots.
"""
import argparse, fcntl, hashlib, json, os, pathlib, platform, shutil, stat, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[1]
# comboot records its stages in ROOT/out/comboot/stages.json: a seed result from another
# checkout does not put stage 1 there (0.0.21: stage2/3 "recorded before stage 1")
LOCATION_BOUND = ('com-comboot-',)
# A job that consumes another's recorded state starts only after it passed (0.0.21 and 0.0.22:
# the longest-first pick ran stage2 before seed -- "stage 2 recorded before stage 1").
AFTER = {'com-comboot-stage2': 'com-comboot-seed', 'com-comboot-stage3': 'com-comboot-stage2',
         'com-comboot-fixedpoint': 'com-comboot-stage3'}

def atomic(path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, sort_keys=True) + '\n')
    tmp.replace(path)

def plan(com):
    out = subprocess.check_output(['sh', 'tests/gate.sh', '--plan'] + (['--com'] if com else []),
                                  env=dict(execution_environment(), TERM_SH='0'), timeout=10)
    words = out.decode().split('\0'); assert words.pop() == ''
    jobs = {}; i = 0
    while i < len(words):
        name, n = words[i:i+2]; n = int(n); i += 2
        assert n > 0 and name not in jobs and i + n <= len(words)
        jobs[name] = words[i:i+n]; i += n
    assert jobs, 'empty gate plan'
    return jobs

def execution_environment():
    # Terminal.app supplies a fresh transport identifier for each window.
    # Suites do not receive it either: reuse hashes the actual execution env.
    return {k:v for k,v in os.environ.items() if k != 'TERM_SESSION_ID'}

def execution_settings():
    return {k:os.environ[k] for k in ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA','CC','CFLAGS','TARGET','DRIVE','NETWORK',
                'EXEC_CC','PAR','STRICT','SHARD','CHAINKEEP','E3KEEP','E4STRICT',
                'UNISA_MAXSTEPS','UNISA_CONTAINER','UNISA_KERNEL','UNISACC_FFI_PROVIDER','UNISACC_FFI_X86_PROVIDER') if k in os.environ}

def executable_inputs(settings):
    # These selectors are one quoted executable argument, never shell commands.
    # MODEL_COM is a file path; the other selectors also permit a PATH command.
    # Empty UA-family selectors retain their existing fallback semantics.
    # External package/kernel selectors are file paths and need no execute bit.
    # Empty values stay in execution_settings, without inventing a file input.
    inputs = {}
    for key in ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA','UNISA_CONTAINER','UNISA_KERNEL'):
        executable = key not in ('UNISA_CONTAINER','UNISA_KERNEL')
        if key not in settings or (not settings[key] and key != 'MODEL_COM'): continue
        value = settings[key]
        try:
            if not value: raise ValueError('empty candidate path')
            name = value if not executable or key == 'MODEL_COM' or '/' in value else shutil.which(value)
            if name is None: raise ValueError('executable name not found in PATH')
            path = pathlib.Path(name).resolve(strict=True)
            expected = 'expected an executable regular file' if executable else 'expected a regular file'
            if not stat.S_ISREG(path.stat().st_mode) or (executable and not os.access(path, os.X_OK)):
                raise ValueError(expected)
            with path.open('rb') as f:
                mode = os.fstat(f.fileno()).st_mode
                if not stat.S_ISREG(mode) or (executable and not os.access(path, os.X_OK)):
                    raise ValueError(expected)
                h = hashlib.sha256()
                for chunk in iter(lambda: f.read(1024*1024), b''): h.update(chunk)
            inputs[key] = {'path':str(path), 'mode':mode, 'sha256':h.hexdigest()}
        except (OSError, ValueError) as error:
            hint = 'use one executable path or wrapper, not a shell command' if executable else 'use one regular-file path'
            raise SystemExit(f'queue {key}: {error}; {hint}')
    return inputs

def fingerprint(jobs):
    """One tree inventory and one hash per file, shared by all suite stamps.

    Declarations are audited code snapshots, not automatic dependency discovery.
    Changed code or an unrecognised command loses selective reuse. Missing
    declared files are identities too: a prior success never covers absence.
    """
    settings = execution_settings()
    executables = executable_inputs(settings)
    files = {}
    def digest(name):
        if name not in files:
            p = pathlib.Path(name)
            try:
                mode = p.stat().st_mode
                if not stat.S_ISREG(mode): raise SystemExit('queue nonregular input: '+name)
                h = hashlib.sha256()
                with p.open('rb') as f:
                    for block in iter(lambda:f.read(1024*1024), b''): h.update(block)
                files[name] = [mode, h.hexdigest()]
            except FileNotFoundError:
                files[name] = ['missing']
        return files[name]
    declaration = 'tests/gatedeps.json'
    manifest = json.loads(pathlib.Path(declaration).read_text()) if digest(declaration) != ['missing'] else {}
    assert not manifest or manifest['version'] == 1, 'unsupported gate dependency declaration'
    raw = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard',
                                  '--', 'src', 'exec', 'tests', 'include', 'kernel', 'weights', 'unisa', 'examples',
                                  'unisacc.c', 'README.md', 'ARCHITECTURE.md', 'AGENTS.md',
                                  'prd.tree.md', 'prd.map.md', 'research/referee.tsv', 'iterate', 'release', 'scripts', 'Makefile'])
    names = sorted(set(raw.decode().split('\0')) - {''})
    if digest('unisacc.com') != ['missing']: names.append('unisacc.com')
    tools = {}
    for name in ('python3', 'sh', 'bash', 'cc', 'openssl', 'grep', 'wc', 'tr', 'uname', 'id', 'env', '/bin/ps', '/usr/bin/openssl'):
        path = shutil.which(name)
        tools[name] = [path, digest(path)] if path else ['missing']
    for key in ('CC', 'EXEC_CC'):
        if settings.get(key):
            path = shutil.which(settings[key])
            tools[key] = [path, digest(path)] if path else ['missing', settings[key]]
    provider_inputs = {}
    for key in ('UNISACC_FFI_PROVIDER','UNISACC_FFI_X86_PROVIDER'):
        if settings.get(key):
            directory = pathlib.Path(settings[key]).resolve(strict=True)
            provider_inputs[key] = {str(directory/name):digest(str(directory/name)) for name in
                ('manifest.json','lib/libffi.a','include/ffi.h','include/ffitarget.h','include/ffi/ffi.h','include/ffi/ffitarget.h','LICENSE')}
    # 0.0.22: the checkout path is not an identity -- a queue continued in a new worktree of the
    # same content reuses its results (0.0.21: ~420/432 invalidated by the path alone).  Jobs that
    # keep state inside the checkout (LOCATION_BOUND) still carry it.
    common = [provider_inputs, execution_environment(), platform.platform(), platform.machine(), sys.version,
              str(pathlib.Path(sys.executable).resolve()), tools,
              {n:digest(n) for n in ('tests/gatequeue.py', 'tests/gate.sh', 'tests/bound.py', declaration)}]
    def stamp(value): return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()
    global_inputs = None
    inventories, family_tools = {}, {}
    result = {}
    for name, command in jobs.items():
        entry = manifest.get('suites', {}).get(name)
        if entry and 'family' in entry:
            entry = dict(manifest['families'][entry['family']], command=entry['command'], family=entry['family'])
        audited = entry is not None and command == entry['command']
        inventory, extra = {}, None
        if entry and entry.get('reviewed_trees'):
            for tree, reviewed in entry['reviewed_trees'].items():
                if tree not in inventories:
                    paths = sorted(p for p in pathlib.Path(tree).rglob('*') if p.is_file() and
                        (tree in entry['all_files_trees'] or p.suffix in entry['inventory_suffixes']) and
                        not any(pathlib.Path(q)==p or pathlib.Path(q) in p.parents for q in entry.get('excluded_dirs', [])))
                    if any(not stat.S_ISREG(p.lstat().st_mode) for p in paths): raise SystemExit('nonregular reviewed input')
                    inventories[tree] = stamp({str(p):digest(str(p)) for p in paths})
                inventory[tree] = inventories[tree]
            audited = audited and inventory == entry['reviewed_trees'] and all(settings.get(k) for k in entry['required_settings'])
            family = entry['family']
            if family not in family_tools:
                selected = {t:shutil.which(t) for t in entry['tools']}
                selected['actual-python'] = str(pathlib.Path(sys.executable).resolve())
                metadata = {}
                if sys.platform == 'darwin':
                    selected['xcrun'] = shutil.which('xcrun')
                    for flag in ('--show-sdk-path','--show-sdk-version','--show-sdk-build-version'):
                        metadata[flag] = subprocess.check_output(['xcrun',flag],timeout=5).decode().strip()
                    selected['sdk-settings'] = metadata['--show-sdk-path']+'/SDKSettings.json'
                    selected['sdk-clang'] = subprocess.check_output(['xcrun','--find','clang'],timeout=5).decode().strip()
                else:
                    for flag in ('-print-prog-name=ld','-print-prog-name=as','-print-file-name=libgcc.a'):
                        value = subprocess.check_output([tools['cc'][0],flag],timeout=5).decode().strip()
                        selected[flag] = value if '/' in value else shutil.which(value)
                    metadata['cc-version'] = subprocess.check_output([tools['cc'][0],'--version'],timeout=5).decode()
                family_tools[family] = [metadata,{t:[p,digest(p)] if p else ['missing'] for t,p in selected.items()}]
            extra = family_tools[family]
            audited = audited and all(extra[1][t][0] != 'missing' for t in entry['tools'])
        if audited:
            guards = entry['guards']
            audited = bool(guards) and all(digest(n)[-1] == sha for n,sha in guards.items())
            # Added Python modules must also lose the reviewed import closure.
            guarded = set(guards)
            audited = audited and all(n in guarded for n in names
                if n.endswith('.py') and any(n.startswith(prefix+'/') for prefix in entry.get('code_trees', [])))
        inputs = []
        if entry:
            inputs = sorted(set(entry['files']) | {n for n in names
                if any(n.startswith(prefix+'/') for prefix in entry.get('trees', []))})
        if audited:
            identity = [common, command, {n:digest(n) for n in inputs}]
            if entry.get('executable_inputs'): identity += [settings, executables, inventory, extra]
            if name.startswith(LOCATION_BOUND): identity.append(str(ROOT))
            result[name] = stamp(identity)
        else:
            if global_inputs is None: global_inputs = {n:digest(n) for n in names}
            # 0.0.23 E: the job's own command, not the whole job table -- adding or changing one
            # `job` line no longer invalidates every undeclared result (inputs stay global)
            identity = [common, command, settings, executables, global_inputs, {n:digest(n) for n in inputs}]
            if inventory: identity += [inventory, extra]
            if name.startswith(LOCATION_BOUND): identity.append(str(ROOT))
            result[name] = stamp(identity)
    return result

def resume(data, stamps, jobs, exclusive):
    # 0.0.21: a changed job list or exclusive set no longer forces a new state: results
    # survive only for jobs whose command and fingerprint are both unchanged; added jobs
    # are simply pending, removed ones are dropped
    if data['jobs'] != jobs or data.get('exclusive', []) != sorted(exclusive):
        oldjobs = data['jobs']
        changed = [n for n in list(data['results']) if n not in jobs or oldjobs.get(n) != jobs[n]]
        for n in changed: data['results'].pop(n, None)
        if changed: print('INVALIDATE', ','.join(changed), flush=True)
        data['jobs'] = jobs; data['exclusive'] = sorted(exclusive)
    old = data['stamp']
    if not isinstance(stamps, dict) or not isinstance(old, dict):
        if old != stamps: raise SystemExit('queue input changed: use a new state directory')
        return
    invalid = [n for n in jobs if n in data['results'] and old.get(n) != stamps[n]]
    for n in invalid: data['results'].pop(n, None)
    if invalid: print('INVALIDATE', ','.join(invalid), flush=True)
    data['stamp'] = stamps

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
    os.chdir(ROOT)
    if args.com:
        subprocess.run([sys.executable, str(ROOT/"exec/c/provenance.py"), "check",
                        os.environ.get("MODEL_COM", str(ROOT/"unisacc.com"))], check=True, timeout=10)
    jobs = plan(args.com)
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
    resume(data, stamp, jobs, exclusive)
    atomic(path, data)
    # Classic and network drivers, and different concurrency, have different costs.
    profile = json.dumps([execution_settings(), sorted(exclusive)], sort_keys=True).encode()
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
                for n in [n for n in pending if AFTER.get(n) in data['results'] and data['results'][AFTER[n]]['rc'] != 0]:
                    pending.remove(n); data['results'][n] = {'rc': 1, 'seconds': 0, 'limit': 0}
                    print('DONE', n, 'rc=1 0.00s predecessor', AFTER[n], 'failed', flush=True)
                fits = [n for n in pending if estimate(n) <= left-1
                        and (AFTER.get(n) not in jobs or AFTER[n] in data['results'])]
                if not fits: break
                alone = [n for n in fits if n in exclusive]
                if alone and active: break  # drain ordinary work before the priority job
                n = max(alone or fits, key=estimate); pending.remove(n)
                limit = max(1, int(left)-1)
                log = (state/(n+'.log')).open('wb')
                p = subprocess.Popen(['python3','tests/bound.py',str(limit),'env','PYTHONUNBUFFERED=1',*jobs[n]],stdout=log,stderr=subprocess.STDOUT, env=execution_environment())
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
    if args.com:
        subprocess.run([sys.executable, str(ROOT/"exec/c/provenance.py"), "check",
                        os.environ.get("MODEL_COM", str(ROOT/"unisacc.com"))], check=True, timeout=10)
    after = fingerprint(jobs)
    if after != stamp:
        if isinstance(after, dict):
            for n in jobs:
                if after[n] != stamp[n]: data['results'].pop(n, None)
        else: data['results'].clear()
        atomic(path, data)
        raise SystemExit('inputs changed during queue: results invalid')
    bad = [n for n,r in data['results'].items() if r['rc'] != 0]
    missing = set(jobs)-data['results'].keys()
    print('queue: %d/%d completed, %d failed, %d pending, window %.2fs; logs %s' %
          (len(data['results']),len(jobs),len(bad),len(missing),time.monotonic()-start,state),flush=True)
    return 75 if missing else (1 if bad else 0)

if __name__ == '__main__':
    sys.exit(main())
