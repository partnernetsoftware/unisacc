#!/usr/bin/env python3
"""Prepared U0 and three nativeboot generations, only the actual native Linux, Darwin or Windows target."""
import argparse,hashlib,json,os,pathlib,platform,shutil,subprocess,sys,tempfile,time
sys.dont_write_bytecode=True
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance,sourceflat
from models import digest
p=argparse.ArgumentParser(description=__doc__);p.add_argument('phase',choices=('prepare','u0','n1','n2','n3'));a=p.parse_args();start=time.monotonic()
WINDOWS=platform.system()=='Windows'
if platform.system() not in ('Linux','Darwin','Windows'):
 print('PLATFORM_UNVERIFIED host-triple requires native Linux, Darwin or Windows',file=sys.stderr);raise SystemExit(77)
bash='bash';cygpath=None
if WINDOWS:
 git=pathlib.Path(shutil.which('git') or '').resolve(strict=True)
 for ancestor in list(git.parents)[:4]:
  try:(ancestor/'bin/bash.exe').stat();(ancestor/'usr/bin/cygpath.exe').stat()
  except FileNotFoundError:continue
  bash=str(ancestor/'bin/bash.exe');cygpath=ancestor/'usr/bin/cygpath.exe';break
 if cygpath is None:raise SystemExit('WINDOWS_TOOL_NOT_READY Git for Windows Bash')
native=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'3',bash,(ROOT/'tests/nativeboot.sh').as_posix(),'--host-triple-target'],capture_output=True,text=True,timeout=5)
if native.returncode:
 sys.stderr.write(native.stderr);raise SystemExit(native.returncode)
target=native.stdout.strip()
if WINDOWS:import msvcrt
else:import fcntl
base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
def windows_tool():
 arch={'win/x86_64':'x64','win/arm64':'arm64'}[target]
 vswhere=pathlib.Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
 vs=pathlib.Path(subprocess.check_output([str(vswhere),'-latest','-products','*','-property','installationPath'],timeout=5).decode().strip())
 candidates=[vs/'VC/Tools/Llvm'/sub/'bin/clang-cl.exe' for sub in (('ARM64','x64','') if arch=='arm64' else ('x64',''))]
 compiler=None
 for candidate in candidates:
  try:candidate.stat()
  except FileNotFoundError:continue
  compiler=candidate.resolve(strict=True);break
 if compiler is None:raise SystemExit('WINDOWS_TOOL_NOT_READY clang-cl')
 def newest(folder,predicate):
  items=[x for x in folder.iterdir() if all(c.isdigit() or c=='.' for c in x.name) and predicate(x)]
  if not items:raise SystemExit('WINDOWS_TOOL_NOT_READY SDK/MSVC')
  return max(items,key=lambda x:tuple(map(int,x.name.split('.'))))
 msvc=newest(vs/'VC/Tools/MSVC',lambda x:(x/'include').is_dir() and (x/'lib'/arch).is_dir())
 sdk=pathlib.Path(os.environ['ProgramFiles(x86)'])/'Windows Kits/10'
 inc=newest(sdk/'Include',lambda x:all((x/n).is_dir() for n in ('ucrt','shared','um','winrt')) and all((sdk/'Lib'/x.name/n/arch).is_dir() for n in ('ucrt','um')))
 includes=[msvc/'include']+[inc/n for n in ('ucrt','shared','um','winrt')]
 libs=[msvc/'lib'/arch]+[sdk/'Lib'/inc.name/n/arch for n in ('ucrt','um')]
 env=dict(os.environ)
 for k in list(env):
  if k.upper() in ('INCLUDE','LIB','CL','_CL_','LINK','_LINK_'):del env[k]
 env.update(INCLUDE=';'.join(map(str,includes)),LIB=';'.join(map(str,libs)))
 linker=compiler.with_name('lld-link.exe').resolve(strict=True)
 ctx={'compiler':str(compiler),'linker':str(linker),'linker_sha256':digest(linker),'INCLUDE':env['INCLUDE'],'LIB':env['LIB'],'arch':arch}
 return compiler,env,ctx
compiler_env=dict(os.environ);windows_context=None
if WINDOWS:cc,compiler_env,windows_context=windows_tool()
else:cc=pathlib.Path(shutil.which('cc')).resolve(strict=True)
version=subprocess.run([str(cc),'--version'],capture_output=True,check=True,timeout=2,env=compiler_env).stdout
from actorclosure import actor_closure
def host_actor_closure():
 return actor_closure(ROOT,['tests/hosttriplecheck.py','tests/nativeboot.sh','tests/build_ref.sh','tests/sourceflat.py','tests/bound.py']+(['tests/refshim.h','tests/reffoot.h'] if WINDOWS else []))
