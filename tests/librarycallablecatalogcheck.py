#!/usr/bin/env python3
"""Independent USLCALL1/2 exact wire and context-owned graph transaction checks."""
import argparse
import os
import platform
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--arch',choices=['arm64','x86_64']);o=p.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=o.arch or host
flags=['-arch',arch] if platform.system()=='Darwin' else []
if arch!=host and platform.system()!='Darwin':raise SystemExit('cross execution requires native runner')
u=lambda n:struct.pack('<Q',n)
def desc(k,w,a):return b''.join(u(n) for n in (0,0,0,k,w,0,a))+bytes([0])+u(0)
I=desc(1,4,4);D=desc(3,8,8);F=desc(3,4,4);C=desc(1,1,1)
def sig(args=(I,),result=I,var=0,mode=1):
 name=b'catalog';return b'USLSIG2\n'+u(1)+u(len(name))+name+bytes([0,1,var,mode])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([0 if var else 1])
FIXED=sig(mode=0);PROTO=sig(var=1);CONCRETE=sig((I,D));ZERO=sig()
def proto(key,record):return u(key)+u(len(record))+record
def site(number=100,key=22,fixed=1,record=CONCRETE,payload_delta=0,length_delta=0):
 return u(32+len(record)+payload_delta)+u(number)+u(key)+u(fixed)+u(len(record)+length_delta)+record
def pack(version=2,protos=((22,PROTO),(23,FIXED)),sites=None):
 b=('USLCALL'+str(version)+'\n').encode()+u(len(protos))+b''.join(proto(k,s) for k,s in protos)
 if version==2:
  sites=sites if sites is not None else [site(),site(101,record=ZERO)];b+=u(len(sites))+b''.join(sites)
 return b
one=pack(1,((11,FIXED),));two=pack()
bad=[pack(sites=[site(0)]),pack(sites=[site(key=999)]),pack(sites=[site(),site()]),
 pack(sites=[site(fixed=2)]),pack(sites=[site(key=23)]),pack(sites=[site(record=sig(mode=0))]),
 pack(sites=[site(record=sig(var=1))]),pack(sites=[site(record=sig(()))]),
 pack(sites=[site(record=sig((D,D)))]),pack(sites=[site(record=sig((I,D),D))]),
 pack(sites=[site(record=sig((I,F)))]),pack(sites=[site(record=sig((I,C)))]),
 pack(sites=[site(payload_delta=1)])+b'x',pack(sites=[site(length_delta=1)]),
 pack(protos=((22,PROTO),(22,FIXED))),pack(protos=(),sites=[site()]),one+u(0),
 b'USLCALL2\n'+u(8193),b'USLCALL2\n'+u(0)+u(8193),two+b'x',
 pack(sites=[site(record=b'USLSIG1\n'+u(0))]),pack(sites=[site(number=1,record=CONCRETE[:65])])]
def run(*cmd):subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,cmd)],check=True)
with tempfile.TemporaryDirectory(prefix='r10-callcatalog-') as tmp:
 t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir()
 for f in (ROOT/'exec/c').iterdir():
  if f.is_file() and f.suffix in ('.c','.h','.S'):shutil.copy2(f,rt/f.name)
 shutil.copy2(ROOT/'src/host_dl.h',t/'src/host_dl.h')
 files=[]
 for i,data in enumerate([one,two,*bad]):
  f=t/(str(i)+'.catalog');f.write_bytes(data);files.append(f)
 for sanitized in (False,True):
  exe=t/'probe';run(os.environ.get('CC','cc'),*flags,'-std=c11','-O1','-I',t,'-Wno-unused-function',ROOT/'tests/librarycallablecatalogcheck.c',rt/('librarycall_'+arch+'.S'),'-lffi',*(['-ldl'] if platform.system()!='Darwin' else []),'-o',exe,*(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if sanitized else []))
  run(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),exe,*files)
print('callable catalogue context: normal + ASan/UBSan passed on '+arch)
