#!/usr/bin/env python3
"""Owned host variadic templates and transactional model call declarations."""
import argparse, os, platform, struct, subprocess, tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import bounded as run
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--arch',choices=['arm64','x86_64']);options=parser.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';target=options.arch or host
if target!=host and platform.system()!='Darwin':raise SystemExit('cross execution requires a native runner')
flags=['-arch',target] if platform.system()=='Darwin' else []
def u(n): return struct.pack('<Q',n)
def desc(kind,width,align,tag=0,payload=b''):
    return b''.join(u(n) for n in (0,0,0,kind,width,0,align))+bytes([tag])+u(len(payload))+payload
I=desc(1,4,4); D=desc(3,8,8); F=desc(3,4,4); C=desc(1,1,1)
PAIR=desc(5,16,8,1,u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
def signature(name,result,args,variadic=0):
    n=name.encode()
    return b'USLSIG2\n'+u(1)+u(len(n))+n+bytes([0,1,variadic,1])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([0 if variadic else 1])
with tempfile.TemporaryDirectory(prefix='r10-callplans-') as tmp:
    t=Path(tmp);exe=t/'probe'
    specs=[('mixed',D,[D,I],1),('mixed',D,[D,I],0),('mixed',D,[D,I]+[I,D]*9,0),('pair',PAIR,[I],1),('pair',PAIR,[I,PAIR,D,I],0)]
    files=[]
    for i,(name,result,args,var) in enumerate(specs):
        f=t/f'{i}.sig';f.write_bytes(signature(name,result,args,var));files.append(f)
    for sanitized in (False,True):
        run(os.environ.get('CC','cc'),*flags,'-std=c99','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function',ROOT/'tests/librarycallplanscheck.c','-lffi','-o',exe,*(['-fsanitize=address,undefined','-g'] if sanitized else []))
        run(*(['arch','-x86_64'] if host=='arm64' and target=='x86_64' else []),exe,*files)
print('callsite host: normal + ASan/UBSan; real va_arg, aggregate returns, ownership and rollback passed')
