#!/usr/bin/env python3
"""Host variadic-call plan only; no public/model variadic coverage claim."""
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
def signature(result,args):
    return b'USLSIG2\n'+u(1)+u(8)+b'exchange'+bytes([0,1,0,1])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([1])
with tempfile.TemporaryDirectory(prefix='r10-nativevar-') as tmp:
    t=Path(tmp);exe=t/'probe';sig=t/'call.sig'
    for sanitized in (False,True):
        run(os.environ.get('CC','cc'),*flags,'-std=c99','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function',ROOT/'tests/librarynativevariadiccheck.c','-lffi','-o',exe,*(['-fsanitize=address,undefined','-g'] if sanitized else []))
        for name,result,args,fixed in [('mixed',D,[D,I]+[I,D]*9,2),('zero',D,[D,I],2),('pair',PAIR,[I,PAIR,D,I],1)]:
            sig.write_bytes(signature(result,args));run(*(['arch','-x86_64'] if host=='arm64' and target=='x86_64' else []),exe,sig,name,fixed)
        for args,fixed in [([D,I,F],2),([D,I,C],2),([D,I],0),([D,I],3)]:
            sig.write_bytes(signature(D,args));run(*(['arch','-x86_64'] if host=='arm64' and target=='x86_64' else []),exe,sig,'reject',fixed)
print('variadic host: normal + ASan/UBSan; promoted mixed20, zero-tail, aggregate and rejection controls passed')
