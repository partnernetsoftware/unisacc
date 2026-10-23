#!/usr/bin/env python3
"""Content-bound closure construction; target consumers delegate original assertions."""
import argparse,fcntl,hashlib,json,os,pathlib,platform,re,shutil,subprocess,sys,tempfile,time
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance,knownfail
from actorclosure import actor_closure
from models import digest
TARGETS=tuple(re.search(r'^TARGETS="([^"]+)"$',(ROOT/'tests/closure.sh').read_text(),re.M).group(1).split())
def need(reason):
 print('NEED_PREPARE closure target artifacts: '+str(reason),file=sys.stderr);raise SystemExit(75)
p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');p.add_argument('--target',choices=TARGETS);p.add_argument('--require-cached',action='store_true');p.add_argument('--source-part');p.add_argument('--source-cuts');p.add_argument('files',nargs='+');a=p.parse_args()
if a.prepare==a.require_cached or (a.require_cached and not a.target) or (a.prepare and a.target):p.error('choose --prepare or --target TARGET --require-cached')
files=a.files
if len(files)!=len(set(files)) or len({pathlib.Path(n).stem for n in files})!=len(files):p.error('duplicate source or basename')
# Selection changes the assertion workload only; preparation identity remains full-source.
consume_files=files
if a.prepare and (a.source_part is not None or a.source_cuts is not None):p.error('source selection is consumption only')
if a.source_cuts is not None and a.source_part is None:p.error('source-cuts requires source-part')
if a.source_part is not None:
 match=re.fullmatch(r'([1-9][0-9]*)/([1-9][0-9]*)',a.source_part)
 if not match:p.error('source-part must be K/N with positive integers')
 try:
  part,parts=map(int,match.groups())
  cuts=[int(v) for v in a.source_cuts.split(',')] if a.source_cuts is not None else [len(files)*i//parts for i in range(1,parts)] if parts<=len(files) else []
 except ValueError:p.error('source selection contains an invalid integer')
 if not 1<=part<=parts<=len(files):p.error('source-part is out of range or would select an empty group')
 if len(cuts)!=parts-1 or any(not 0<x<len(files) for x in cuts) or any(x>=y for x,y in zip(cuts,cuts[1:])):p.error('source-cuts must be N-1 strictly increasing interior boundaries')
 boundaries=[0,*cuts,len(files)];consume_files=files[boundaries[part-1]:boundaries[part]]
 if not consume_files:p.error('source selection is empty')
ua=pathlib.Path(os.environ.get('UA','/tmp/ua_ref')).resolve()
try:ua.stat()
except FileNotFoundError:need('reference compiler absent; supply same-source UA')
if not os.access(ua,os.X_OK):need('reference compiler is not executable')
actors,edges=actor_closure(ROOT,['tests/closure.sh','tests/closureprepared.py','tests/knownfail.sh','tests/knownfail.py','tests/par.sh','tests/lib.sh','tests/bound','tests/bound.py','exec/c/provenance.py'])
assert {'tests/bound.c','tests/actorclosure.py'}<=set(actors)
# Conservative tests/c closure retains all local fixture headers and quoted inputs.
fixture_tree={str(q.relative_to(ROOT)):digest(q) for q in sorted((ROOT/'tests/c').rglob('*')) if q.is_file() and '__pycache__' not in q.parts}
known=knownfail.read(ROOT/'tests/difftest.knownfail')
basis={'schema':'closure-target-inputs-v1','sources':provenance.source_digest(),'actors':actors,'actor_edges':edges,'files':[(n,digest(ROOT/n)) for n in files],'fixture_tree':fixture_tree,'known_sha256':digest(ROOT/'tests/difftest.knownfail'),'targets':TARGETS,'ua':str(ua),'ua_sha256':digest(ua),'python':sys.executable,'python_sha256':digest(pathlib.Path(sys.executable).resolve()),'host':[platform.system(),platform.machine()],'environment':{k:os.environ.get(k) for k in ('PATH','PYTHONPATH','PYTHONHASHSEED','PAR','LANG','LC_ALL','LD_LIBRARY_PATH','LD_PRELOAD','LD_AUDIT','UNISA_ABLATE','BOUND_CACHE','UNISACC_BOUND_STOP_SECS','UNISACC_BOUND_TEST_NO_KILL')}}
key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest();base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'));cache=base/('closure-target-'+key)
def target_role(target):return {'identity':key,'target':target,'ordered_files':files,'ua_sha256':basis['ua_sha256']}
def required(target):
 tt=target.replace('/','_');return {str(pathlib.Path(pathlib.Path(n).stem+'.d')/f) for n in files if pathlib.Path(n).stem not in known for f in ('tape','initial.rc','tape.'+tt,'py.'+tt,'ua.'+tt)}
def verify(target):
 folder=cache/target.replace('/','-');m=folder/'artifacts.json'
 try:cache.stat()
 except FileNotFoundError:
  if a.prepare:return None
  need('该 key 无准备（源或 actor 闭包变了）: '+key)
 try:
  record=provenance.verify(m);data=json.loads(m.read_text())
  if record.get('closure_target')!=target_role(target) or data.get('role')!=target_role(target) or set(data.get('outputs',{}))!=required(target):raise ValueError('target role or required output set changed')
  for n,h in data['outputs'].items():
   q=folder/n
   if digest(q)!=h or q.stat().st_size==0:raise ValueError('target artifact corrupt: '+n)
  return folder
 except (SystemExit,FileNotFoundError,ValueError,KeyError,TypeError) as error:
  if a.prepare:return None
  need(error)
if a.prepare:
 base.mkdir(parents=True,exist_ok=True)
 with pathlib.Path(str(cache)+'.lock').open('a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  if all(verify(target) is not None for target in TARGETS):print(json.dumps({'status':'HIT','identity':key,'cache':str(cache)}));raise SystemExit(0)
  with tempfile.TemporaryDirectory(prefix='closure-source-prepare-',dir=base) as td:
   work=pathlib.Path(td);capture=work/'capture';capture.mkdir();env=dict(os.environ,UA=str(ua),_CLOSURE_CAPTURE='1',_CLOSURE_ARTIFACT_DIR=str(capture));env.pop('_CLOSURE_REPLAY',None);env.pop('_CLOSURE_TARGET',None)
   log=work/'producer.log';start=time.monotonic()
   with log.open('wb') as f:q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'55','bash',str(ROOT/'tests/closure.sh'),*files],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT)
   print('closure source construction rc='+str(q.returncode)+' seconds='+str(time.monotonic()-start),flush=True)
   if q.returncode:sys.stdout.write(log.read_text());raise SystemExit(q.returncode)
   for target in TARGETS:
    folder=work/target.replace('/','-');folder.mkdir();outputs={}
    for n in sorted(required(target)):
     dest=folder/n;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(capture/n,dest)
     if not dest.stat().st_size:raise SystemExit('empty prepared target artifact: '+n)
     outputs[n]=digest(dest)
    m=folder/'artifacts.json';m.write_text(json.dumps({'role':target_role(target),'outputs':outputs},sort_keys=True)+'\n');provenance.write(m,basis['sources']);side=pathlib.Path(str(m)+'.build.json');rec=json.loads(side.read_text());rec['closure_target']=target_role(target);side.write_text(json.dumps(rec,sort_keys=True)+'\n')
   # Never publish an in-flight source change as this key.
   assert provenance.source_digest()==basis['sources']
   actors_now,_=actor_closure(ROOT,['tests/closure.sh','tests/closureprepared.py','tests/knownfail.sh','tests/knownfail.py','tests/par.sh','tests/lib.sh','tests/bound','tests/bound.py','exec/c/provenance.py']);assert actors_now==actors
   subprocess.run(['sh',str(ROOT/'tests/cxbin'),str(ROOT/'tests/safedel.cx'),'--purge','--max-files','100000','--',str(capture)],check=True)   # the one deletion guard (commissar ruling 2026-10-23)
   try:cache.stat()
   except FileNotFoundError:pass
   else:subprocess.run(['sh',str(ROOT/'tests/cxbin'),str(ROOT/'tests/safedel.cx'),'--max-files','100000','--',str(cache)],check=True)   # the replaced cache entry: into the trash
   work.rename(cache)
 print(json.dumps({'status':'PREPARED','identity':key,'cache':str(cache),'targets':TARGETS,'ordered_files':files,'actors':actors},sort_keys=True));raise SystemExit(0)
folder=verify(a.target)
with tempfile.TemporaryDirectory(prefix='closure-target-consume-') as td:
 scratch=pathlib.Path(td);shutil.copytree(folder,scratch/'artifacts');env=dict(os.environ,UA=str(ua),_CLOSURE_ARTIFACT_DIR=str(scratch/'artifacts'),_CLOSURE_REPLAY='1',_CLOSURE_TARGET=a.target);env.pop('_CLOSURE_CAPTURE',None)
 q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'10','bash',str(ROOT/'tests/closure.sh'),*consume_files],cwd=ROOT,env=env)
 raise SystemExit(q.returncode)
