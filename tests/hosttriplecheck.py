#!/usr/bin/env python3
"""Prepared U0 and three nativeboot generations, only native lnx/x86_64."""
import argparse,hashlib,json,os,pathlib,platform,shutil,subprocess,sys,tempfile,time
sys.dont_write_bytecode=True
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance,sourceflat
from models import digest
p=argparse.ArgumentParser(description=__doc__);p.add_argument('phase',choices=('prepare','u0','n1','n2','n3'));a=p.parse_args();start=time.monotonic()
if platform.system()!='Linux' or platform.machine()!='x86_64':
 print('PLATFORM_UNVERIFIED host-triple requires native lnx/x86_64',file=sys.stderr);raise SystemExit(77)
import fcntl
base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
cc=pathlib.Path(shutil.which('cc')).resolve(strict=True)
version=subprocess.run([str(cc),'--version'],capture_output=True,check=True,timeout=2).stdout
from actorclosure import actor_closure
def host_actor_closure():
 return actor_closure(ROOT,['tests/hosttriplecheck.py','tests/nativeboot.sh','tests/build_ref.sh','tests/sourceflat.py','tests/bound.py'])
sources=provenance.source_digest();actors,actor_edges=host_actor_closure()
assert {'tests/lib.sh','tests/bound','tests/bound.c'}<=set(actors)

basis={'sources':sources,'actors':actors,'host':'lnx/x86_64','cc':str(cc),'cc_sha256':digest(cc),'cc_version':version.hex(),'CFLAGS':'-O2','compiler_environment':{k:os.environ.get(k) for k in ('PATH','CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SDKROOT','MACOSX_DEPLOYMENT_TARGET','SOURCE_DATE_EPOCH','CC','BOUND_CACHE','GITHUB_ACTIONS','UNISACC_BOUND_STOP_SECS','UNISACC_BOUND_TEST_NO_KILL')}}
key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest();u0=base/('host-triple-u0-'+key)
def need(name,reason):
 print('NEED_PREPARE host-triple-'+name+': '+reason,file=sys.stderr);raise SystemExit(75)
def role(stage,self_sha,parent_sha):return {'stage':stage,'target':'lnx/x86_64','identity':key,'self_sha256':self_sha,'parent_sha256':parent_sha}
def check(folder,name,expected):
 try:
  record=provenance.verify(folder/name)
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
 try:fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB)
 except BlockingIOError:f.close();need(a.phase,'producer is in progress')
 return f

command_times=[]
def run(args,bound,stdout=None):
 t=time.monotonic()
 with (stdout.open('wb') if stdout is not None else tempfile.TemporaryFile()) as output:
  q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),str(bound),*map(str,args)],cwd=ROOT,stdout=output,stderr=subprocess.PIPE,timeout=bound+2,env=dict(os.environ,CFLAGS='-O2',PYTHONDONTWRITEBYTECODE='1'))
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
    work=pathlib.Path(td);sourceflat.export_source(work/'self.c')
    run(['sh',ROOT/'tests/build_ref.sh',work/'U0.ref.c',work/'U0'],55)
    attest(work/'U0',role('u0',digest(work/'self.c'),digest(cc)));publish(work,u0)
  selfsha=verify_u0()
 print(json.dumps({'status':'PREPARED_U0','scope':'NATIVE_LNX_X86_64_ONLY','sources':sources,'identity':key,'U0':str(u0/'U0'),'self_sha256':selfsha,'seconds_monotonic':time.monotonic()-start,'preparation_not_test':True,'actors':actors,'actor_edges':actor_edges},sort_keys=True));raise SystemExit(0)
selfsha=verify_u0();chain=[(u0,'U0')];parent_sha=digest(u0/'U0');dirs={}
for n in range(1,int(a.phase[-1])+1 if a.phase!='u0' else 1):
 name='N'+str(n);stage=name.lower();dest=base/('host-triple-'+stage+'-'+hashlib.sha256((key+parent_sha).encode()).hexdigest());expected=role(stage,selfsha,parent_sha)
 if stage!=a.phase:
  check(dest,name,expected)
 else:
  with lock(dest):
   with tempfile.TemporaryDirectory(prefix='host-triple-generation-',dir=base) as td:
    work=pathlib.Path(td);previous=chain[-1][0]/chain[-1][1];before=digest(previous)
    run(['bash',ROOT/'tests/nativeboot.sh','--host-triple-build',u0/'self.c',previous,work/name],9)
    if digest(previous)!=before:raise SystemExit('host-triple parent changed during compilation')
    if stage=='n3':
     run(['cmp','--',chain[1][0]/'N1',chain[2][0]/'N2'],1)
     run(['cmp','--',chain[2][0]/'N2',work/'N3'],1)
    attest(work/name,expected);publish(work,dest)
  check(dest,name,expected)
 chain.append((dest,name));parent_sha=digest(dest/name);dirs[name]=str(dest/name)
print(json.dumps({'status':'PASS','scope':'NATIVE_LNX_X86_64_ONLY_NOT_SIX_TARGETS_OR_PRODUCT_PACK','phase':a.phase,'identity':key,'images':{name:digest(folder/name) for folder,name in chain},'seconds_monotonic':time.monotonic()-start,'three_images_compared':a.phase=='n3','command_times':command_times,'missing_predecessor_never_constructs':True,'actors':actors,'actor_edges':actor_edges},sort_keys=True))
