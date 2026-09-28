#!/usr/bin/env python3
"""Complete native signatures through the real public library, private sources.
This proves fixed scalar/Pair bidirectional calls, not full variadic/callback FFI.
"""
import argparse,pathlib,platform,shutil,subprocess,tempfile,json,hashlib,os,struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
def command(args):
 p=subprocess.run(list(map(str,args)),capture_output=True,timeout=20)
 if p.returncode:raise AssertionError((args,p.returncode,p.stdout.decode(),p.stderr.decode()))
 return p.stdout.decode()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True);ap.add_argument('--arch',choices=['arm64','x86_64']);a=ap.parse_args()
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64'
 arch=a.arch or host
 if arch!=host and platform.system()!='Darwin':raise AssertionError('cross-native execution requires an explicit platform runner')
 flags=['-arch',arch] if platform.system()=='Darwin' else []
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 package=pathlib.Path(a.package).resolve()
 with tempfile.TemporaryDirectory(prefix='r10-sig2-native-') as name:
  td=pathlib.Path(name);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir()
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.S'):shutil.copy2(p,runtime/p.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h')
  lib=td/'library.dylib';command(['cc',*flags,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',runtime/'libunisacc.c',runtime/('librarycall_'+arch+'.S'),'-lffi','-o',lib])
  probe=td/'probe';command(['cc',*flags,'-std=c11','-O2','-Wall','-Wextra','-I',runtime,ROOT/'tests/libraryabi/nine_mixed_bidirectional.c',lib,'-o',probe])
  u=lambda n:struct.pack('<Q',n)
  def desc(kind,width,align,depth=0,tag=0,payload=b''):
   return b''.join(u(n) for n in (depth,0,0,kind,width,0,align))+bytes([tag])+u(len(payload))+payload
  I=desc(1,4,4);L=desc(1,8,8);D=desc(3,8,8);F=desc(3,4,4);P=desc(2,8,8,1)
  pair=desc(5,16,8,tag=1,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(8)+u(0)+u(0)+u(4)+I)
  for nm,result,args in [('host_exchange9',pair,[I,D,F,P,pair,I,D,I,I]),('host_double9',D,[D]*9),('host_float9',F,[F]*9),('host_integers17',L,[L]*17)]:
   name=nm.encode();signature=b'USLSIG2\n'+u(1)+u(len(name))+name+bytes([0,1,0,1])+u(len(args))+result+u(len(args))+b''.join(args)+bytes([1]);(td/(nm+'.sig')).write_bytes(signature)
  output=command([*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),probe,package,target,td]);print(output,end='')
  print(json.dumps({'target':target,'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest(),'runtime_sha256':hashlib.sha256(lib.read_bytes()).hexdigest(),'optimisation_levels':[0,1,2],'calls_per_level':100,'scope':'actual public us_sym native->script->typed native fixed FP/GP17/Pair9 return; not complete variadic/callback ABI'}))
if __name__=='__main__':main()
