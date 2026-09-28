#!/usr/bin/env python3
"""Build host-native POSIX static/dynamic libunisacc from unchanged runtime.
Host C11 compiler, assembler, archiver and libffi development files are explicit
build dependencies. This is not a six-target cross-library builder. The model
package is delivered separately beside the libraries and passed to us_new().
"""
import argparse,hashlib,json,os,pathlib,platform,shlex,shutil,subprocess,tempfile,time
ROOT=pathlib.Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def host_target():
 osname={'Darwin':'osx','Linux':'lnx'}.get(platform.system())
 arch={'arm64':'arm64','aarch64':'arm64','x86_64':'x86_64','amd64':'x86_64'}.get(platform.machine().lower())
 if not osname or not arch:raise ValueError('only host-native POSIX arm64/x86_64 libraries supported')
 return osname+'/'+arch

def build(package,out,cc,ar,cflags,ldflags,target=None):
 host=host_target()
 if target is not None and target!=host:raise ValueError('cross-target library build is not implemented')
 package=pathlib.Path(package).resolve();out=pathlib.Path(out).resolve()
 from targetpackage import package_bytes
 package_bytes(package.read_bytes()) # validate carried P1/P2/P3 model envelope
 package_sha=sha(package);out.mkdir(parents=True,exist_ok=True)
 sources=sorted(p for p in (ROOT/'exec/c').iterdir() if p.is_file() and p.suffix in ('.c','.h','.S'))+[ROOT/'src/host_dl.h']
 before={str(p.relative_to(ROOT)):sha(p) for p in sources};commands=[]
 builder_paths=[pathlib.Path(__file__).resolve(),ROOT/'exec/c/targetpackage.py',ROOT/'exec/c/packageformat.py',ROOT/'exec/c/networkformat.py']
 builder_sources={str(p.relative_to(ROOT)):sha(p) for p in builder_paths}
 def run(args):
  t=time.monotonic();r=subprocess.run(list(map(str,args)),capture_output=True,timeout=30,env=dict(os.environ,ZERO_AR_DATE='1'))
  commands.append({'argv':list(map(str,args)),'rc':r.returncode,'seconds':time.monotonic()-t,'stderr':r.stderr.decode(errors='replace')})
  if r.returncode:raise RuntimeError(json.dumps(commands[-1]))
  return r.stdout
 with tempfile.TemporaryDirectory(prefix='unisacc-library-build-',dir=out) as folder:
  d=pathlib.Path(folder);runtime=d/'exec/c';runtime.mkdir(parents=True);(d/'src').mkdir()
  for p in sources:shutil.copy2(p,d/p.relative_to(ROOT))
  arch=host.split('/')[1];common=cc+['-std=c11','-O2','-fPIC','-fvisibility=hidden']+cflags
  run(common+['-c',runtime/'libunisacc.c','-o',d/'library.o'])
  run(cc+cflags+['-c',runtime/('librarycall_'+arch+'.S'),'-o',d/'bridge.o'])
  static=d/'libunisacc.a';run(ar+['rcs',static,d/'library.o',d/'bridge.o'])
  dynamic=d/('libunisacc.dylib' if host.startswith('osx/') else 'libunisacc.so')
  mode=['-dynamiclib','-Wl,-install_name,@rpath/libunisacc.dylib'] if host.startswith('osx/') else ['-shared','-Wl,-soname,libunisacc.so']
  run(cc+mode+[d/'library.o',d/'bridge.o']+ldflags+['-o',dynamic])
  shutil.copy2(runtime/'libunisacc.h',d/'libunisacc.h');shutil.copy2(package,d/'compiler.pkg')
  if before!={str(p.relative_to(ROOT)):sha(p) for p in sources} or builder_sources!={str(p.relative_to(ROOT)):sha(p) for p in builder_paths} or sha(package)!=package_sha:raise ValueError('source or model package changed during build')
  files=[static,dynamic,d/'libunisacc.h',d/'compiler.pkg']
  manifest={'schema':1,'target':host,'scope':'host-native POSIX static and dynamic libraries; other targets not qualified','package_sha256':package_sha,'sources':before,'builder_sources':builder_sources,'dependencies':{'compiler':cc,'archiver':ar,'archive_environment':{'ZERO_AR_DATE':'1'},'ffi_compile_flags':cflags,'ffi_link_flags':ldflags,'runtime':'system C library and libffi; no compiler needed by library consumers at runtime'},'commands':commands,'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in files}}
  for p in files:os.replace(p,out/p.name)
  (out/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 return manifest
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--package',required=True,type=pathlib.Path);ap.add_argument('--output',required=True,type=pathlib.Path);ap.add_argument('--target');ap.add_argument('--cc',default=os.environ.get('CC','cc'));ap.add_argument('--ar',default=os.environ.get('AR','ar'));ap.add_argument('--ffi-cflags',default='');ap.add_argument('--ffi-ldflags',default='-lffi');a=ap.parse_args()
 try:
  m=build(a.package,a.output,shlex.split(a.cc),shlex.split(a.ar),shlex.split(a.ffi_cflags),shlex.split(a.ffi_ldflags),a.target);print(json.dumps({'target':m['target'],'artifacts':m['artifacts'],'package_sha256':m['package_sha256']},sort_keys=True))
 except (OSError,ValueError,RuntimeError,subprocess.SubprocessError) as e:ap.exit(1,'buildlibrary: '+str(e)+'\n')
