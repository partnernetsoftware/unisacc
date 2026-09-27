#!/usr/bin/env python3
"""Declare compiler CLI routes using existing, shared stage networks.
Only construction: the runtime reads the resulting package without Python.
"""
import argparse
from pathlib import Path
import tempfile,struct
import subprocess,sys
from pack import build

def compiler_package(manifests, o1, includes, kernels=None):
    mounts=[('006864722f',includes)]
    if kernels is not None:
        kernels=Path(kernels)
        if not kernels.is_dir() or {p.name for p in kernels.iterdir()}!={'arm64','x86_64'}:
            raise ValueError('kernel directory must contain arm64 and x86_64 only')
        for isa,arch in enumerate(('arm64','x86_64'),1):
            raw=(kernels/arch).read_bytes()
            if len(raw)<40 or raw[:8]!=b'UNIKERN1': raise ValueError('invalid kernel header')
            kind,entry,slot,length=struct.unpack_from('<4Q',raw,8)
            if (kind!=isa or length!=len(raw)-40 or entry>=length or slot+8>length
                    or slot%8 or any(raw[40+slot:48+slot])):
                raise ValueError('invalid kernel ISA or extent')
        mounts.append(('006b65726e656c2f',kernels))
    specs=[]
    for text in Path(__file__).with_name('compiler-routes.tsv').read_text().splitlines():
        if text and not text.startswith('#'): specs.append(text.split('\t'))
    rows=[];targets=set()
    for path in map(Path,manifests):
        stages=[x.split('\t') for x in path.read_text().splitlines() if x and not x.startswith('#')]
        if not stages or any(len(s)!=5 for s in stages): raise ValueError('invalid source route')
        target=stages[0][0]
        if target in targets or any(s[0]!=target for s in stages): raise ValueError('duplicate/mixed target')
        targets.add(target)
        names=[s[1] for s in stages]
        if names!=['e2','e1','e3','e4','lower','elf']: raise ValueError('unexpected image stages')
        for _,name,inp,out,model in stages[:2]:
            rows.append('\t'.join([target+'/unit',name,inp,out,str((path.parent/model).resolve())]))
        _,name,inp,out,model=stages[-1]
        rows.append('\t'.join([target+'/memory',name,inp,'memory-v1',str((path.parent/model).resolve())]))
        for suffix,last,opt in specs:
            for _,name,inp,out,model in stages:
                if name=='e4' and opt=='none': continue
                model=Path(o1).resolve() if name=='e4' and opt=='O1' else (path.parent/model).resolve()
                rows.append('\t'.join([target+'/'+suffix,name,inp,out,str(model)]))
                if name==last: break
    with tempfile.TemporaryDirectory(prefix='compiler-package-') as td:
        # Offline construction of the shared unit-framing network. Runtime
        # does not invoke these Python tools or parse declarations in C.
        here=Path(__file__).resolve().parent
        uj=Path(td)/'units.json';ut=Path(td)/'units.tbl';un=Path(td)/'units.net'
        for script,args in [(here.parent/'parse2/units.py',[uj]),(here/'tbl.py',[uj,ut]),(here/'net.py',[ut,un])]:
            subprocess.run([sys.executable,str(script),*map(str,args)],check=True,timeout=60)
        base=list(rows)
        for target in sorted(targets):
            for suffix,last,opt in specs:
                if suffix=='pp': continue
                route=target+'/multi/'+suffix
                rows.append('\t'.join([route,'units','units.typed','tokens.typed',str(un)]))
                for row in base:
                    cols=row.split('\t')
                    if cols[0]==target+'/'+suffix and cols[1] not in ('e2','e1'):
                        cols[0]=route;rows.append('\t'.join(cols))
        manifest=Path(td)/'routes.tsv';manifest.write_text('\n'.join(rows)+'\n')
        return build([manifest],mounts)

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('-o',required=True,type=Path)
    ap.add_argument('--o1',required=True,type=Path)
    ap.add_argument('--include',required=True,type=Path)
    ap.add_argument('--kernels',type=Path,help='explicit directory containing both ISA kernel blobs')
    ap.add_argument('manifests',nargs='+',type=Path)
    a=ap.parse_args()
    try:
        payload=compiler_package(a.manifests,a.o1,a.include,a.kernels);a.o.write_bytes(payload)
    except (OSError,ValueError) as e: ap.exit(1,f'compilerpack: {e}\n')
    print(f'compiler package: {len(payload)} B')
