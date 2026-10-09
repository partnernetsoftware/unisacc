#!/usr/bin/env python3
"""Rolling parallel gate, bounded windows, durable exact per-suite results.

Run through tests/term.sh. Exit 75 means pending work: invoke the SAME command
again. Audited input closures permit per-suite reuse; unknown suites retain global
input invalidation. Model preparation may
be shared separately. Each window is <=55 s, leaving cleanup inside the 60 s
outer watchdog. Long jobs go first; shorter jobs fill remaining slots.
"""
import argparse, bisect, re, fcntl, hashlib, json, os, pathlib, platform, shutil, stat, subprocess, sys, time
from gatelayers import LAYERS, select as select_layers

ROOT = pathlib.Path(__file__).resolve().parents[1]
# comboot records its stages in ROOT/out/comboot/stages.json: a seed result from another
# checkout does not put stage 1 there (0.0.21: stage2/3 "recorded before stage 1")
LOCATION_BOUND = ('com-comboot-',)
# A job that consumes another's recorded state starts only after it passed (0.0.21 and 0.0.22:
# the longest-first pick ran stage2 before seed -- "stage 2 recorded before stage 1").
# scheduling order lives in tests/gateorder.json (0.0.32 P7'): it decides only when a job starts, so it is
# kept out of the common identity -- two edits of this table in q10 invalidated all 688 results each time
AFTER = json.loads((pathlib.Path(__file__).resolve().parent/'gateorder.json').read_text())['after']
# a job waits for one predecessor (a name) or for several (a list): a union over shards waits for all of them
PREDS = {n: ([p] if isinstance(p, str) else list(p)) for n, p in AFTER.items()}

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
    # OLDPWD is shell history, not an input; PWD (the worktree) goes through portable() as {ROOT}.
    return {k:v for k,v in os.environ.items() if k not in ('TERM_SESSION_ID','OLDPWD')}

def execution_settings():
    return {k:os.environ[k] for k in ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA','CC','CFLAGS','TARGET','DRIVE','NETWORK',
                'EXEC_CC','PAR','STRICT','SHARD','CHAINKEEP','E3KEEP','E4STRICT',
                'UNISA_MAXSTEPS','UNISA_CONTAINER','UNISA_KERNEL','UNISACC_FFI_PROVIDER','UNISACC_FFI_X86_PROVIDER') if k in os.environ}

# 0.0.31 P7': path-valued selectors (the candidate, the same-source UA, the FFI provider) live in a
# per-round scratch directory, so their PATHS changed every rebuild and voided every stamp (0.0.30
# third queue: 0/671 reused although only tests/docs changed).  Their contents are identities already
# (executable_inputs, provider_inputs); stamps and the stored job table see {KEY} instead of the path.
PATH_KEYS = ('MODEL_COM','UA','UA_RUN','TOOLS_UA','CORPUS_UA','UNISA_CONTAINER','UNISA_KERNEL',
             'UNISACC_FFI_PROVIDER','UNISACC_FFI_X86_PROVIDER','SEED_DIR')

# hashed, so portable() cannot fold it into {ROOT}: LOCATION_BOUND jobs keep state inside the checkout
ROOT_ID = hashlib.sha256(str(ROOT).encode()).hexdigest()

def portable(text):
    pairs = []
    for k in PATH_KEYS:
        v = os.environ.get(k, '')
        if '/' not in v: continue
        pairs.append((v, '{%s}' % k))
        try: r = str(pathlib.Path(v).resolve())
        except OSError: r = v
        if r != v: pairs.append((r, '{%s}' % k))
    # The worktree itself (cdx: PWD alone moved every stamp): resolved and as the shell spells it.
    for v in {str(ROOT), os.environ.get('PWD', '')}:
        if v and pathlib.Path(v).resolve() == ROOT: pairs.append((v, '{ROOT}'))
    for v, key in sorted(pairs, key=lambda kv: -len(kv[0])): text = text.replace(v, key)
    return text

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

