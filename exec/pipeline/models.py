#!/usr/bin/env python3
"""Reuse verified model preparation, never source execution or test results."""
import fcntl, hashlib, json, os, pathlib, platform, shutil, subprocess, sys, tempfile, time

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
    for directory in ('exec', 'unisa', 'src', 'kernel', 'include', 'weights'):
        for path in sorted((ROOT / directory).rglob('*')):
            # Declared generated seed/cache output, never a construction input.
            if path.relative_to(ROOT).parts[:2] == ('exec', 'build'):
                continue
            if path.is_file() and path.suffix in ('.py', '.c', '.h', '.inc', '.tsv', '.json', '.sh'):
                h.update(str(path.relative_to(ROOT)).encode() + b'\0')
                h.update(bytes.fromhex(digest(path)))
    path = ROOT / 'iterate/kernel/typekw.tsv'
    h.update(str(path.relative_to(ROOT)).encode() + b'\0')
    h.update(bytes.fromhex(digest(path)))
    return h

def identity(target, network, compiler):
    exe = pathlib.Path(shutil.which(compiler) or compiler).resolve(strict=True)
    version = subprocess.run([str(exe), '--version'], capture_output=True, timeout=5, check=True)
    h = hashlib.sha256(json.dumps([str(ROOT), target, network, str(exe), digest(exe),
                                  platform.platform(), sys.version, version.stdout.hex()]).encode())
    return closure(h).hexdigest()

def valid(cache, network):
    try:
        manifest = json.loads((cache / 'manifest.json').read_text())
        required = {'run'} | {s+'.'+ext for s in ('e2','e1','e3','e4','prune','lower','elf')
                             for ext in (('json','tbl','net') if network == '1' else ('json','tbl'))}
        if network == '1': required |= {'models.pkg', 'route.tsv'}
        if not isinstance(manifest, dict) or set(manifest) != required:
            return False
        for name, value in manifest.items():
            if pathlib.Path(name).name != name or digest(cache / name) != value:
                return False
        return manifest
    except (FileNotFoundError, ValueError):
        return False

def prepare(out, target, network, compiler):
    start = time.monotonic()
    key = identity(target, network, compiler)
    base = pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE', tempfile.gettempdir() + '/unisacc-model-cache'))
    base.mkdir(parents=True, exist_ok=True)
    cache = base / key
    with (base / (key + '.lock')).open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        manifest = valid(cache, network)
        hit = bool(manifest)
        if not hit:
            work = pathlib.Path(tempfile.mkdtemp(prefix=key + '.build-', dir=base))
            try:
                subprocess.run(['python3', str(ROOT / 'tests/bound.py'), '60', 'sh',
                                str(ROOT / 'exec/pipeline/prepare.sh'), str(work),
                                target, network, compiler], cwd=ROOT, check=True)
                manifest = {p.name: digest(p) for p in sorted(work.iterdir()) if p.is_file()}
                (work / 'manifest.json').write_text(json.dumps(manifest, sort_keys=True))
                try: cache.stat()
                except FileNotFoundError: pass
                else: shutil.rmtree(cache)
                work.rename(cache)
            finally:
                try: work.stat()
                except FileNotFoundError: pass
                else: shutil.rmtree(work)
        out.mkdir(parents=True, exist_ok=True)
        for name in manifest:
            shutil.copy2(cache / name, out / name)
    print('model preparation: %s %s %.2fs key=%s' %
          ('hit' if hit else 'built', target, time.monotonic() - start, key), flush=True)

if __name__ == '__main__':
    if len(sys.argv) != 5 or sys.argv[3] not in ('0', '1'):
        raise SystemExit('usage: models.py OUT TARGET NETWORK COMPILER')
    prepare(pathlib.Path(sys.argv[1]).resolve(), *sys.argv[2:])
