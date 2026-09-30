#!/usr/bin/env python3
"""Actual public source-certified bitfield calls; any rejection is failure."""
import argparse,hashlib,json,os,pathlib,platform,shutil,subprocess,tempfile,sys
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',type=pathlib.Path,default=pathlib.Path(os.environ.get('MODEL_COM',ROOT/'unisacc.com')));ap.add_argument('--arch',choices=('arm64','x86_64'));ap.add_argument('--ffi-provider',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if platform.system() not in ('Darwin','Linux'):raise ValueError('this native caller requires Darwin or Linux')
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 cflags=[];ldflags=['-lffi'];provider=a.ffi_provider or os.environ.get('UNISACC_FFI_X86_PROVIDER' if arch=='x86_64' else 'UNISACC_FFI_PROVIDER')
 if provider:
  sys.path.insert(0,str(ROOT/'exec/c'));from buildlibrary import provider_flags
  cflags,ldflags,manifest=provider_flags(provider,target)
 package=a.package.resolve(strict=True);e={'schema':1,'status':'failed','target':target,'scope':__doc__,'package_sha256':sha(package),'commands':[]}
 try:
  with tempfile.TemporaryDirectory(prefix='r10-source-bitfield-') as name:
   t=pathlib.Path(name);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir()
   sources=[p for p in (ROOT/'exec/c').iterdir() if p.is_file() and p.suffix in ('.c','.h','.S')]+[ROOT/'src/host_dl.h',ROOT/'tests/libraryabi/source-bitfield.c']
   before={str(p.relative_to(ROOT)):sha(p) for p in sources}
   for p in sources:
    to=t/('probe.c' if p.name=='source-bitfield.c' else p.relative_to(ROOT));shutil.copy2(p,to)
   frozen=t/'compiler.pkg';shutil.copy2(package,frozen)
   def run(command):
    cmd=[str(ROOT/'tests/bound'),'20',*map(str,command)];r=subprocess.run(cmd,capture_output=True,text=True,timeout=25);e['commands'].append({'command':cmd,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr});assert r.returncode==0,e['commands'][-1];return r.stdout
   executable=t/'probe';flags=['-arch',arch] if platform.system()=='Darwin' else []
   run([os.environ.get('CC','cc'),*flags,'-std=c11','-O2','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',f'-DSOURCE_TARGET="{target}"','-I'+str(rt),*cflags,t/'probe.c',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),*ldflags,'-o',executable])
   invoke=['arch','-'+arch,executable] if host!=arch else [executable]
   out=run([*invoke,frozen]);assert out=='source bitfields: 900 actual native calls; unknown modifier refused\n',out
   assert before=={str(p.relative_to(ROOT)):sha(p) for p in sources} and sha(package)==e['package_sha256']
   e.update(status='passed',source_closure=before,native_calls=900,optimisation_levels=[0,1,2],unknown_modifier_refused=True)
 finally:
  if a.evidence:a.evidence.write_text(json.dumps(e,indent=2)+'\n')
 print(json.dumps({'status':e['status'],'target':target,'native_calls':e.get('native_calls')}))
if __name__=='__main__':main()