def gate_runner():
    """gate.sh without its job lines: the wrapper every job runs under (bound, output, ordering)."""
    try: text = pathlib.Path('tests/gate.sh').read_text()
    except FileNotFoundError: return 'missing'
    keep = [l for l in text.splitlines() if not re.match(r'\s*(job |for .*; do .*job |\[ .*\] \|\| job )', l)]
    return hashlib.sha256('\n'.join(keep).encode()).hexdigest()

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
                # git's view of a mode: only the execute bit is content (cdx/cc 0.0.31: a 0600 copy of a
                # 0644 file voided half the queue across worktrees)
                files[name] = [0o100755 if mode & 0o111 else 0o100644, h.hexdigest()]
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
    # 0.0.38 ②: the files under a tree prefix, by binary search on the sorted list -- the same set the
    # per-job scan `any(n.startswith(prefix+'/') ...)` produced, without 10.8M startswith calls per
    # fingerprint (12 s, twice per window, measured 10-10)
    under_cache = {}
    def under(prefix, pool=None):
        key = (prefix, id(pool))
        if key not in under_cache:
            if pool is None and 'sorted' not in under_cache: under_cache['sorted'] = sorted(names)   # names gains unisacc.com unsorted
            src = under_cache['sorted'] if pool is None else pool
            lo = bisect.bisect_left(src, prefix + '/'); hi = bisect.bisect_left(src, prefix + '0')   # '0' follows '/'
            under_cache[key] = src[lo:hi]
        return under_cache[key]
    if digest('unisacc.com') != ['missing']: names.append('unisacc.com')
    tools = {}
    for name in ('python3', 'sh', 'bash', 'cc', 'openssl', 'grep', 'wc', 'tr', 'uname', 'id', 'env', '/bin/ps', '/usr/bin/openssl'):
        path = shutil.which(name)
        tools[name] = [path, digest(path)] if path else ['missing']
    for key in ('CC', 'EXEC_CC'):
        if settings.get(key):
            path = shutil.which(settings[key])
            tools[key] = [path, digest(path)] if path else ['missing', settings[key]]
    # 0.0.38 P2/P7: external tools the host plan declares are identities too -- the csmith executable
    # and the headers it compiles against, and the LLVM tools a cross build runs (a changed header or
    # tool must not reuse an old result; the hostcheck summary is never a suite result)
    path = shutil.which(os.environ.get('CSMITH') or 'csmith')
    tools['csmith'] = [path, digest(path)] if path else ['missing']
    incs = [os.environ['CSMITH_INCLUDE']] if os.environ.get('CSMITH_INCLUDE') else (
        sorted(str(d) for d in pathlib.Path(path).resolve().parents[1].glob('include/csmith-*')) + ['/usr/include/csmith'] if path else [])
    for d in incs:
        dp = pathlib.Path(d)
        if dp.is_dir():
            tools['csmith-include:' + d] = {str(f.relative_to(dp)): digest(str(f)) for f in sorted(dp.rglob('*.h'))}
    if os.environ.get('LLVM_BIN'):
        lb = pathlib.Path(os.environ['LLVM_BIN'])
        for name in ('clang', 'ld.lld', 'lld-link', 'llvm-nm', 'llvm-ar', 'llvm-objcopy'):
            tools['LLVM_BIN/' + name] = [str(lb / name), digest(str(lb / name))] if (lb / name).exists() else ['missing']
    provider_inputs = {}
    for key in ('UNISACC_FFI_PROVIDER','UNISACC_FFI_X86_PROVIDER'):
        if settings.get(key):
            directory = pathlib.Path(settings[key]).resolve(strict=True)
            provider_inputs[key] = {str(directory/name):digest(str(directory/name)) for name in
                ('manifest.json','lib/libffi.a','include/ffi.h','include/ffitarget.h','include/ffi/ffi.h','include/ffi/ffitarget.h','LICENSE')}
    # 0.0.22: the checkout path is not an identity -- a queue continued in a new worktree of the
    # same content reuses its results (0.0.21: ~420/432 invalidated by the path alone).  Jobs that
    # keep state inside the checkout (LOCATION_BOUND) still carry it.
    # 0.0.31 P7' fix (found by cdx): SEED_DIR is a {KEY} path in stamps, so the seed's CONTENT must be an
    # identity here -- before, only the changing path kept two different seeds apart
    seed_inputs = {}
    if os.environ.get('SEED_DIR'):
        for name in ('unisacc-seed.com', 'unisacc-seed.com.build.json'):
            seed_inputs[name] = digest(str(pathlib.Path(os.environ['SEED_DIR']) / name))
    common = [provider_inputs, seed_inputs, execution_environment(), platform.platform(), platform.machine(), sys.version,
              str(pathlib.Path(sys.executable).resolve()), tools,
              {'tests/gatequeue.py':contract_digest('tests/gatequeue.py'), 'tests/bound.py':digest('tests/bound.py')}, gate_runner()]
    # 0.0.29 P7': gate.sh's job lines and gatedeps.json are no longer global identities -- a job's own
    # command, declared inputs and guards are in its stamp already, so editing one job line or refreshing
    # another suite's declaration keeps every other result (0.0.28 measured 0% reuse because of these two)
    def stamp(value): return hashlib.sha256(portable(json.dumps(value, sort_keys=True)).encode()).hexdigest()
    global_inputs = None
    inventories, family_tools = {}, {}
    extra_trees = {}
    result = {}
    for name, command in jobs.items():
        entry = manifest.get('suites', {}).get(name)
        if entry and 'family' in entry:
            entry = dict(manifest['families'][entry['family']], command=entry['command'], family=entry['family'])
        # a declared command may name the candidate as {MODEL_COM}: gate.sh expands
        # ${MODEL_COM:-./unisacc.com} at list time, so the path differs between runs while the
        # job does not (its bytes are an executable input either way)
        mc = settings.get('MODEL_COM') or str(ROOT / 'unisacc.com')
        # 0.0.29 P7': {MODEL_COM} may stand inside an argument (UA={MODEL_COM}); every product suite was
        # declared with the main checkout's absolute path, so in a release queue (MODEL_COM = the candidate)
        # none of the 87 com- declarations matched and all of them hashed the whole tree
        # 0.0.32 P7': {UA} too (combo, exec-prune-0..3 were declared with it but never matched); every
        # PATH_KEYS value set for this queue is expanded the same way on both sides
        keys = {'{%s}' % k: settings.get(k) or os.environ.get(k) for k in PATH_KEYS}
        keys['{MODEL_COM}'] = mc
        def expand(a):
            for k, v in keys.items():
                if v: a = a.replace(k, v)
            return a
        declared_cmd = [expand(a) for a in entry['command']] if entry else None
        # 0.0.23 E: "match": "contains" -- the declared command is a contiguous run of the job's
        # command; the job's full command stays in the identity, so a changed argument list (new
        # test files in a shard, an env prefix) re-runs the job but no longer drops its declaration
        # 0.0.32 P7': the planned job keeps {MODEL_COM} unexpanded, so expand it on both sides (q11: combo and
        # exec-container never matched and fell back to the global fingerprint)
        shown = [expand(a) for a in command]
        if entry and entry.get('match') == 'contains':
            k = len(declared_cmd)
            audited = any(shown[i:i+k] == declared_cmd for i in range(len(shown)-k+1))
        else:
            audited = entry is not None and shown == declared_cmd
        inventory, extra = {}, None
        if entry and entry.get('reviewed_trees'):
            for tree, reviewed in entry['reviewed_trees'].items():
                if tree not in inventories:
                    # gitignored generated output (exec/build/* beyond the tracked *.py) is not a reviewed input
                    visible = set(subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others',
                        '--exclude-standard', '--', tree]).decode().split('\0')) - {''}
                    paths = sorted(p for p in pathlib.Path(tree).rglob('*') if str(p) in visible and p.is_file() and
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
            guards = entry.get('guards', {})   # no guards (fresh-order-*): not audited, global inputs
            audited = bool(guards) and all(digest(n)[-1] == sha for n,sha in guards.items())
            # Added Python modules must also lose the reviewed import closure.
            guarded = set(guards)
            audited = audited and all(n in guarded for prefix in entry.get('code_trees', [])
                for n in under(prefix) if n.endswith('.py'))
        inputs = []
        if entry:
            trees = entry.get('trees', [])
            for prefix in trees:   # declared trees outside the fingerprint pathspec (seed/, plans/...)
                if prefix not in extra_trees:
                    raw2 = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--', prefix])
                    extra_trees[prefix] = sorted(set(raw2.decode().split('\0')) - {''})
            # tracked = names plus every listed extra tree; keep the names under any declared prefix
            sel = set(entry['files'])
            for q in trees:
                sel.update(under(q))
                for p in trees: sel.update(under(q, extra_trees[p]))
            inputs = sorted(sel)
        if audited:
            identity = [common, command, {n:digest(n) for n in inputs}]
            if entry.get('executable_inputs'): identity += [settings, executables, inventory, extra]
            if name.startswith(LOCATION_BOUND): identity.append(ROOT_ID)
            result[name] = stamp(identity)
        else:
            if global_inputs is None: global_inputs = {n:digest(n) for n in names}
            # 0.0.23 E: the job's own command, not the whole job table -- adding or changing one
            # `job` line no longer invalidates every undeclared result (inputs stay global)
            identity = [common, command, settings, executables, global_inputs, {n:digest(n) for n in inputs}]
            if inventory: identity += [inventory, extra]
            if name.startswith(LOCATION_BOUND): identity.append(ROOT_ID)
            result[name] = stamp(identity)
    return result

def stagelog_segments(args, *marks):
    """0.0.38 P6: one paired begin/end per window segment in the private stage log (never fatal)."""
    if not args.stagelog_run: return
    sys.path.insert(0, str(ROOT/'release/tools'))
    try:
        import stagelog
        boot = stagelog.boot_id(); path = stagelog.logpath(None)
        for name, a, b in zip(('prologue', 'jobs', 'epilogue'), marks, marks[1:]):
            eid = 'e-' + os.urandom(8).hex()
            utc = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())
            stagelog.write(path, {'event':'begin','id':eid,'run':stagelog.token(args.stagelog_run,'run'),'phase':'queue',
                                  'subphase':name,'attempt':1,'parent_id':stagelog.token(args.stagelog_parent or None,'parent id'),
                                  'source_commit':None,'mono':round(a,6),'boot_id':boot,'utc':utc})
            stagelog.write(path, {'event':'end','id':eid,'rc':None,'acceptance_status':'UNVERIFIED','execution_status':None,
                                  'authorization_ref':None,'cpu_user':None,'cpu_sys':None,'rss_kib':None,
                                  'mono':round(b,6),'boot_id':boot,'utc':utc})
    except (SystemExit, OSError, ImportError) as e:
        print('stagelog: window segments not recorded (%s)' % e, file=sys.stderr)

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