sources=provenance.source_digest();actors,actor_edges=host_actor_closure()
assert {'tests/lib.sh','tests/bound','tests/bound.c'}<=set(actors)

basis={'sources':sources,'actors':actors,'host':target,'cc':str(cc),'cc_sha256':digest(cc),'cc_version':version.hex(),'CFLAGS':'-O2','native_flags':['-O2'],'compiler_environment':{k:os.environ.get(k) for k in ('PATH','CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SDKROOT','MACOSX_DEPLOYMENT_TARGET','SOURCE_DATE_EPOCH','CC','BOUND_CACHE','GITHUB_ACTIONS','UNISACC_BOUND_STOP_SECS','UNISACC_BOUND_TEST_NO_KILL')}}
if WINDOWS:
 basis['windows_tool']=windows_context
 basis['windows_shell']={'bash':bash,'bash_sha256':digest(pathlib.Path(bash)),'cygpath':str(cygpath),'cygpath_sha256':digest(pathlib.Path(cygpath))}
key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest();u0=base/('host-triple-u0-'+key)
def need(name,reason):
 print('NEED_PREPARE host-triple-'+name+': '+reason,file=sys.stderr);raise SystemExit(75)
def role(stage,self_sha,parent_sha):return {'stage':stage,'target':target,'identity':key,'self_sha256':self_sha,'parent_sha256':parent_sha,'native_flags':['-O2']}
def check(folder,name,expected):
 try:
  record=provenance.verify(folder/(name+'.exe' if WINDOWS else name))
  if record.get('host_triple')!=expected:need(name.lower(),'generation or parent identity mismatch')
 except SystemExit as e:
  if e.code==75:raise
  need(name.lower(),str(e))
 return record

def attest(path,expected):
 provenance.write(path,sources)
 rec=pathlib.Path(str(path)+'.build.json');j=json.loads(rec.read_text());j['host_triple']=expected;rec.write_text(json.dumps(j,sort_keys=True,indent=2)+'\n')
def publish(work,dest):
 dest.mkdir(parents=True,exist_ok=True)
 for f in work.iterdir():os.replace(f,dest/f.name)
def lock(dest):
 base.mkdir(parents=True,exist_ok=True);f=pathlib.Path(str(dest)+'.lock').open('a')
 try:
  if WINDOWS:
   f.close();f=pathlib.Path(str(dest)+'.lock').open('a+b');f.seek(0,2)
   if f.tell()==0:f.write(b'0');f.flush()
   f.seek(0);msvcrt.locking(f.fileno(),msvcrt.LK_NBLCK,1)
  else:fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB)
 except OSError:f.close();need(a.phase,'producer is in progress')
 return f

command_times=[]
def run(args,bound,stdout=None):
 t=time.monotonic()
 with (stdout.open('wb') if stdout is not None else tempfile.TemporaryFile()) as output:
  q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),str(bound),*map(str,args)],cwd=ROOT,stdout=output,stderr=subprocess.PIPE,timeout=bound+2,env=dict(compiler_env,CFLAGS='-O2',PYTHONDONTWRITEBYTECODE='1'))
 command_times.append({'argv':list(map(str,args)),'seconds_monotonic':time.monotonic()-t,'rc':q.returncode})
 if q.returncode:
  sys.stderr.buffer.write(q.stderr);raise SystemExit(q.returncode)
 return q

def verify_u0():
 try:sourceflat.verify_source(u0/'self.c')
 except (FileNotFoundError,ValueError,KeyError,json.JSONDecodeError) as e:need('u0',str(e))
 selfsha=digest(u0/'self.c');check(u0,'U0',role('u0',selfsha,digest(cc)));return selfsha
