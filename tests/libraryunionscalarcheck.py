#!/usr/bin/env python3
"""True public fixed union scalar matrix; unsupported actual cases are red."""
from checklib import copy_runtime
import argparse,hashlib,json,os,platform,shutil,struct,subprocess,tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def descriptor(k,w,a,uns=0,tag=0,payload=b''):
 return b''.join(U(n) for n in (0,0,0,k,w,uns,a))+bytes([tag])+U(len(payload))+payload
# Independent exact member layout, no C carrier choice/classifier.
CASES={'int8':(0,8,8,[(1,8,1),(1,8,0)]),'float4':(1,4,4,[(3,4,0),(3,4,0)]),'double8':(2,8,8,[(3,8,0),(3,8,0)]),
 'mixed2':(3,8,8,[(3,8,0),(1,8,1)]),'mixed3':(4,8,8,[(3,8,0),(1,8,1),(1,8,0)]),'mixed-reordered':(5,8,8,[(1,8,1),(1,8,0),(3,8,0)]),
 'int1':(6,1,1,[(1,1,1),(1,1,0)]),'int2':(7,2,2,[(1,2,1),(1,2,0)]),'int4':(8,4,4,[(1,4,1),(1,4,0)])}
PRESSURES={'plain':0,'gp9':1,'fp9':2}
def tool_environment():
 # Cached native helpers use an explicit loader environment. The caller cannot
 # substitute runtime code via LD_LIBRARY_PATH/PRELOAD/AUDIT or loader tunables.
 return {k:v for k,v in os.environ.items() if not k.startswith('LD_') and k!='GLIBC_TUNABLES'}

def tool_context(arch,sanitize):
 import sys
 sys.path.insert(0,str(ROOT/'exec/c'))
 import provenance
 from models import digest
 if platform.system()!='Linux' or platform.machine()!='x86_64' or arch!='x86_64':
  print('PLATFORM_UNVERIFIED union scalar prepared tools: native lnx/x86_64 only',file=__import__('sys').stderr);raise SystemExit(77)
 cc=Path(shutil.which(os.environ.get('CC','cc')) or os.environ.get('CC','cc')).resolve(strict=True)
 version=subprocess.run([str(cc),'--version'],env=tool_environment(),capture_output=True,check=True,timeout=2).stdout.hex()
 from actorclosure import actor_closure
 actors,actor_edges=actor_closure(ROOT,['tests/libraryunionscalarcheck.py','tests/libraryabi/union_scalar.c','tests/checklib.py','tests/bound','tests/bound.py','exec/c/provenance.py','exec/pipeline/models.py'])
 assert {'tests/bound','tests/bound.c','tests/actorclosure.py'}<=set(actors)
 basis={'schema':'union-scalar-tools-v2','loader_policy':'remove-LD_-and-GLIBC_TUNABLES; verify actual loader init path and runtime SHA','sources':provenance.source_digest(),'actors':actors,'actor_edges':actor_edges,'target':'lnx/x86_64','sanitize':sanitize,'cc':str(cc),'cc_sha256':digest(cc),'cc_version':version,'environment':{n:os.environ.get(n) for n in ('PATH','CPATH','C_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SOURCE_DATE_EPOCH')}}
 key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest()
 base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
 return base,key,basis,cc

def tool_need(reason):
 print('NEED_PREPARE union scalar tools: '+str(reason),file=__import__('sys').stderr);raise SystemExit(75)

def tool_verify(path,expected):
 import provenance
 try:
  record=provenance.verify(path)
  if record.get('union_scalar_tools')!=expected:raise ValueError('role/flags/parent identity mismatch')
 except (SystemExit,ValueError) as error:tool_need(error)
 return record

def tool_attest(path,sources,expected,inputs):
 import provenance
 provenance.write(path,sources);p=Path(str(path)+'.build.json');r=json.loads(p.read_text());r['union_scalar_tools']=expected;r['source_closure']=inputs
 tmp=Path(str(p)+'.tmp');tmp.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n');tmp.replace(p)

