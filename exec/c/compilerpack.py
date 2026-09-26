#!/usr/bin/env python3
"""Declare compiler CLI routes using existing, shared stage networks.
Only construction: the runtime reads the resulting package without Python.
"""
import argparse
from pathlib import Path
import tempfile
from pack import build

def compiler_package(manifests, o1, includes):
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
        for suffix,last,opt in specs:
            for _,name,inp,out,model in stages:
                if name=='e4' and opt=='none': continue
                model=Path(o1).resolve() if name=='e4' and opt=='O1' else (path.parent/model).resolve()
                rows.append('\t'.join([target+'/'+suffix,name,inp,out,str(model)]))
                if name==last: break
    with tempfile.TemporaryDirectory(prefix='compiler-package-') as td:
        manifest=Path(td)/'routes.tsv';manifest.write_text('\n'.join(rows)+'\n')
        return build([manifest],[('006864722f',includes)])

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('-o',required=True,type=Path)
    ap.add_argument('--o1',required=True,type=Path)
    ap.add_argument('--include',required=True,type=Path)
    ap.add_argument('manifests',nargs='+',type=Path)
    a=ap.parse_args()
    try:
        payload=compiler_package(a.manifests,a.o1,a.include);a.o.write_bytes(payload)
    except (OSError,ValueError) as e: ap.exit(1,f'compilerpack: {e}\n')
    print(f'compiler package: {len(payload)} B')
