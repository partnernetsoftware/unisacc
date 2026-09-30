#!/usr/bin/env python3
"""Freeze host sources and a prepared package, then real C typed calls on macOS.
No shared UA or root product writes; each native child compile/exec <=20 seconds.
Run --package PRIVATE/compiler.pkg [--arch arm64|x86_64].
"""
import argparse,hashlib,json,pathlib,shutil,subprocess,tempfile,time
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',required=True,type=pathlib.Path);ap.add_argument('--arch',choices=('arm64','x86_64'),required=True);ap.add_argument('--evidence',type=pathlib.Path);a=ap.parse_args()
 assert a.package.is_absolute() and a.package.is_file()
 with tempfile.TemporaryDirectory(prefix='r10-exports-real-native-') as folder:
  d=pathlib.Path(folder);runtime=d/'exec/c';runtime.mkdir(parents=True);(d/'src').mkdir();records=[]
  for p in (ROOT/'exec/c').iterdir():
   if p.is_file() and p.suffix in ('.h','.c','.S'):
    shutil.copy2(p,runtime/p.name);records.append({'path':str(p.relative_to(ROOT)),'sha256':sha(runtime/p.name)})
  shutil.copy2(ROOT/'src/host_dl.h',d/'src/host_dl.h');records.append({'path':'src/host_dl.h','sha256':sha(d/'src/host_dl.h')})
  harness=d/'harness.c';shutil.copy2(ROOT/'tests/libunisaccexportnative.c',harness);records.append({'path':'tests/libunisaccexportnative.c','sha256':sha(harness)})
  package=d/'compiler.pkg';shutil.copy2(a.package,package);records.append({'path':'compiler.pkg','sha256':sha(package)})
  exe=d/'host';commands=[]
  for cmd in [['cc','-arch',a.arch,'-std=c11','-O2','-Wall','-Wextra','-I',runtime,harness,runtime/'libunisacc.c',runtime/('librarycall_'+a.arch+'.S'),'-lffi','-o',exe],
              (['/usr/bin/arch','-x86_64',exe,package,'osx/'+a.arch] if a.arch=='x86_64' else [exe,package,'osx/'+a.arch])]:
   started=time.monotonic();r=subprocess.run(list(map(str,cmd)),cwd=d,capture_output=True,timeout=20)
   commands.append({'argv':list(map(str,cmd)),'rc':r.returncode,'seconds':time.monotonic()-started,'stdout':r.stdout.decode(errors='replace'),'stderr':r.stderr.decode(errors='replace')});assert r.returncode==0,commands[-1]
  result={'scope':'true E3 metadata and native C typed function pointers; '+a.arch,'inputs':records,'commands':commands}
  if a.evidence:a.evidence.write_text(json.dumps(result,indent=2))
  print(json.dumps({'arch':a.arch,'package_sha256':sha(package),'output':commands[-1]['stdout'].strip(),'seconds':commands[-1]['seconds']}))
if __name__=='__main__':main()
