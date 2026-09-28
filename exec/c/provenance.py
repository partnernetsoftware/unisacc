#!/usr/bin/env python3
"""Build-time source and artifact identity; never attest an existing stale build."""
import hashlib, json, os, pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'exec/pipeline'))
from models import closure, digest

def source_digest():
    h = closure(hashlib.sha256())
    for p in sorted((ROOT / 'exec/c/asm').glob('*.S')):
        h.update(str(p.relative_to(ROOT)).encode() + b'\0')
        h.update(bytes.fromhex(digest(p)))
    return h.hexdigest()

def verify(path):
    path = pathlib.Path(path)
    try:
        record = json.loads(pathlib.Path(str(path) + '.build.json').read_text())
        if record['schema'] != 1 or record['sources_sha256'] != source_digest():
            raise ValueError('product inputs changed; rebuild with make com')
        if record['artifact_sha256'] != digest(path) or record['bytes'] != path.stat().st_size:
            raise ValueError('artifact differs from its build record')
    except (OSError, ValueError, KeyError, TypeError) as e:
        raise SystemExit(f'product freshness: {path}: {e}')
    return record

def write(path, expected):
    current = source_digest()
    if current != expected:
        raise SystemExit('product inputs changed during packaging')
    path = pathlib.Path(path)
    record = dict(schema=1, sources_sha256=current, artifact_sha256=digest(path),
                  bytes=path.stat().st_size,
                  commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, timeout=5).decode().strip(),
                  settings={k:v for k,v in os.environ.items() if k.startswith(('E1','E2','E3','E4','PYTHONHASHSEED'))})
    dest = pathlib.Path(str(path) + '.build.json')
    tmp = pathlib.Path(str(dest) + '.tmp')
    tmp.write_text(json.dumps(record, indent=2, sort_keys=True) + '\n')
    tmp.replace(dest)

if __name__ == '__main__':
    if sys.argv[1:] == ['identity']:
        print(source_digest())
    elif len(sys.argv) == 4 and sys.argv[1] == 'write':
        write(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3 and sys.argv[1] == 'check':
        r = verify(sys.argv[2]); print('product freshness: verified', r['artifact_sha256'])
    else:
        raise SystemExit('usage: provenance.py identity | write ARTIFACT START_DIGEST | check ARTIFACT')