def contract_digest(name):
    """sha256 of NAME without its SCHEDULING-BEGIN..END regions: an edit that only changes which job
    starts when keeps every result; any other edit (commands, limits, retries, environment) does not."""
    out, skip = [], False
    try: text = pathlib.Path(name).read_text()
    except FileNotFoundError: return ['missing']   # as digest(): queuecheck fixtures run from a tree without it
    for line in text.splitlines(True):
        tag = line.strip()
        if tag.startswith('# SCHEDULING-BEGIN'):
            if skip: raise SystemExit('queue: nested SCHEDULING-BEGIN in '+name)
            skip = True; continue
        if tag == '# SCHEDULING-END':
            if not skip: raise SystemExit('queue: stray SCHEDULING-END in '+name)
            skip = False; continue
        if not skip: out.append(line)
    if skip: raise SystemExit('queue: unterminated SCHEDULING-BEGIN in '+name)
    return hashlib.sha256(''.join(out).encode()).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--state', type=pathlib.Path)
    ap.add_argument('--com', action='store_true')
    ap.add_argument('--jobs', type=int, default=2)
    ap.add_argument('--window', type=int, default=55)
    ap.add_argument('--suite', action='append', default=[])
    layers = ap.add_mutually_exclusive_group()
    layers.add_argument('--layer', choices=LAYERS, help='run exactly one diagnostic breadth layer')
    layers.add_argument('--through-layer', choices=LAYERS,
                        help='run this layer and all narrower layers')
    ap.add_argument('--list-selection', action='store_true', help='print selected suite names without running')
    ap.add_argument('--exclusive-suite', action='append', default=[],
                    help='selected suite that must run alone (repeatable; prioritized)')
    # 0.0.38 P3: the parent's deadline (time.monotonic(), same boot) reaches the scheduler as an
    # argument, never through the suites' environment, so it is not part of any job identity
    ap.add_argument('--parent-deadline', type=float, default=None,
                    help='monotonic second by which this window must have exited (outer bound)')
    # 0.0.38 P6: where this window's prologue/jobs/epilogue go in the private stage log (optional)
    ap.add_argument('--stagelog-run', default=None); ap.add_argument('--stagelog-parent', default=None)
    args = ap.parse_args()
    if not 1 <= args.jobs <= 4 or not 5 <= args.window <= 55: ap.error('jobs 1..4; window 5..55')
    if not args.list_selection and args.state is None:
        ap.error('--state is required when running suites')
    t_begin = time.monotonic()
    os.chdir(ROOT)
    if args.com and not args.list_selection:
        subprocess.run([sys.executable, str(ROOT/"exec/c/provenance.py"), "check",
                        os.environ.get("MODEL_COM", str(ROOT/"unisacc.com"))], check=True, timeout=10)
    jobs = plan(args.com)
    jobs = select_layers(jobs, args.layer, args.through_layer)
    if args.suite:
        if len(set(args.suite)) != len(args.suite) or set(args.suite) - jobs.keys():
            ap.error('duplicate/unknown suite or suite outside selected layer')
        jobs = {n: jobs[n] for n in args.suite}
    if args.list_selection:
        print('\n'.join(jobs))
        return
    exclusive = set(args.exclusive_suite)
    if len(exclusive) != len(args.exclusive_suite) or exclusive - jobs.keys():
        ap.error('duplicate/unknown exclusive suite (must belong to selected jobs)')
    state = args.state.resolve(); state.mkdir(parents=True, exist_ok=True)
    lock = (state/'lock').open('a'); fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    stamp = fingerprint(jobs); path = state/'results.json'
    data = json.loads(path.read_text()) if path.is_file() else {'stamp':stamp, 'jobs':jobs, 'exclusive':sorted(exclusive), 'results':{}}
    resume(data, stamp, {n:portable(c) if isinstance(c, str) else json.loads(portable(json.dumps(c))) for n, c in jobs.items()}, exclusive)
    atomic(path, data)
    # Classic and network drivers, and different concurrency, have different costs.
    # 0.0.33 Q1: path-valued selectors name a per-round scratch path; keyed by the path, every
    # candidate started with no history, every job was estimated at 30 s (admitted only with >=41 s
    # left: cand13 limits were all 40-49 s, mean job 9.7 s), and window tails went unused.
    settings = {k:('{'+k+'}' if k in PATH_KEYS else v) for k, v in execution_settings().items()}
    # 0.0.38: one history per settings profile -- not per exclusive set (changing one exclusive suite
    # moved every suite to an empty history); each entry carries its suite's identity instead
    profile = json.dumps([settings], sort_keys=True).encode()
    histpath = pathlib.Path(os.environ.get('TMPDIR','/tmp')) / ('unisacc-gate-times-'+hashlib.sha256(profile).hexdigest()[:16]+'.json')
    history = json.loads(histpath.read_text()) if histpath.is_file() else {}
    pending = [n for n in jobs if n not in data['results']]
    exclusive |= {n for n in data.get('retried', []) if n in pending}   # a retry runs alone in later windows too
    active = {}; start = time.monotonic()
    t_jobs = start
    # 0.0.32: q15 -- a 54 s window plus the epilogue (provenance check, refingerprint) crossed term.sh's 60 s
    # SCHEDULING-BEGIN (0.0.38 full038b: the wrap-up reserve is the slowest measured wrap-up plus 1 s,
    # never less than before; a fixed 4 s let a loaded wrap-up overrun the outer bound)
    epilogue = max(4 if args.com else 2, int(-(-(data.get('epilogue_max', 0) + 1) // 1)))
    # SCHEDULING-END
    window = args.window - epilogue
    deadline = start + window
    # 0.0.38 P3: one deadline for every layer.  The parent's remaining time already paid for the
    # prologue above (provenance, fingerprint); a job limit never exceeds what the outer bound leaves,
    # so the outer kill no longer lands before the job's own limit (0.0.37: rowcov x4, 12 windows)
    if args.parent_deadline is not None:
        deadline = min(deadline, args.parent_deadline - epilogue)
    # 0.0.38 P3: a window killed from outside leaves its inflight records.  The job's real rc is
    # unknown: count an interruption (never a synthetic 142); the second one is the result
    # (status INTERRUPTED, rc null).  A killed window that had completed nothing stops the driver.
    killed = data.pop('inflight', {})
    last = data.get('window', {})
    for n, rec in killed.items():
        if n in data['results'] or n not in jobs: continue
        hits = data.setdefault('interrupted', {})
        hits[n] = hits.get(n, 0) + 1
        print('INTERRUPTED', n, 'attempt', hits[n], 'limit', rec.get('limit'), flush=True)
        if hits[n] >= 2:
            data['results'][n] = {'rc': None, 'status': 'INTERRUPTED', 'seconds': None, 'limit': rec.get('limit')}
        elif n not in exclusive:
            exclusive.add(n)
    stall = data.get('stalled', 0)
    if killed and last.get('completed', 0) == 0:
        stall = max(stall, 3)
    data['stalled'] = stall
    data['window'] = {'completed': 0}
    atomic(path, data)
    pending = [n for n in jobs if n not in data['results']]
    if stall >= 3:
        print('queue: STALLED -- %s; no legal completion in the last windows (stop, do not repeat with 75)' %
              ('a window was killed from outside with nothing completed' if killed else '3 windows without progress'), flush=True)
        return 3
    if pending and args.parent_deadline is not None and deadline - time.monotonic() <= 3:
        print('queue: BUDGET_UNSCHEDULABLE -- %.1fs left after the prologue' % (deadline-time.monotonic()), flush=True)
        return 3
    # 0.0.33 Q1: a job with no record takes the median of the recorded ones (30 s until five exist).  The flat
    # 30 s (-> 40 s estimate) kept every unmeasured job out of the last 40 s of each window; a job that
    # then runs out of window is deferred and retried with twice its elapsed time (below), so a wrong guess
    # costs one cut-short attempt.
    # SCHEDULING-BEGIN (0.0.35 P11: admission order/estimates only; masked out of the queue identity)
    # 0.0.38: an entry counts only for the suite identity it was measured under; a changed suite starts
    # from the cold prior.  Completed durations ('ok') and lower bounds from timeouts/deferrals ('lb')
    # are kept apart.  Estimates order admission only: they never set a limit or decide a result.
    ident = (lambda n: stamp.get(n)) if isinstance(stamp, dict) else (lambda n: stamp)
    def entry(n):
        e = history.get(n)
        return e if isinstance(e, dict) and e.get('id') == ident(n) else None
    def known(n):
        e = entry(n)
        return None if e is None else max(e.get('ok') or 0, e.get('lb') or 0) or None
    def note(n, ok=None, lb=None):
        e = entry(n) or {'id': ident(n)}
        if ok is not None: e['ok'] = max(ok, ((e.get('ok') or ok) + ok) / 2)   # rise at once, fall by halves
        if lb is not None: e['lb'] = max(lb, e.get('lb') or 0)
        history[n] = e
    def cold():
        v = sorted(e['ok'] for n, e in history.items() if isinstance(e, dict) and e.get('ok') and entry(n))
        return v[len(v)//2] if len(v) >= 5 else 30
    # 0.0.38 P3 fix (full038): under a parent deadline a full window is `span`, not `window`; capping
    # estimates and full-attempt tests at `window` made every 46 s attempt a 'late fill' and doubled its
    # estimate past any window -- nothing admissible, the queue stalled with the job never decided
    span = deadline - start
    def estimate(n): return min(span-2, max(2, (known(n) or cold())*1.3+1))
    # SCHEDULING-END
    try:
        while pending or active:
            left = deadline-time.monotonic()
            while pending and len(active) < args.jobs and left > 2:
                if exclusive.intersection(active): break
                for n in [n for n in pending if any(p in data['results'] and data['results'][p]['rc'] != 0 for p in PREDS.get(n, ()))]:
                    bad = [p for p in PREDS[n] if p in data['results'] and data['results'][p]['rc'] != 0]
                    pending.remove(n); data['results'][n] = {'rc': 1, 'seconds': 0, 'limit': 0}
                    print('DONE', n, 'rc=1 0.00s predecessor', ' '.join(bad), 'failed', flush=True)
                # SCHEDULING-BEGIN
                # 0.0.35 P12: a job deferred out of a tail runs only at a full window, first (rc4 tools-1 was
                # deferred twice, from 13 s and 36 s tails: 49 s of slot time lost)
                full = set(data.get('fullwindow', []))
                fits = [n for n in pending if estimate(n) <= left-1 and (n not in full or left >= span-3)
                        and all(p not in jobs or p in data['results'] for p in PREDS.get(n, ()))]
                if not fits: break
                first = [n for n in fits if n in full]
                if first: fits = first
                alone = [n for n in fits if n in exclusive]
                if alone and active: break  # drain ordinary work before the priority job
                n = max(alone or fits, key=estimate); pending.remove(n)
                # SCHEDULING-END
                limit = max(1, int(left)-1)
                # 0.0.38: the attempt's kind is decided here, from the exact time left, and recorded --
                # not rebuilt at exit from the rounded limit (full038b: 46 s attempts counted as tail fills)
                kind = 'solo' if n in data.get('retried', []) else ('full' if left >= span - 3 else 'tail')
                data.setdefault('inflight', {})[n] = {'limit': limit, 'kind': kind, 'left': round(left, 3)}; atomic(path, data)
                log = (state/(n+'.log')).open('wb')
                p = subprocess.Popen(['python3','tests/bound.py',str(limit),'env','PYTHONUNBUFFERED=1',*jobs[n]],stdout=log,stderr=subprocess.STDOUT, env=execution_environment())
                active[n] = (p, log, time.monotonic(), limit)
                print('START',n,'limit='+str(limit),flush=True)
                left = deadline-time.monotonic()
            for n, (p, log, t, limit) in list(active.items()):
                rc = p.poll()
                if rc is None: continue
                log.close(); elapsed = time.monotonic()-t
                progress = True
                kind = data.get('inflight', {}).get(n, {}).get('kind', 'full')
                if rc == 142 and kind == 'tail':
                    # A late fill used only the window remainder, not a full
                    # attempt. Keep it pending and give it an early slot next.
                    # 0.0.38 P3: deferring a job that already had its early slot is not progress
                    progress = n not in data.get('fullwindow', [])
                    pending.append(n)
                    data.setdefault('deferred', []).append({'name':n,'limit':limit})
                    if n not in data.setdefault('fullwindow', []): data['fullwindow'].append(n)
                    note(n, lb=elapsed*2)
                elif rc == 142 and n not in data.setdefault('retried', []) and n not in exclusive:
                    # 0.0.32: a full attempt that timed out is retried once, alone (q10-q12: jobs that take
                    # 24-45 s alone hit the watchdog beside three others); a second timeout is the result
                    pending.append(n); exclusive.add(n); data['retried'].append(n)
                    note(n, lb=elapsed)
                else:
                    data['results'][n] = {'rc':rc, 'seconds':round(elapsed,3),'limit':limit}
                    # 0.0.38 P7: rc 77 is a suite's UNVERIFIED (its host lacks a declared requirement):
                    # neither PASS nor FAILED; the release check lists it and does not pass it
                    if rc == 77: data['results'][n]['status'] = 'UNVERIFIED'
                    # Q1 (0.0.33): rise at once, fall by halves -- a cache-warm 2 s run must not
                    # admit the next cold 20 s run into a 6 s tail (cand33: 17% of slot time died as DEFER)
                    note(n, ok=elapsed)
                # a result, a first retry or a first deferral is progress; a repeated deferral is not
                if progress: data['window']['completed'] = data['window'].get('completed', 0) + 1
                data.get('inflight', {}).pop(n, None)
                atomic(path,data); atomic(histpath,history)
                lines = (state/(n+'.log')).read_text(errors='replace').splitlines()
                print('DONE' if n in data['results'] else 'DEFER',n,'rc='+str(rc),'%.2fs'%elapsed,lines[-1] if lines else '(no output)',flush=True)
                del active[n]
            if not active and (not pending or not any(estimate(n) <= deadline-time.monotonic()-1 for n in pending)): break
            if time.monotonic() >= deadline: break
            time.sleep(.05)
    finally:
        t_tail = time.monotonic()   # 0.0.38: the tail budget starts here, reaping included
        # A signal/exception/window ending cannot leave tests behind.  0.0.38 full038b: stop the leftovers
        # together within one 2 s grace, then kill and wait at most 1 s more each (bound.py reaps its own
        # process group); a 2 s wait per job in turn, raising on a slow one, overran the outer bound.
        for p, log, _, _ in active.values(): p.terminate()
        grace = time.monotonic() + 2
        for p, log, _, _ in active.values():
            try: p.wait(timeout=max(0.05, grace - time.monotonic()))
            except subprocess.TimeoutExpired:
                p.kill()
                try: p.wait(timeout=1)
                except subprocess.TimeoutExpired: print('queue: a killed job did not exit within 1 s', flush=True)
        for n, (p, log, t, limit) in active.items():
            log.close()
            data['results'][n] = {'rc':142,'seconds':round(time.monotonic()-t,3),'limit':limit}
            data.get('inflight', {}).pop(n, None)
            data['window']['completed'] = data['window'].get('completed', 0) + 1
        # 0.0.38 P3: three windows in a row without a legal completion stop the driver
        data['stalled'] = 0 if data['window'].get('completed') else data.get('stalled', 0) + 1
        atomic(path,data)
    t_epi = t_tail
    if args.com:
        subprocess.run([sys.executable, str(ROOT/"exec/c/provenance.py"), "check",
                        os.environ.get("MODEL_COM", str(ROOT/"unisacc.com"))], check=True, timeout=10)
    after = fingerprint(jobs)
    t_end = time.monotonic()
    data['window'].update(prologue_s=round(t_jobs-t_begin,3), jobs_s=round(t_epi-t_jobs,3), epilogue_s=round(t_end-t_epi,3))
    # 0.0.38: remember the slowest whole tail (reaping, saves, provenance, fingerprint) for the next reserve
    data['epilogue_max'] = round(max(data.get('epilogue_max', 0), t_end - t_tail), 3)
    atomic(path, data)
    stagelog_segments(args, t_begin, t_jobs, t_epi, t_end)
    if after != stamp:
        if isinstance(after, dict):
            changed = [n for n in jobs if after[n] != stamp[n]]
            for n in changed: data['results'].pop(n, None)
        else:
            changed = ['(fingerprint shape changed)']
            data['results'].clear()
        atomic(path, data)
        raise SystemExit('inputs changed during queue: results invalid; %d jobs: %s' % (len(changed), ' '.join(changed[:20])))
    unv = [n for n,r in data['results'].items() if r.get('status') == 'UNVERIFIED']
    bad = [n for n,r in data['results'].items() if r['rc'] != 0 and n not in unv]
    missing = set(jobs)-data['results'].keys()
    print('queue: %d/%d completed, %d failed, %d unverified, %d pending, window %.2fs; logs %s' %
          (len(data['results']),len(jobs),len(bad),len(unv),len(missing),time.monotonic()-start,state),flush=True)
    if data.get('stalled', 0) >= 3 and missing:
        print('queue: STALLED -- 3 windows without a legal completion (stop, do not repeat with 75)', flush=True)
        return 3
    return 75 if missing else (1 if bad else (4 if unv else 0))

if __name__ == '__main__':
    sys.exit(main())
