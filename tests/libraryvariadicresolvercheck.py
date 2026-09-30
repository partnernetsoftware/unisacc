#!/usr/bin/env python3
"""Frozen template candidates only; concrete model callsites are separate."""
import argparse,platform,subprocess,tempfile,struct
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import bounded as run
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--arch',choices=['arm64','x86_64']);o=ap.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=o.arch or host
flags=['-arch',arch] if platform.system()=='Darwin' else []
if arch!=host and platform.system()!='Darwin':raise SystemExit('cross execution requires a native runner')
def u(n):return struct.pack('<Q',n)
def desc(kind,width):return b''.join(u(x)for x in(0,0,0,kind,width,0,width))+b'\0'+u(0)
D=desc(3,8);I=desc(1,4)
sig=b'USLSIG2\n'+u(1)+u(8)+b'exchange'+bytes([0,1,1,1])+u(2)+D+u(2)+D+I+b'\0'
with tempfile.TemporaryDirectory(prefix='r10-varresolver-')as t:
 p=Path(t);s=p/'prototype';s.write_bytes(sig)
 for sanitize in(False,True):
  exe=p/'check';run('cc',*flags,'-std=c99','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function',ROOT/'tests/libraryvariadicresolvercheck.c','-lffi','-o',exe,*(['-fsanitize=address,undefined','-g']if sanitize else []))
  run(*(['arch','-x86_64']if host=='arm64'and arch=='x86_64'else []),exe,s)
print('variadic resolver normal + ASan/UBSan passed')