def tools_for(cases,pressures,arch,sanitize,prepare=False,journal=None):
 import fcntl
 base,key,basis,cc=tool_context(arch,sanitize);san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else []
 libname='libunisacc.so';runtime=base/('tools-union-scalar-runtime-'+key);role={'identity':key,'role':'runtime'}
 def valid(path,expected):
  import provenance
  try:
   rec=provenance.verify(path)
   if rec.get('union_scalar_tools')!=expected:raise ValueError('role/flags/parent identity mismatch')
   return rec
  except (SystemExit,ValueError):
   if prepare:return None
   raise
 def locked(folder):
  base.mkdir(parents=True,exist_ok=True);lock=Path(str(folder)+'.lock').open('a');fcntl.flock(lock,fcntl.LOCK_EX);return lock
 def publish(work,dest):
  try:dest.stat()
  except FileNotFoundError:pass
  else:shutil.rmtree(dest)
  work.rename(dest)
 def command(stage,args):
  import time,sys
  row={'stage':stage,'argv':list(map(str,args)),'state':'RUNNING','start_monotonic':time.monotonic()}
  if journal:journal(row)
  q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'55',*map(str,args)],env=tool_environment(),capture_output=True,text=True,timeout=58)
  row.update(state='DONE',rc=q.returncode,seconds_monotonic=time.monotonic()-row['start_monotonic'],stdout=q.stdout,stderr=q.stderr)
  if journal:journal(row)
  if q.returncode:raise RuntimeError(row)
 if prepare:
  with locked(runtime):
   rec=valid(runtime/libname,role)
   if rec is None:
    with tempfile.TemporaryDirectory(prefix='tools-union-scalar-build-',dir=base) as td:
     w=Path(td);inputs=copy_runtime(ROOT,w);rt=w/'exec/c'
     command('runtime_build',[cc,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden','-Wl,-soname,'+libname,rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',w/libname])
     tool_attest(w/libname,basis['sources'],role,inputs)
     assert tool_context(arch,sanitize)[1]==key,'preparation inputs changed'
     publish(w,runtime)
 rec=tool_verify(runtime/libname,role);parent=sha(runtime/libname);inputs=rec['source_closure'];probes={}
 for name in cases:
  for pressure in pressures:
   case=CASES[name][0];prefix=PRESSURES[pressure];child={'identity':key,'role':'probe','parent_sha256':parent,'case':name,'pressure':pressure}
   childkey=hashlib.sha256(json.dumps(child,sort_keys=True).encode()).hexdigest();folder=base/('tools-union-scalar-probe-'+childkey)
   if prepare:
    with locked(folder):
     rec=valid(folder/'probe',child)
     if rec is None:
      with tempfile.TemporaryDirectory(prefix='tools-union-scalar-build-',dir=base) as td:
       w=Path(td);closure=copy_runtime(ROOT,w);rt=w/'exec/c';shutil.copy2(ROOT/'tests/libraryabi/union_scalar.c',w/'probe.c');closure['tests/libraryabi/union_scalar.c']=sha(w/'probe.c');shutil.copy2(runtime/libname,w/libname)
       command('probe_build',[cc,*san,'-std=c11','-O2','-Wall','-Wextra','-I',rt,'-DUNION_CASE='+str(case),'-DUNION_PRESSURE='+str(prefix),w/'probe.c',w/libname,'-Wl,-rpath,$ORIGIN','-o',w/'probe'])
       tool_attest(w/libname,basis['sources'],dict(child,role='probe-runtime'),closure);tool_attest(w/'probe',basis['sources'],child,closure)
       assert tool_context(arch,sanitize)[1]==key,'preparation inputs changed'
       publish(w,folder)
   tool_verify(folder/'probe',child);tool_verify(folder/libname,dict(child,role='probe-runtime'))
   if sha(folder/libname)!=parent:tool_need('prepared probe runtime differs from parent')
   probes[(name,pressure)]=folder/'probe'
 inputs=dict(inputs);inputs['tests/libraryabi/union_scalar.c']=sha(ROOT/'tests/libraryabi/union_scalar.c')
 return runtime/libname,probes,inputs,key

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',default=str(ROOT/'unisacc.com'));ap.add_argument('--prepare',action='store_true');ap.add_argument('--require-cached',action='store_true');ap.add_argument('--arch',choices=['arm64','x86_64']);ap.add_argument('--cases',default='int8,float4,double8,mixed2,mixed3,mixed-reordered');ap.add_argument('--pressures',default='plain,gp9,fp9');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--build-only',action='store_true');ap.add_argument('--evidence',type=Path);a=ap.parse_args()
 if (a.prepare or a.require_cached) and (platform.system()!='Linux' or platform.machine()!='x86_64' or (a.arch is not None and a.arch!='x86_64')):
  print('PLATFORM_UNVERIFIED union scalar prepared tools: native lnx/x86_64 only',file=__import__('sys').stderr);return 77
 cases=a.cases.split(',');pressures=a.pressures.split(',');assert cases and pressures and all(x in CASES for x in cases) and all(x in PRESSURES for x in pressures)
 if (a.prepare or a.require_cached) and a.evidence is None:
  directory=Path(tempfile.gettempdir())/'union-scalar-evidence';directory.mkdir(parents=True,exist_ok=True);a.evidence=directory/(str(os.getpid())+'.json')
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if platform.system() not in ('Darwin','Linux') or (arch!=host and platform.system()!='Darwin'):raise RuntimeError('requires matching native platform runner')
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch;flags=['-arch',arch] if platform.system()=='Darwin' else []
 if a.prepare and a.require_cached:ap.error('choose prepare or require-cached')
 if a.prepare:
  events=[]
  def journal(row):
   events.append(dict(row))
   if a.evidence:
    a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps({'status':'PREPARING','events':events},indent=2)+'\n')
  lib,probes,inputs,key=tools_for(cases,pressures,arch,a.sanitize,True,journal)
  result={'status':'PREPARED_UNION_SCALAR_TOOLS','identity':key,'runtime':str(lib),'probes':{n+'/'+p:str(exe) for (n,p),exe in probes.items()},'source_closure':inputs,'events':events,'scope':'native lnx/x86_64 pilot; preparation, not public calls','evidence_path':str(a.evidence)}
  if a.evidence:a.evidence.write_text(json.dumps(result,indent=2)+'\n')
  print(json.dumps(result));return 0
 package=Path(a.package).resolve(strict=True)
 record={'schema':1,'target':target,'scope':'build-only; no public calls' if a.build_only else 'actual public fixed scalar union matrix; any rejection is failure','package_path':str(package),'package_sha256':sha(package),'sanitize':a.sanitize,'cases':cases,'pressures':pressures,'commands':[],'variants':[],'status':'failed'}
 def finish(rc):
  record['actual_runner_rc']=rc
  if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(record,indent=2)+'\n')
  print(json.dumps({'target':target,'status':record['status'],'actual_runner_rc':rc,'evidence_path':str(a.evidence),'variants':[{'case':v['case'],'pressure':v['pressure'],'rc':v['rc'],'calls_100':[e['opt'] for e in v['events'] if e['stage']=='calls_100']} for v in record['variants']]}));return rc
 with tempfile.TemporaryDirectory(prefix='r10-union-scalar-') as tmp:
  t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir();inputs={}
  cached_probes={};cached_lib=None
  if a.require_cached:
   cached_lib,cached_probes,inputs,tool_key=tools_for(cases,pressures,arch,a.sanitize)
   record['prepared_tool_identity']=tool_key
  else:
   inputs.update(copy_runtime(ROOT,t))
   shutil.copy2(ROOT/'tests/libraryabi/union_scalar.c',t/'probe.c');inputs['tests/libraryabi/union_scalar.c']=sha(t/'probe.c')
  record['source_closure']=inputs
  frozen=t/'compiler.pkg';shutil.copy2(package,frozen);assert sha(frozen)==record['package_sha256']
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else []
  def flush():
   if a.evidence:
    a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(record,indent=2)+'\n')
  def run(stage,command):
   import time
   cmd=[str(ROOT/'tests/bound'),'20',*map(str,command)];item={'stage':stage,'command':cmd,'state':'RUNNING','start_monotonic':time.monotonic()};record['commands'].append(item);flush()
   child_env=tool_environment() if a.require_cached else dict(os.environ)
   if a.require_cached and stage=='public_probe':child_env['LD_DEBUG']='libs'
   r=subprocess.run(cmd,env=child_env,capture_output=True,text=True,timeout=25)
   if a.require_cached and stage=='public_probe' and r.returncode==0:
    loaded=[line.split('calling init:',1)[1].strip() for line in r.stderr.splitlines() if 'calling init:' in line and 'libunisacc.so' in line]
    expected=Path(command[0]).parent/'libunisacc.so'
    assert len(loaded)==1 and Path(loaded[0]).resolve(strict=True)==expected.resolve(strict=True),('runtime-load-path',loaded,str(expected))
    assert sha(Path(loaded[0]))==record['runtime_sha256'],('runtime-load-sha',loaded)
    item['loaded_runtime']={'path':str(Path(loaded[0]).resolve(strict=True)),'sha256':sha(Path(loaded[0])),'loader_environment':'LD_* and GLIBC_TUNABLES cleared; diagnostic LD_DEBUG=libs'}
   item.update(state='DONE',rc=r.returncode,seconds_monotonic=time.monotonic()-item['start_monotonic'],stdout=r.stdout,stderr=r.stderr);flush();return item
  try:
   if a.require_cached:lib=cached_lib
   else:
    lib=t/'library.so';r=run('runtime_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',lib])
    if r['rc']:return finish(1)
   record['runtime_sha256']=sha(lib)
   for name in cases:
    case,w,al,members=CASES[name];payload=U(len(members))+b''.join(U(0)*3+U(mw)+descriptor(k,mw,mw,uns) for k,mw,uns in members);union=descriptor(5,w,al,tag=2,payload=payload)
    for pressure in pressures:
     prefix=PRESSURES[pressure];params=([descriptor(1 if prefix==1 else 3,8,8,1 if prefix==1 else 0)]*9 if prefix else [])+[union]
     nm=b'host_step';sig=b'USLSIG2\n'+U(1)+U(len(nm))+nm+bytes([0,1,0,int(len(params)>6)])+U(len(params))+union+U(len(params))+b''.join(params)+bytes([0]);sf=t/'host_step.sig';sf.write_bytes(sig);exe=t/'probe'
     variant={'case':name,'pressure':pressure,'union_extent':w,'union_alignment':al,'signature_hex':sig.hex(),'signature_sha256':sha(sf),'native_member_layout':members,'events':[],'rc':1};record['variants'].append(variant)
     if a.require_cached:exe=cached_probes[(name,pressure)]
     else:
      r=run('probe_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-Wall','-Wextra','-I',rt,'-DUNION_CASE='+str(case),'-DUNION_PRESSURE='+str(prefix),t/'probe.c',lib,'-o',exe])
      if r['rc']:return finish(1)
     if a.build_only:variant['rc']=0;continue
     r=run('public_probe',[*(['arch','-x86_64'] if host!=arch else []),exe,frozen,target,sf]);variant['rc']=r['rc']
     for line in r['stdout'].splitlines():
      try:event=json.loads(line)
      except json.JSONDecodeError:continue
      if isinstance(event,dict) and 'stage' in event:variant['events'].append(event)
     complete=[e for e in variant['events'] if e['stage']=='calls_100' and e['rc']==0]
     if r['rc'] or [e['opt'] for e in complete]!=[0,1,2]:return finish(1)
     assert all(e['extent']==w and e['alignment']==al for e in variant['events'])
     assert [e['native_calls'] for e in complete]==[100,200,300]
   record['status']='built_only' if a.build_only else 'passed';return finish(0)
  except Exception as exc:record['exception']=repr(exc);return finish(1)
if __name__=='__main__':raise SystemExit(main())
