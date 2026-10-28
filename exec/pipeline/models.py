#!/usr/bin/env python3
"""Reuse verified model preparation, never source execution or test results."""
import hashlib, json, os, pathlib, platform, shutil, subprocess, sys, tempfile, time
try:
    import fcntl          # POSIX build lock; absent on Windows, where only read-only identity (closure/digest) is used
except ImportError:   # pragma: no cover - Windows runners
    fcntl = None

ROOT = pathlib.Path(__file__).resolve().parents[2]

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def closure(h):
    """Conservative dependency closure: includes imported Python, embedded C
    templates, gold tables, carried headers and the inference verifier."""
    # seed/: the C99 constructor tools that build tables and networks by default since 0.0.26 B3'
    for directory in ('exec', 'unisa', 'src', 'kernel', 'include', 'weights', 'seed', 'cx'):
        # Byte-identical on every host: POSIX path spelling and case-sensitive part order
        # (Windows would spell backslashes and sort case-insensitively).
        for path in sorted((ROOT / directory).rglob('*'), key=lambda q: q.relative_to(ROOT).parts):
            # Generated output under exec/build (its subdirectories and any non-.py file) is never a
            # construction input; exec/build/*.py (gen.py, graph.py, procs.py, parsebase.py,
            # parse2base.py) are.  exec/.gitignore keeps only these (0.0.24 T3).
            rel = path.relative_to(ROOT).parts
            if rel[0] == 'seed' and rel[-1] not in ('tbl.c', 'net.c', 'json.h', 'gen.c', 'facts.h', 'ident.c', 'blob.c', 'sha256.h', 'pack.c', 'compilerpack.c', 'ape.c'):
                continue   # only the tools the build runs (0.0.32 B5: seed/gen.c builds the shared deltas)
            if rel[0] == 'exec' and path.suffix in ('.sh', '.py', '.cx') and path.stem.endswith('check'):
                continue   # 0.0.32 X31: exec/**/*check.{sh,py} verify the deltas; no build step runs or imports them
            if 'probes' in rel[:-1] and path.suffix in ('.c', '.h'):
                continue   # 0.0.35 P8: probe sources are test inputs; no generator reads them (a probe edit kept invalidating every model)
            if rel[:2] == ('exec', 'build') and (len(rel) > 3 or path.suffix != '.py'):
                continue   # only the tracked drivers exec/build/*.py are inputs; anything else there is generated
            if path.is_file() and path.suffix in ('.py', '.c', '.cx', '.h', '.inc', '.tsv', '.json', '.sh'):
                h.update(path.relative_to(ROOT).as_posix().encode() + b'\0')
                h.update(bytes.fromhex(digest(path)))
    path = ROOT / 'iterate/kernel/typekw.tsv'
    h.update(path.relative_to(ROOT).as_posix().encode() + b'\0')
    h.update(bytes.fromhex(digest(path)))
    # Product construction support moved out of the retired test tree.
    for name in ('bound', 'bound.c', 'cxbin', 'safedel.cx', 'lib.sh',
                 'build_ref.sh', 'export_ref.sh', 'refshim.h', 'reffoot.h', 'sourceflat.cx'):
        path = ROOT / 'tools' / name
        h.update(path.relative_to(ROOT).as_posix().encode() + b'\0')
        h.update(bytes.fromhex(digest(path)))
    return h

def identity(target, network, compiler, native_ua=None):
    exe = pathlib.Path(shutil.which(compiler) or compiler).resolve(strict=True)
    version = subprocess.run([str(exe), '--version'], capture_output=True, timeout=5, check=True)
    h = hashlib.sha256(json.dumps([str(ROOT), target, network, str(exe), digest(exe),
                                  platform.platform(), sys.version, version.stdout.hex()]).encode())
    if native_ua is not None:
        for path in (pathlib.Path(native_ua).resolve(strict=True), ROOT/'exec/c/nativecheck.sh', ROOT/'tools/sourceflat.cx', ROOT/'tools/export_ref.sh', ROOT/'unisacc.com'):
            h.update(str(path).encode()+b'\0'+bytes.fromhex(digest(path)))
        h.update(b'native-resources-preparation-v2-sourceflat-cx')
    return closure(h).hexdigest()

