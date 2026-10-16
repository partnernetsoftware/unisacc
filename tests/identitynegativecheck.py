#!/usr/bin/env python3
"""Three bounded identity refusal witnesses; no construction or product execution."""
import argparse,hashlib,importlib.util,json,os,pathlib,shutil,subprocess,sys,tempfile,time
sys.dont_write_bytecode=True
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def present(p):
 try:p.stat()
 except FileNotFoundError:raise SystemExit('NEED_PREPARE identity smoke input: '+str(p))
 return p
p=argparse.ArgumentParser(description=__doc__);p.add_argument('part',choices=('product','include','prep'));p.add_argument('--candidate',type=pathlib.Path);p.add_argument('--ua',type=pathlib.Path);a=p.parse_args()
start=time.monotonic();steps=[];pins={};kind=''
def protect(p):
 p=present(p.resolve());pins[str(p)]=sha(p);return p
def call(name,args,rc=0,text=b''):
 t=time.monotonic();q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'4',*map(str,args)],cwd=ROOT,capture_output=True,timeout=6)
 elapsed=time.monotonic()-t
 steps.append({'step':name,'argv':list(map(str,args)),'rc':q.returncode,'monotonic':elapsed,'stdout_sha256':hashlib.sha256(q.stdout).hexdigest(),'stderr_sha256':hashlib.sha256(q.stderr).hexdigest()})
 if q.returncode!=rc or elapsed>5 or text and text not in q.stdout+q.stderr:raise AssertionError((name,q.returncode,q.stdout,q.stderr))
 return q
with tempfile.TemporaryDirectory(prefix='identity-smoke-') as td:
 t=pathlib.Path(td)
 if a.part=='product':
  candidate=protect(a.candidate or ROOT/'unisacc.com');side=protect(pathlib.Path(str(candidate)+'.build.json'))
  call('candidate-fresh',[sys.executable,'-B',ROOT/'exec/c/provenance.py','check',candidate],text=b'product freshness: verified')
  root_product=protect(ROOT/'unisacc.com');protect(ROOT/'unisacc.com.build.json')
  call('actual-root-pair',['cmp','--',root_product,candidate])
  call('actual-root-sidecar',['cmp','--',ROOT/'unisacc.com.build.json',side])
  good=t/'positive/unisacc.com';bad=t/'negative/unisacc.com';good.parent.mkdir();bad.parent.mkdir()
  shutil.copy2(candidate,good);shutil.copy2(side,pathlib.Path(str(good)+'.build.json'))
  raw=candidate.read_bytes();assert raw;different=bytearray(raw);different[0]^=1;bad.write_bytes(different);shutil.copy2(side,pathlib.Path(str(bad)+'.build.json'))
  kind='ONE_BYTE_DIFFERENT_PRODUCT_CONTROL_NOT_HISTORICAL_RELEASE'
  call('positive-pair',['cmp','--',good,candidate])
  call('positive-fresh',[sys.executable,'-B',ROOT/'exec/c/provenance.py','check',good],text=b'product freshness: verified')
  call('different-pair',['cmp','--',bad,candidate],rc=1)
  call('artifact-refusal',[sys.executable,'-B',ROOT/'exec/c/provenance.py','check',bad],rc=1,text=b'artifact differs from its build record')
 elif a.part=='include':
  helper=protect(ROOT/'tests/checklib.py');header=protect(ROOT/'exec/c/resourceledger.h');dep=protect(ROOT/'include/unisa_sha256.h')
  sp=importlib.util.spec_from_file_location('identity_copy_runtime',helper);m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
  source=t/'source'
  for f in (header,dep):
   dest=source/f.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,dest)
  expected={f.relative_to(ROOT).as_posix():pins[str(f)] for f in (header,dep)}
  before=time.monotonic();copied=m.copy_runtime(source,t/'positive');assert copied==expected and all(sha(t/'positive'/n)==h for n,h in expected.items());steps.append({'step':'positive-closure','rc':0,'monotonic':time.monotonic()-before,'copied':copied})
  missing=source/'include/unisa_sha256.h';missing.unlink();before=time.monotonic()
  try:m.copy_runtime(source,t/'negative')
  except FileNotFoundError as e:assert str(e)=='runtime include missing: '+str(missing);message=str(e)
  else:raise AssertionError('missing runtime include accepted')
  steps.append({'step':'missing-header','exception':'FileNotFoundError','reason':message,'monotonic':time.monotonic()-before,'successful_closure_returned':False})
  kind='REAL_QUOTED_INCLUDE_REMOVED_ONLY_IN_TEMP_ROOT'
 else:
  ua=protect(a.ua or pathlib.Path(os.environ.get('UA','/tmp/ua_ref')));target={'Linux':'lnx','Darwin':'osx'}.get(__import__('platform').system());arch={'x86_64':'x86_64','arm64':'arm64','aarch64':'arm64'}.get(__import__('platform').machine());assert target and arch,'identity prep host not covered'
  cache=t/'cache';cache.mkdir();output=t/'output';args=[sys.executable,'-B',str(ROOT/'exec/pipeline/models.py'),str(output),target+'/'+arch,'1','cc','--native-ua',str(ua),'--require-cached']
  before=time.monotonic();q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'4',*args],cwd=ROOT,env=dict(os.environ,UNISACC_MODEL_CACHE=str(cache)),capture_output=True,timeout=6)
  steps.append({'step':'missing-prep','argv':args,'rc':q.returncode,'monotonic':time.monotonic()-before,'stdout_sha256':hashlib.sha256(q.stdout).hexdigest(),'stderr_sha256':hashlib.sha256(q.stderr).hexdigest(),'reason':q.stderr.decode(errors='replace')})
  assert q.returncode==75 and b'NEED_PREPARE native resources: run exec-native-resources-prep' in q.stderr,(q.returncode,q.stdout,q.stderr)
  files=[f for f in cache.rglob('*')];assert all(f.is_file() and f.name.endswith('.lock') for f in files),files
  try:output.stat()
  except FileNotFoundError:pass
  else:raise AssertionError('missing prep published output')
  kind='EMPTY_TEMP_CACHE_EXISTING_REQUIRE_CACHED_REFUSAL_ONLY'
assert all(sha(path)==h for path,h in pins.items()),'original input changed'
elapsed=time.monotonic()-start;assert elapsed<=10,elapsed
print(json.dumps({'part':a.part,'status':'PASS_EXPECTED_REFUSAL','kind':kind,'monotonic':elapsed,'sources':pins,'steps':steps,'temporary_only':True,'no_product_execution_or_cold_build':True,'warm_cache_positive':'REFERENCE_STOCK8640_IDENTITY_VALID_REFUSALS_ONLY_INDEX_e6a73b41cabbc86235e9d7fdeb46f4e4f271d419b2bd164b4e3f3d6255538bbf' if a.part=='prep' else 'NOT_APPLICABLE','actual_consumer_coverage':'NOT_RUN_HELPER_ONLY_BYPASS_CONSUMERS_KEEP_ORIGINAL_ASSERTIONS' if a.part=='include' else 'NOT_APPLICABLE','meal':'NOT_ASSESSED'},sort_keys=True))
