#!/usr/bin/env python3
"""Build host-native POSIX static/dynamic libunisacc from unchanged runtime.
Host C11 compiler, assembler, archiver and libffi development files are explicit
build dependencies. This is not a six-target cross-library builder. The model
package is delivered separately beside the libraries and passed to us_new().
"""
import argparse,hashlib,json,os,pathlib,platform,shlex,shutil,subprocess,tempfile,time
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=pathlib.Path(__file__).resolve().parents[2]
def host_target():
 osname={'Darwin':'osx','Linux':'lnx'}.get(platform.system())
 arch={'arm64':'arm64','aarch64':'arm64','x86_64':'x86_64','amd64':'x86_64'}.get(platform.machine().lower())
 if not osname or not arch:raise ValueError('only host-native POSIX arm64/x86_64 libraries supported')
 return osname+'/'+arch

def provider_flags(directory,target):
 directory=pathlib.Path(directory).resolve(strict=True)
 manifest=json.loads((directory/'manifest.json').read_text())
 if not isinstance(manifest,dict):raise ValueError('invalid FFI provider manifest')
 if manifest.get('schema')!=1 or manifest.get('target')!=target or manifest.get('version')!='3.5.2' or manifest.get('source_tar_sha256')!='f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc':raise ValueError('unqualified FFI provider identity')
 artifacts=manifest.get('artifacts',{})
 required=('lib/libffi.a','include/ffi.h','include/ffitarget.h','include/ffi/ffi.h','include/ffi/ffitarget.h','LICENSE')
 if not isinstance(artifacts,dict) or not all(name in artifacts and isinstance(artifacts[name],dict) for name in required):raise ValueError('incomplete FFI provider')
 for name in required:
  item=artifacts[name];p=directory/name
  if p.stat().st_size!=item.get('bytes') or sha(p)!=item.get('sha256'):raise ValueError('FFI provider content changed: '+name)
 return ['-I'+str(directory/'include')],[str(directory/'lib/libffi.a')],manifest