def valid(cache, network, native_ua=None, target=None):
    try:
        manifest = json.loads((cache / 'manifest.json').read_text())
        required = {'run'} | {s+'.'+ext for s in ('e2','e1','e3','e4','prune','lower','elf')
                             for ext in (('json','tbl','net') if network == '1' else ('json','tbl'))}
        if network == '1': required |= {'models.pkg', 'route.tsv'}
        if native_ua is not None:required |= {'run-ua','run.'+('pe' if target.startswith('win/') else 'macho' if target.startswith('osx/') else 'elf')}
        if not isinstance(manifest, dict) or set(manifest) != required:
            return False
        for name, value in manifest.items():
            if pathlib.Path(name).name != name or digest(cache / name) != value:
                return False
        return manifest
    except (FileNotFoundError, ValueError):
        return False

def prepare(out, target, network, compiler, native_ua=None, require_cached=False):
    start = time.monotonic()
    if native_ua is not None:
        hostos={'Linux':'lnx','Darwin':'osx'}.get(platform.system())
        hostarch={'x86_64':'x86_64','arm64':'arm64','aarch64':'arm64'}.get(platform.machine())
        if not hostos or not hostarch or target!=hostos+'/'+hostarch:
            print('not covered: native resources preparation host/target mismatch',file=sys.stderr)
            raise SystemExit(2)
    key = identity(target, network, compiler, native_ua)
    base = pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE', tempfile.gettempdir() + '/unisacc-model-cache'))
    base.mkdir(parents=True, exist_ok=True)
    cache = base / key
    with (base / (key + '.lock')).open('a') as lock:
        if fcntl is None: raise RuntimeError("model construction lock needs fcntl (POSIX); this is a read-only host")
        fcntl.flock(lock, fcntl.LOCK_EX)
        manifest = valid(cache, network, native_ua, target)
        hit = bool(manifest)
        if not hit and require_cached:
            print('NEED_PREPARE native resources: run exec-native-resources-prep',file=sys.stderr)
            raise SystemExit(75)
        if not hit and os.environ.get('UNISACC_REQUIRE_PREPARED'):
            raise SystemExit('not prepared: run exec-driver-core-prep-0 first (UNISACC_REQUIRE_PREPARED: no construction here)')
        if not hit:
            work = pathlib.Path(tempfile.mkdtemp(prefix=key + '.build-', dir=base))
            try:
                subprocess.run([str(ROOT / 'tools/bound'), '60', 'sh',
                                str(ROOT / 'exec/pipeline/prepare.sh'), str(work),
                                target, network, compiler], cwd=ROOT, check=True)
                if native_ua is not None:
                    subprocess.run([str(ROOT/'tools/bound'),'55','sh',str(ROOT/'exec/c/nativecheck.sh'),'--build-prepared',str(work)],cwd=ROOT,env=dict(os.environ,UA=str(pathlib.Path(native_ua).resolve()),TARGET=target),check=True)
                    keep={'run','run-ua','models.pkg','route.tsv','run.'+('pe' if target.startswith('win/') else 'macho' if target.startswith('osx/') else 'elf')}|{x+'.'+ext for x in ('e2','e1','e3','e4','prune','lower','elf') for ext in ('json','tbl','net')}
                    for path in work.iterdir():
                        if path.is_file() and path.name not in keep:subprocess.run(['sh',str(ROOT/'tools/cxbin'),str(ROOT/'tools/safedel.cx'),'--purge','--',str(path)],check=True)
                manifest = {p.name: digest(p) for p in sorted(work.iterdir()) if p.is_file()}
                (work / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True))
                try: cache.stat()
                except FileNotFoundError: pass
                else: subprocess.run(['sh',str(ROOT/'tools/cxbin'),str(ROOT/'tools/safedel.cx'),'--',str(cache)],check=True)   # a stale cache entry: into the .trash of its root
                work.rename(cache)
            finally:
                try: work.stat()
                except FileNotFoundError: pass
                else: subprocess.run(['sh',str(ROOT/'tools/cxbin'),str(ROOT/'tools/safedel.cx'),'--purge','--max-files','20000','--',str(work)],check=True)   # own mkdtemp build tree: a site ceiling
        out.mkdir(parents=True, exist_ok=True)
        for name in manifest:
            shutil.copy2(cache / name, out / name)
    print('model preparation: %s %s %.2fs key=%s' %
          ('hit' if hit else 'built', target, time.monotonic() - start, key), flush=True)

if __name__ == '__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('out',type=pathlib.Path);p.add_argument('target');p.add_argument('network',choices=('0','1'));p.add_argument('compiler');p.add_argument('--native-ua');p.add_argument('--require-cached',action='store_true');a=p.parse_args()
    if a.native_ua and a.network!='1':p.error('native resource preparation requires network=1')
    prepare(a.out.resolve(),a.target,a.network,a.compiler,a.native_ua,a.require_cached)
