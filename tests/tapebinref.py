#!/usr/bin/env python3
"""Reference compiler for tests/tapebinproduct.py, built once per content key (0.0.43 T-1).

prepare  builds the reference from the current tree (export_ref + the shim and footer, as
         build_ref.sh does) into <cache>/<key>/ref with a manifest, or reuses a complete entry.
locate   returns that binary only when the manifest's key equals the key of the current tree
         and the binary's digest matches; a missing or stale entry is refused (rc 2), never
         rebuilt here.  The shards call locate(); com-tapebin-ref is their gate prerequisite.
key      prints the key.

The key covers the exported source, the shim and footer, build_ref.sh, CFLAGS and the first
line of `cc --version`, so an edit to any of them makes the old entry stale.
"""
import hashlib, json, os, pathlib, subprocess, sys, tempfile

R = pathlib.Path(__file__).resolve().parents[1]
ENTRY = 'manifest.json'


def cache_root():
    return pathlib.Path(os.environ.get('UNISACC_TAPEBIN_REF_CACHE', tempfile.gettempdir() + '/unisacc-tapebin-ref'))


def _sha(data):
    return hashlib.sha256(data).hexdigest()


def _file_sha(path):
    return _sha(pathlib.Path(path).read_bytes())


def _cflags():
    return os.environ.get('CFLAGS', '-O2')


def _cc_version():
    out = subprocess.run(['cc', '--version'], capture_output=True, timeout=10, check=True).stdout
    return out.decode(errors='replace').splitlines()[0]


def _exported(tmp):
    flat = tmp / 'flat.c'
    subprocess.run([str(R / 'tests/export_ref.sh'), str(flat)], cwd=R, check=True, timeout=30)
    return flat.read_bytes()


def key():
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='tapebinref-key-'))
    try:
        flat = _exported(tmp)
    finally:
        for p in tmp.glob('*'):
            p.unlink()
        tmp.rmdir()
    parts = [
        'tapebin-ref-v1', _sha(flat), _file_sha(R / 'tests/refshim.h'), _file_sha(R / 'tests/reffoot.h'),
        _file_sha(R / 'tests/build_ref.sh'), _cflags(), _cc_version(),
    ]
    return _sha(json.dumps(parts).encode())


def _valid(entry):
    try:
        manifest = json.loads((entry / ENTRY).read_text())
        ref = entry / 'ref'
        return manifest if manifest.get('ref_sha256') == _file_sha(ref) else None
    except (OSError, ValueError, KeyError):
        return None


def prepare():
    k = key()
    entry = cache_root() / k
    if _valid(entry):
        print('tapebin ref reuse key=%s' % k[:16])
        return 0
    cache_root().mkdir(parents=True, exist_ok=True)
    tmp = pathlib.Path(tempfile.mkdtemp(prefix='tapebinref-', dir=cache_root()))
    try:
        flat = tmp / 'flat.c'
        flat.write_bytes(_exported(tmp))
        source = tmp / 'ref.c'
        source.write_bytes((R / 'tests/refshim.h').read_bytes() + flat.read_bytes() + (R / 'tests/reffoot.h').read_bytes())
        ref = tmp / 'ref'
        subprocess.run(['cc', '-w', '-std=c99', *_cflags().split(), '-o', str(ref), str(source)],
                       check=True, timeout=50)
        manifest = {'key': k, 'ref_sha256': _file_sha(ref), 'bytes': ref.stat().st_size, 'cc': _cc_version(),
                    'cflags': _cflags()}
        (tmp / ENTRY).write_text(json.dumps(manifest, indent=1, sort_keys=True) + '\n')
        flat.unlink(); source.unlink()
        if not entry.exists():
            os.rename(tmp, entry)
        else:
            for p in tmp.glob('*'):
                p.unlink()
            tmp.rmdir()
    except BaseException:
        for p in tmp.glob('*'):
            p.unlink()
        tmp.rmdir()
        raise
    if not _valid(entry):
        raise SystemExit('tapebinref: prepared entry does not verify')
    print('tapebin ref prepared key=%s bytes=%d' % (k[:16], (entry / 'ref').stat().st_size))
    return 0


def locate():
    """Path to the prepared reference for the current tree, or SystemExit(2)."""
    k = key()
    entry = cache_root() / k
    manifest = _valid(entry)
    if not manifest or manifest.get('key') != k:
        raise SystemExit('tapebin reference not prepared (or stale) for key %s: run com-tapebin-ref first' % k[:16])
    return entry / 'ref'


def state_dir(product_sha):
    d = cache_root() / key() / 'observed' / product_sha[:16]
    d.mkdir(parents=True, exist_ok=True)
    return d


def main(argv):
    cmd = argv[1] if len(argv) > 1 else ''
    if cmd == 'prepare':
        return prepare()
    if cmd == 'locate':
        print(locate())
        return 0
    if cmd == 'key':
        print(key())
        return 0
    print('usage: tapebinref.py prepare|locate|key', file=sys.stderr)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
