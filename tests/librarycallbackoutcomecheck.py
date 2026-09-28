#!/usr/bin/env python3
"""Private public-API sticky outcome proof; not callback parameter ABI proof."""
import argparse
import os
import platform
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--package',required=True);p.add_argument('--arch',choices=['arm64','x86_64']);p.add_argument('--baseline',action='store_true');o=p.parse_args()
host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=o.arch or host
flags=['-arch',arch] if platform.system()=='Darwin' else []
target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
if arch!=host and platform.system()!='Darwin':raise SystemExit('native cross runner required')
def run(*cmd):
 return subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,cmd)],capture_output=True)
def check(*cmd):
 r=run(*cmd)
 if r.returncode:raise AssertionError((r.returncode,r.stdout.decode(),r.stderr.decode()))
 return r.stdout.decode()
u=lambda n:struct.pack('<Q',n)
def desc(k,w,a,tag=0,payload=b''):return b''.join(u(n) for n in (0,0,0,k,w,0,a))+bytes([tag])+u(len(payload))+payload
I=desc(1,4,4);D=desc(3,8,8);PAIR=desc(5,16,8,1,u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
with tempfile.TemporaryDirectory(prefix='r10-outcome-') as tmp:
 t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir()
 for f in (ROOT/'exec/c').iterdir():
  if f.is_file() and f.suffix in ('.c','.h','.S'):shutil.copy2(f,rt/f.name)
 shutil.copy2(ROOT/'src/host_dl.h',t/'src/host_dl.h')
 package=t/'package.com';shutil.copy2(Path(o.package).resolve(),package)
 sig=t/'host_drive.sig';sig.write_bytes(b'USLSIG2\n'+u(1)+u(10)+b'host_drive'+bytes([0,1,0,0])+u(1)+PAIR+u(1)+I+bytes([1]))
 for san in (False,True) if not o.baseline else (False,):
  sf=['-fsanitize=address,undefined','-g'] if san else []
  lib=t/'library.dylib';exe=t/'probe'
  if not o.baseline:
   unit=t/'arena-probe'
   check(os.environ.get('CC','cc'),*flags,*sf,'-std=c11','-O1','-DUS_OUTCOME_ARENA_PROBE','-I',t,ROOT/'tests/librarycallbackoutcomecheck.c','-lffi','-o',unit)
   print(check(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),unit,sig),end='')
  check(os.environ.get('CC','cc'),*flags,*sf,'-std=c11','-O1','-shared','-fPIC','-fvisibility=hidden',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',lib)
  check(os.environ.get('CC','cc'),*flags,*sf,'-std=c11','-O1','-Wall','-Wextra','-I',rt,ROOT/'tests/librarycallbackoutcomecheck.c',lib,'-o',exe)
  r=run(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),exe,package,target,sig)
  if o.baseline:
   assert r.returncode==1 and b'sticky first exit23' in r.stderr,(r.returncode,r.stdout,r.stderr)
   print('actual red baseline:',r.stderr.decode().strip())
  else:
   assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
   print(r.stdout.decode(),end='')
print('outcome '+('baseline rejected' if o.baseline else 'normal + ASan/UBSan passed')+' on '+arch)
