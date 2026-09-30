#!/usr/bin/env python3
"""Private exact declarations exercising capability gates, not callback execution."""
import argparse
import os
import platform
import struct
import subprocess
import tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import bounded as run
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--arch',choices=['arm64','x86_64']);o=p.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=o.arch or host
flags=['-arch',arch] if platform.system()=='Darwin' else []
if arch!=host and platform.system()!='Darwin':raise SystemExit('native runner required')
u=lambda n:struct.pack('<Q',n)
def desc(k,w,a,depth=0,tag=0,payload=b''):return b''.join(u(n) for n in (depth,0,0,k,w,0,a))+bytes([tag])+u(len(payload))+payload
I=desc(1,4,4);D=desc(3,8,8);F=desc(3,4,4);P=desc(2,8,8,1)
PAIR=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
def callback(payload=b''):return desc(4,8,8,1,4,payload)
def define(i,result,args):return callback(bytes([1,0])+u(i)+bytes([0,int(len(args)>6)])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([1]))
def ref(i):return callback(bytes([1,1])+u(i))
def pack(result,args,support=1):return b'USLSIG2\n'+u(1)+u(10)+b'host_drive'+bytes([0,1,0,int(len(args)>6)])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([support])
def graph(result=PAIR,last=I):
 leaf=define(2,result,[I,D,F,P,PAIR,I,D,I,last]);relay=define(1,PAIR,[leaf]);return pack(PAIR,[relay,ref(2)])
with tempfile.TemporaryDirectory(prefix='r10-callbackplan-') as tmp:
 t=Path(tmp);files=[]
 for i,b in enumerate([graph(),graph(last=D),graph(result=D),pack(I,[callback()],0),pack(I,[I])]):
  f=t/(str(i)+'.sig');f.write_bytes(b);files.append(f)
 for sanitized in (False,True):
  exe=t/'probe';run(os.environ.get('CC','cc'),*flags,'-std=c11','-I',ROOT,'-Wall','-Wextra','-Wno-unused-function',ROOT/'tests/librarycallbackplancheck.c','-lffi',*(['-ldl'] if platform.system()!='Darwin' else []),'-o',exe,*(['-fsanitize=address,undefined','-g'] if sanitized else []))
  run(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),exe,*files)
print('callback declaration/plan: normal + ASan/UBSan passed on '+arch)
