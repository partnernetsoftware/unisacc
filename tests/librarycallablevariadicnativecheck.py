#!/usr/bin/env python3
"""Actual model-produced indirect concrete variadic callable sites."""
import argparse
import hashlib
import json
import os
import platform
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--package',required=True);p.add_argument('--sanitize',action='store_true');p.add_argument('--arch',choices=['arm64','x86_64']);p.add_argument('--evidence');a=p.parse_args()
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if arch!=host and platform.system()!='Darwin':raise SystemExit('cross execution requires native runner')
 flags=['-arch',arch] if platform.system()=='Darwin' else [];target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 def run(*cmd):
  r=subprocess.run([str(ROOT/'tests/bound'),'20',*map(str,cmd)],capture_output=True)
  if r.returncode:raise AssertionError((r.returncode,r.stdout.decode(),r.stderr.decode()))
  return r.stdout.decode()
 with tempfile.TemporaryDirectory(prefix='r10-callable-var-native-') as tmp:
  t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir()
  for f in (ROOT/'exec/c').iterdir():
   if f.is_file() and f.suffix in ('.c','.h','.S'):shutil.copy2(f,rt/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',t/'src/host_dl.h');package=t/'compiler.pkg';shutil.copy2(Path(a.package).resolve(strict=True),package)
  u=lambda n:struct.pack('<Q',n)
  def desc(k,w,al,depth=0,tag=0,payload=b''):return b''.join(u(n) for n in (depth,0,0,k,w,0,al))+bytes([tag])+u(len(payload))+payload
  I=desc(1,4,4);D=desc(3,8,8);pair=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
  name=b'host_exchange';args=[D,I]
  signature=b'USLSIG2\n'+u(1)+u(len(name))+name+bytes([0,1,1,1])+u(2)+pair+u(2)+b''.join(args)+bytes([0])
  (t/'host_exchange.sig').write_bytes(signature)
  def concrete(name,args,result=pair,variadic=0):
   n=name.encode();return b'USLSIG2\n'+u(1)+u(len(n))+n+bytes([0,1,variadic,1])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([0])
  good=concrete('scriptvar',[D,I,D,I])
  shapes={'script-full.sig':good,'script-zero.sig':concrete('scriptvar',[D,I]),
          'script-wrongname.sig':concrete('other',[D,I,D,I]),
          'script-result.sig':concrete('scriptvar',[D,I,D,I],D),
          'script-prefix.sig':concrete('scriptvar',[I,I,D,I]),
          'script-float.sig':concrete('scriptvar',[D,I,desc(3,4,4),I]),
          'script-narrow.sig':concrete('scriptvar',[D,I,D,desc(1,1,1)]),
          'script-short.sig':concrete('scriptvar',[D]),
          'script-variable.sig':concrete('scriptvar',[D,I],variadic=1),
          'script-truncated.sig':good[:-1]}
  for filename,blob in shapes.items():(t/filename).write_bytes(blob)

  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else []
  lib=t/'library.dylib';probe=t/'probe'
  run(os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',lib)
  run(os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-Wall','-Wextra','-I',rt,ROOT/'tests/libraryabi/callable_variadic.c',lib,'-o',probe)
  output=run(*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),probe,package,target,t);print(output,end='')
  evidence={'target':target,'sanitize':a.sanitize,'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest(),'runtime_sha256':hashlib.sha256(lib.read_bytes()).hexdigest(),'optimisation_levels':[0,1,2],'repeats_per_level':100,'callback_exit_status':23,'explicit_export_specializations':['promoted double/int tail','zero-tail'],'invalid_specialization_declarations':8,'failure_checks':'native normal return, outer ABI zero/canary, independent next-call recovery','host_fixture_sha256':hashlib.sha256((ROOT/'tests/libraryabi/callable_variadic.c').read_bytes()).hexdigest(),'scope':'actual public native->script entry(Var) -> concrete indirect native varargs -> fixed script Leaf; frozen native template introduction; concrete script va_arg reads promoted double/int tails, zero-tail and nested site restoration; unknown-tail script closures not supported'}
  print(json.dumps(evidence,ensure_ascii=False))
  if a.evidence:Path(a.evidence).write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n')
if __name__=='__main__':main()
