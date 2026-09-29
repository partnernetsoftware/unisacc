#!/usr/bin/env python3
"""Check canonical trim resource, six target E2 outputs and the CLI alias.
Caller supplies a private C executor, rebuilt classic UA and the new E2 net.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'exec/pp'),str(ROOT/'exec/c')]
from gen import predefine_resources
from pack import build as package

def check(run,net,ua,out,targets=()):
    out.mkdir(parents=True,exist_ok=True)
    res=out/'resources';res.mkdir(exist_ok=True)
    for k,v in predefine_resources().items():
        f=res/k[1:].decode();f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(v)
    sel=res/'cli/target';sel.parent.mkdir(exist_ok=True);flag=res/'cli/fno-trim-libc'   # opt-out resource: absent = bodies on demand (R11-3)
    route=out/'route.tsv';route.write_text('pp\te2\tsrc.c\tpp.text\t'+str(net)+'\n')
    probes={'stdio':'#include <stdio.h>\nint main(void){printf("hi %d\\n",7);return 0;}\n',
            'stdlib':'#include <stdlib.h>\nint main(void){return atoi("0");}\n',
            'address':'#include <string.h>\nint main(void){unsigned long (*f)(const char *) = strlen;return f("abc")==3?0:1;}\n'}
    def call(args):
        r=subprocess.run(list(map(str,args)),cwd=out,timeout=10,capture_output=True)
        assert r.returncode==0,(args,r.returncode,r.stderr[:300])
        return r.stdout
    records=[]
    # E2 is target-bound at construction (gen.py OUT target): compare only the net's own target.
    for target in (targets if targets else ('lnx/x86_64',)):
        sel.write_bytes(target.encode())
        for on in (False,True):
            if not on:flag.write_bytes(b'1')
            else:
                try:flag.unlink()
                except FileNotFoundError:pass
            pkg=out/'pp.pkg';pkg.write_bytes(package([route],[('00',res),('006864722f',ROOT/'include')]))
            for name,text in probes.items():
                source=out/(name+'.c');source.write_text(text)
                model=call([run,'--bundle',pkg,'pp',source,source,ROOT/'include'])
                classic=call([ua,'-b',target,'-ftrim-libc' if on else '-fno-trim-libc','-E',source])   # default is on since R11-3
                assert model==classic,(target,on,name,'E2 differs')
                if on:assert call([ua,'-b',target,'-libneed','-E',source])==classic,(target,name,'alias differs')
                records.append([target,on,name,len(model)])
    (out/'result.json').write_text(json.dumps({'equal':records,'alias_equal':len(records)//2},indent=2)+'\n')
    print('ftrim-libc: %d E2 outputs equal (%d target(s) x 2 modes x 3 probes)'%(len(records),max(1,len(targets))))

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('run','net','ua','output'):ap.add_argument('--'+name,type=lambda p:Path(p).resolve(),required=True)
    ap.add_argument('--target',action='append',default=[],help='target the E2 net was constructed for (repeatable); default lnx/x86_64')
    a=ap.parse_args();check(a.run,a.net,a.ua,a.output,tuple(a.target))
