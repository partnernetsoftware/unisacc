#!/usr/bin/env python3
"""Fresh private full source export plus canonical/dependency provenance."""
import hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def digest(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def identity():
    paths=[ROOT/'unisacc.c',ROOT/'tests/export_ref.sh',Path(__file__).resolve()]
    for name in ('src','kernel'):
        paths.extend(p for p in sorted((ROOT/name).rglob('*')) if p.is_file() and p.suffix in ('.c','.h','.inc'))
    h=hashlib.sha256()
    for p in paths: h.update(str(p.relative_to(ROOT)).encode()+b'\0'+bytes.fromhex(digest(p)))
    return h.hexdigest()
def export_source(output):
    output=Path(output).resolve(); before=identity()
    if output==ROOT/'unisacc.c': raise ValueError('cannot overwrite canonical source')
    subprocess.run(['sh',str(ROOT/'tests/export_ref.sh'),str(output)],check=True,timeout=8,cwd=ROOT)
    if output.stat().st_size==0 or identity()!=before: raise ValueError('empty source or inputs changed during export')
    Path(str(output)+'.source.json').write_text(json.dumps(dict(source=before,flat=digest(output)))+'\n')
    return output
def verify_source(output):
    output=Path(output); r=json.loads(Path(str(output)+'.source.json').read_text())
    if r['source']!=identity() or r['flat']!=digest(output): raise ValueError('stale or damaged flat source')
    return output
if __name__=='__main__':
    try:
        if len(sys.argv)!=2: raise ValueError('usage: sourceflat.py OUTPUT')
        export_source(sys.argv[1])
    except (OSError,ValueError,KeyError,subprocess.SubprocessError) as e:
        print('sourceflat: FAIL '+str(e),file=sys.stderr); sys.exit(1)