if a.phase=='prepare':
 with lock(u0):
  valid=False
  try:verify_u0();valid=True
  except SystemExit as e:
   if e.code!=75:raise
  if not valid:
   with tempfile.TemporaryDirectory(prefix='host-triple-build-',dir=base) as td:
    work=pathlib.Path(td)
    if WINDOWS:
     before=sourceflat.identity();flat=work/'self.c'
     posix=subprocess.check_output([str(cygpath),'-u',str(flat)],timeout=2).decode().strip()
     run([bash,'--noprofile','--norc','tests/export_ref.sh',posix],8)
     if flat.stat().st_size==0 or sourceflat.identity()!=before:raise SystemExit('sourceflat: empty source or inputs changed during export')
     pathlib.Path(str(flat)+'.source.json').write_text(json.dumps({'source':before,'flat':sourceflat.digest(flat)})+'\n')
     sourceflat.verify_source(flat)
    else:sourceflat.export_source(work/'self.c')
    if WINDOWS:
     wrapped=work/'U0.ref.c';wrapped.write_bytes((ROOT/'tests/refshim.h').read_bytes()+(work/'self.c').read_bytes()+(ROOT/'tests/reffoot.h').read_bytes())
     triple={'win/x86_64':'x86_64-pc-windows-msvc','win/arm64':'aarch64-pc-windows-msvc'}[target]
     run([cc,'--target='+triple,'/TC','/O2','/clang:-std=c99','/clang:-fuse-ld=lld',wrapped,'/Fo'+str(work/'U0.obj'),'/Fe'+str(work/'U0.exe')],55)
    else:run(['sh',ROOT/'tests/build_ref.sh',work/'U0.ref.c',work/'U0'],55)
    attest(work/('U0.exe' if WINDOWS else 'U0'),role('u0',digest(work/'self.c'),digest(cc)));publish(work,u0)
  selfsha=verify_u0()
 print(json.dumps({'status':'PREPARED_U0','scope':'NATIVE_'+target.replace('/','_').upper()+'_ONLY','sources':sources,'identity':key,'U0':str(u0/('U0.exe' if WINDOWS else 'U0')),'self_sha256':selfsha,'seconds_monotonic':time.monotonic()-start,'preparation_not_test':True,'actors':actors,'actor_edges':actor_edges},sort_keys=True));raise SystemExit(0)
selfsha=verify_u0();chain=[(u0,'U0')];parent_sha=digest(u0/('U0.exe' if WINDOWS else 'U0'));dirs={}
for n in range(1,int(a.phase[-1])+1 if a.phase!='u0' else 1):
 name='N'+str(n);stage=name.lower();dest=base/('host-triple-'+stage+'-'+hashlib.sha256((key+parent_sha).encode()).hexdigest());expected=role(stage,selfsha,parent_sha)
 if stage!=a.phase:
  check(dest,name,expected)
 else:
  with lock(dest):
   with tempfile.TemporaryDirectory(prefix='host-triple-generation-',dir=base) as td:
    work=pathlib.Path(td);previous=chain[-1][0]/(chain[-1][1]+'.exe' if WINDOWS else chain[-1][1]);before=digest(previous)
    executable=work/(name+'.parent-executable'+('.exe' if WINDOWS else ''))
    if WINDOWS:shutil.copyfile(previous,executable)
    else:run(['bash',ROOT/'tests/nativeboot.sh','--host-triple-runnable',previous,executable],9)
    if digest(previous)!=before:raise SystemExit('host-triple signing changed original parent')
    execution_sha=digest(executable)
    run([executable,u0/'self.c','-O2','-b',target],9,stdout=work/(name+'.exe')) if WINDOWS else run(['bash',ROOT/'tests/nativeboot.sh','--host-triple-build',u0/'self.c',executable,work/name],9)
    if digest(executable)!=execution_sha:raise SystemExit('host-triple execution copy changed during compilation')
    (work/(name+'.execution.json')).write_text(json.dumps({'parent_sha256':before,'execution_sha256':execution_sha,'target':target,'original_and_execution_copy_separate':True},sort_keys=True)+'\n')
    executable.unlink()
    if digest(previous)!=before:raise SystemExit('host-triple parent changed during compilation')
    if stage=='n3':
     if WINDOWS:
      if not (chain[1][0]/'N1.exe').read_bytes()==(chain[2][0]/'N2.exe').read_bytes()==(work/'N3.exe').read_bytes():raise SystemExit('host-triple native images differ')
     else:
      run(['cmp','--',chain[1][0]/'N1',chain[2][0]/'N2'],1)
      run(['cmp','--',chain[2][0]/'N2',work/'N3'],1)
    attest(work/(name+'.exe' if WINDOWS else name),expected);publish(work,dest)
  check(dest,name,expected)
 chain.append((dest,name));parent_sha=digest(dest/(name+'.exe' if WINDOWS else name));dirs[name]=str(dest/(name+'.exe' if WINDOWS else name))
print(json.dumps({'status':'PASS','scope':'NATIVE_'+target.replace('/','_').upper()+'_ONLY_NOT_SIX_TARGETS_OR_PRODUCT_PACK','phase':a.phase,'identity':key,'images':{name:digest(folder/(name+'.exe' if WINDOWS else name)) for folder,name in chain},'seconds_monotonic':time.monotonic()-start,'three_images_compared':a.phase=='n3','command_times':command_times,'missing_predecessor_never_constructs':True,'actors':actors,'actor_edges':actor_edges},sort_keys=True))