def build(package,out,cc,ar,cflags,ldflags,target=None,provider=None):
 native=host_target();host=target or native
 if host!=native and not (native.startswith('osx/') and host in ('osx/arm64','osx/x86_64')):raise ValueError('cross-target library build is not implemented')
 if host.startswith('osx/'):cc=cc+['-arch',host.split('/')[1]]
 provider_manifest=None
 if provider:
  if cflags or ldflags!=['-lffi']:raise ValueError('FFI provider cannot be mixed with explicit FFI flags')
  cflags,ldflags,provider_manifest=provider_flags(provider,host)
 package=pathlib.Path(package).resolve();out=pathlib.Path(out).resolve()
 from targetpackage import package_bytes
 package_bytes(package.read_bytes()) # validate carried P1/P2/P3 model envelope
 package_sha=sha(package);out.mkdir(parents=True,exist_ok=True)
 sources=sorted(p for p in (ROOT/'exec/c').iterdir() if p.is_file() and p.suffix in ('.c','.h','.S'))+[ROOT/'src/host_dl.h']
 before={str(p.relative_to(ROOT)):sha(p) for p in sources};commands=[]
 builder_paths=[pathlib.Path(__file__).resolve(),ROOT/'exec/c/targetpackage.py',ROOT/'exec/c/packageformat.py',ROOT/'exec/c/networkformat.py',ROOT/'tests/libraryabi/ffi-provider.c']
 builder_sources={str(p.relative_to(ROOT)):sha(p) for p in builder_paths}
 def run(args):
  t=time.monotonic();r=subprocess.run(list(map(str,args)),capture_output=True,timeout=30,env=dict(os.environ,ZERO_AR_DATE='1'))
  commands.append({'argv':list(map(str,args)),'rc':r.returncode,'seconds':time.monotonic()-t,'stdout':r.stdout.decode(errors='replace'),'stderr':r.stderr.decode(errors='replace')})
  if r.returncode:raise RuntimeError(json.dumps(commands[-1]))
  return r.stdout
 with tempfile.TemporaryDirectory(prefix='unisacc-library-build-',dir=out) as folder:
  d=pathlib.Path(folder);runtime=d/'exec/c';runtime.mkdir(parents=True);(d/'src').mkdir()
  for p in sources:shutil.copy2(p,d/p.relative_to(ROOT))
  arch=host.split('/')[1];common=cc+['-std=c11','-O2','-fPIC','-fvisibility=hidden']+cflags
  # Execute the selected FFI backend, including mixed register exhaustion.
  # A version label or a successful linker is not ABI qualification.
  probe=d/'ffi-provider-probe'
  run(cc+['-std=c99','-O2']+cflags+[ROOT/'tests/libraryabi/ffi-provider.c']+ldflags+['-o',probe])
  invoke=['arch','-'+arch,str(probe)] if host!=native else [str(probe)]
  qualification=run(invoke).decode()
  events=[json.loads(line) for line in qualification.splitlines()]
  expected=('true_C_union','true_C_union_result','ffi_union','ffi_union_result','ffi_struct','ffi_struct_result')
  if tuple(x.get('stage') for x in events)!=expected or any(x.get('rc')!=0 for x in events):raise ValueError('FFI provider actual boundary qualification failed')
  run(common+['-c',runtime/'libunisacc.c','-o',d/'library.o'])
  run(cc+cflags+['-c',runtime/('librarycall_'+arch+'.S'),'-o',d/'bridge.o'])
  static=d/'libunisacc.a';run(ar+['rcs',static,d/'library.o',d/'bridge.o'])
  dynamic=d/('libunisacc.dylib' if host.startswith('osx/') else 'libunisacc.so')
  mode=['-dynamiclib','-Wl,-install_name,@rpath/libunisacc.dylib'] if host.startswith('osx/') else ['-shared','-Wl,-soname,libunisacc.so']
  run(cc+mode+[d/'library.o',d/'bridge.o']+ldflags+['-o',dynamic])
  shutil.copy2(runtime/'libunisacc.h',d/'libunisacc.h');shutil.copy2(package,d/'compiler.pkg')
  if before!={str(p.relative_to(ROOT)):sha(p) for p in sources} or builder_sources!={str(p.relative_to(ROOT)):sha(p) for p in builder_paths} or sha(package)!=package_sha:raise ValueError('source or model package changed during build')
  if provider and provider_flags(provider,host)[2]!=provider_manifest:raise ValueError('FFI provider changed during build')
  files=[static,dynamic,d/'libunisacc.h',d/'compiler.pkg']
  if provider:
   shutil.copy2(pathlib.Path(provider)/'lib/libffi.a',d/'libunisacc-ffi.a')
   shutil.copy2(pathlib.Path(provider)/'LICENSE',d/'LIBFFI-LICENSE')
   files.extend([d/'libunisacc-ffi.a',d/'LIBFFI-LICENSE'])
  manifest={'schema':1,'target':host,'scope':'host-native POSIX static and dynamic libraries; other targets not qualified','package_sha256':package_sha,'sources':before,'builder_sources':builder_sources,'dependencies':{'compiler':cc,'archiver':ar,'archive_environment':{'ZERO_AR_DATE':'1'},'ffi_compile_flags':cflags,'ffi_link_flags':ldflags,'runtime':'system C library and selected qualified libffi; no compiler needed by library consumers at runtime','provider':provider_manifest,'qualification_stdout':qualification,'ffi_link_inputs':{str(pathlib.Path(x).resolve()):sha(pathlib.Path(x)) for x in ldflags if not x.startswith('-')}},'commands':commands,'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in files}}
  for p in files:os.replace(p,out/p.name)
  (out/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 return manifest
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--package',required=True,type=pathlib.Path);ap.add_argument('--output',required=True,type=pathlib.Path);ap.add_argument('--target');ap.add_argument('--cc',default=os.environ.get('CC','cc'));ap.add_argument('--ar',default=os.environ.get('AR','ar'));ap.add_argument('--ffi-cflags',default='');ap.add_argument('--ffi-ldflags',default='-lffi');ap.add_argument('--ffi-provider',type=pathlib.Path);a=ap.parse_args()
 try:
  m=build(a.package,a.output,shlex.split(a.cc),shlex.split(a.ar),shlex.split(a.ffi_cflags),shlex.split(a.ffi_ldflags),a.target,a.ffi_provider);print(json.dumps({'target':m['target'],'artifacts':m['artifacts'],'package_sha256':m['package_sha256']},sort_keys=True))
 except (OSError,ValueError,RuntimeError,subprocess.SubprocessError) as e:ap.exit(1,'buildlibrary: '+str(e)+'\n')
