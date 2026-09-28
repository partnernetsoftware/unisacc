#!/usr/bin/env python3
"""Complete native signatures through the real public library, private sources.
This proves host->script fixed scalar/Pair cases, not full bidirectional FFI.
"""
import argparse,pathlib,platform,shutil,subprocess,tempfile,json,hashlib,os
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
  probe=td/'probe';command(['cc',*flags,'-std=c11','-O2','-Wall','-Wextra','-I',runtime,ROOT/'tests/libraryabi/nine_mixed_native.c',lib,'-o',probe])
  output=command([*(['arch','-x86_64'] if host=='arm64' and arch=='x86_64' else []),probe,package,target]);print(output,end='')
  print(json.dumps({'target':target,'package_sha256':hashlib.sha256(package.read_bytes()).hexdigest(),'runtime_sha256':hashlib.sha256(lib.read_bytes()).hexdigest(),'optimisation_levels':[0,1,2],'calls_per_level':100,'scope':'actual public us_sym native->script fixed FP/GP17/Pair9 return; not complete bidirectional/variadic/callback ABI'}))
if __name__=='__main__':main()
