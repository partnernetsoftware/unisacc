#!/usr/bin/env python3
"""Split fresh Python selfhost construction; no cached verdicts."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/pipeline')]
from models import closure,digest
from sourceflat import export_source,verify_source,identity as source_identity

def identity(ua):
    h=closure(hashlib.sha256()); h.update(bytes.fromhex(source_identity()))
    for p in (ROOT/'unisacc.c',Path(ua),Path(__file__),ROOT/'tests/selfhost.sh'):
        h.update(bytes.fromhex(digest(p)))
    return h.hexdigest()

def main():
    mode,state,ua=sys.argv[1:]; state=Path(state); state.mkdir(parents=True,exist_ok=True)
    src=identity(ua); record=state/'record.json'; tape=state/'self.tape'; image=state/'self.image'; flat=state/'unisacc.flat.c'
    if mode=='tape':
        record.unlink(missing_ok=True); image.unlink(missing_ok=True)
        export_source(flat)
        cmd=[sys.executable,'-m','unisa','tape',str(flat),'--target','osx/arm64','--drive','built']
    else:
        verify_source(flat)
        r=json.loads(record.read_text())
        if r['source']!=src or r['tape']!=digest(tape): raise ValueError('stale or damaged selfhost tape')
        if mode=='verify':
            if r.get('image')!=digest(image): raise ValueError('stale or damaged selfhost image')
            print(image); return
        if mode!='image': raise ValueError('unknown selfhost preparation')
        cmd=[sys.executable,'-m','unisa','compile',str(tape),'--from-tape','-o',str(image),'--target','osx/arm64','--drive','built']
    p=subprocess.run(cmd,cwd=ROOT,capture_output=True,timeout=45)
    if mode=='tape' and p.returncode==0: tape.write_bytes(p.stdout)
    else: sys.stdout.buffer.write(p.stdout)
    sys.stderr.buffer.write(p.stderr)
    if p.returncode!=0: raise ValueError('selfhost '+mode+' exited '+str(p.returncode))
    out=tape if mode=='tape' else image
    if out.stat().st_size==0 or identity(ua)!=src: raise ValueError('empty or changed preparation')
    r=dict(source=src,tape=digest(tape))
    if mode=='image':
        image.chmod(0o755)
        subprocess.run(['codesign','-f','-s','-',str(image)],check=True,timeout=8)
        r['image']=digest(image)
    record.write_text(json.dumps(r)+'\n')
    print('selfhost '+mode+' sealed')
if __name__=='__main__':
    try: main()
    except (OSError,ValueError,KeyError,subprocess.SubprocessError) as e:
        print('selfhost prepare: FAIL '+str(e),file=sys.stderr); sys.exit(1)
