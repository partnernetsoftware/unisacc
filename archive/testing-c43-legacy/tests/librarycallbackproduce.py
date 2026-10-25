"""Observation orchestration only; existing bound/typed cache owner/SC1 runner.
No model semantics, watchdog, resource fallback or independent cache lifecycle.
"""
import argparse,hashlib,importlib.util,json,os,pathlib,subprocess,sys,time,shutil
P=pathlib.Path
p=argparse.ArgumentParser();p.add_argument('--root',type=P,required=True);p.add_argument('--product',type=P,required=True);p.add_argument('--cache',type=P);p.add_argument('--out',type=P,required=True);p.add_argument('--part',type=int,choices=(1,2,3,4),required=True);p.add_argument('--selftest',action='store_true');p.add_argument('--run',action='store_true');p.add_argument('--capture',nargs=8,metavar=('SOURCE','MODEL','TAPE','CONTRACT','SELECTION','GRAPHS','REAL_SIG','PRODUCER'));p.add_argument('--cases',type=P);p.add_argument('--checker',type=P);p.add_argument('--auth',nargs=4);p.add_argument('--off-artifacts',type=P);p.add_argument('--capture-cases',type=P);p.add_argument('--e3-capture',nargs=5);p.add_argument('--snapshot-runner',type=P);a=p.parse_args()
R=a.root.resolve();O=a.out.resolve();O.mkdir(parents=True,exist_ok=True);product=a.product.resolve(strict=True)
if O==R or R in O.parents:raise SystemExit('INTERFACE_ERROR output must be outside source tree')
spec=importlib.util.spec_from_file_location('_callback_reseal',R/'release/tools/model-reseal.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);SOURCE_HASH=m.source_hash(R)
checker=(R/a.checker if a.checker and not a.checker.is_absolute() else a.checker) if a.checker else R/'tests/librarycallbacklowercheck.cx'
checker=checker.resolve(strict=True);checker.relative_to(R)
identity={'checker':{'path':str(checker),'sha256':hashlib.sha256(checker.read_bytes()).hexdigest()}}
records=[]
def digest(q):return hashlib.sha256(P(q).read_bytes()).hexdigest()
def run(name,argv,limit=15):
 start=time.monotonic();d=O/name;d.mkdir(exist_ok=False)
 with (d/'stdout').open('wb') as out,(d/'stderr').open('wb') as err:rc=subprocess.run(['python3',str(R/'tests/bound.py'),str(limit),*map(str,argv)],cwd=R,stdout=out,stderr=err).returncode
 (d/'status').write_text(str(rc)+'\n');row={'step':name,'argv':list(map(str,argv)),'rc':rc,'monotonic':time.monotonic()-start,'product_sha':digest(product),'source_hash':SOURCE_HASH,'artifact_sha':digest(d/'output.bytes' if (d/'output.bytes').is_file() else O/'output.bytes' if (O/'output.bytes').is_file() else d/'stdout')};records.append(row);(O/'steps.json').write_text(json.dumps(records,indent=2)+'\n');cols=['step','argv','rc','monotonic','product_sha','source_hash','artifact_sha'];(O/'steps.tsv').write_text('\t'.join(cols)+'\n'+''.join('\t'.join([x['step'],json.dumps(x['argv'],separators=(',',':')),str(x['rc']),str(x['monotonic']),x['product_sha'],x['source_hash'],x['artifact_sha']])+'\n' for x in records));return rc,d

def snapshot_consume(name,runner,snapshot,upstream_stage,stage,model,input_path,capture_out=None):
 # Shared independent-process consumption for both E3 and lower semantic owners.
 captured=capture_out if capture_out is not None else O/name/'captured'
 command=[runner,'--observe-callables','--package',product,'--ri-snapshot',snapshot,
          '--upstream-stage',upstream_stage,'--stage',stage,'--capture-out',captured,model,input_path]
 rc,d=run(name,command,55)
 if rc not in (0,1):return rc,d,captured
 for original,target in [('stdout','output.bytes'),('stderr','error-stream.bytes'),('status','status')]:
  shutil.copyfile(d/original,captured/target)
 return rc,d,captured

def judge(name,args):
 rc,d=run(name,['sh',str(product),'-run',str(checker),'--',*args]);
 if rc:raise SystemExit(rc)
 return d
if a.capture_cases:
 # Capture actual adapter responses for explicit hash-bound observation inputs.
 # No ABSENT/EMPTY inference and no mutation of model or program semantics.
 request=json.loads(a.capture_cases.read_text());cases=[]
 if request.get('format')!='CALLBACK_CAPTURE_REQUEST1' or not request.get('cases'):raise SystemExit('INTERFACE_ERROR capture request')
 def checked(item):
  q=P(item['path']).resolve(strict=True)
  if item.get('state')!='PRESENT' or digest(q)!=item['sha256']:raise ValueError('capture artifact mismatch')
  return q
 def present(q):return {'state':'PRESENT','path':str(q),'sha256':digest(q)}
 identity['capture_request']={'path':str(a.capture_cases.resolve()),'sha256':digest(a.capture_cases)}
 for i,item in enumerate(request['cases']):
  args=[checked(item[k]) for k in ['source','model','input','contract','selection','graphs','signature','producer']]
  source,model,tape,contract,selection,graphs,sig,producer=args
  # API producer only materializes inputs. The standalone process owns model
  # execution and a fresh capture. No process-global capture state is reset.
  runner=checked(item['runner'])
  name='provider-%03d'%i
  rc,provider_dir=run(name,[producer,'--snapshot',product,source,O/name,model,tape,contract,selection,graphs,sig],55)
  if rc:raise SystemExit('snapshot export failed: '+name+' rc='+str(rc))
  name='capture-%03d'%i
  rc,d,captured=snapshot_consume(name,runner,provider_dir/'provider-snapshot',item['stage'],item['stage'],model,provider_dir/'consumer-input.bytes')
  expected=1 if item['kind']=='refusal' else 0
  if rc!=expected:raise SystemExit('capture outcome differs: '+name+' rc='+str(rc))
  row={'id':item['id'],'part':item['part'],'kind':item['kind'],'model':str(model),'model_sha256':digest(model),'input':present(provider_dir/'consumer-input.bytes'),'ledger':present(captured/'resources.tsv'),'stage':item['stage']}
  if expected:
   row.update(reference_error=present(checked(item['reference_error'])),reason=item['reason'])
  else:row['reference']=present(checked(item['reference']))
  cases.append(row)
 (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 (O/'cases.json').write_text(json.dumps({'format':'CALLBACK_LOWER_CASES1','source_hash':SOURCE_HASH,'cases':cases},indent=2)+'\n')
 print('captured',len(cases),'cases; replay verdict is separate')
elif a.off_artifacts:
 # Harvest explicit constructed pairs; never construct or synthesize baselines.
 base=a.off_artifacts.resolve(strict=True);rows=[]
 variants=['lnx-x','lnx-a','osx-x','osx-a','win-x','win-a','obj-lnx-x','obj-lnx-a','obj-osx-x','obj-osx-a','obj-win-x','obj-win-a']
 def present(q):
  q=q.resolve(strict=True)
  return {'state':'PRESENT','path':str(q),'sha256':digest(q)}
 for i,v in enumerate(variants):
  for suffix in ['c.json','p.json','tbl','net']:
   actual=base/v/('off-'+suffix if suffix.endswith('json') else 'off.'+suffix)
   reference=base/v/('base-'+suffix if suffix.endswith('json') else 'base.'+suffix)
   rows.append({'id':v+'-'+suffix,'part':i//3+1,'kind':'artifact-bytes','actual':present(actual),'reference':present(reference)})
 (O/'cases.json').write_text(json.dumps({'format':'CALLBACK_LOWER_CASES1','source_hash':SOURCE_HASH,'scope':'off construction artifact pairs only; on witnesses supplied separately','cases':rows},indent=2)+'\n')
 print('wrote',len(rows),'explicit artifact pairs; NOT A VERDICT')
elif a.selftest:
 # No constructors/model run. Every entry exercises its part selection and the
 # exact same dynamic-C verdict machinery; negative mutations are synthetic.
 test=O/'cx-fixtures';test.mkdir();judge('checker-selftest',['--selftest',str(test)])
 (O/'entry.json').write_text(json.dumps({'part':a.part,'rc':0,'scope':'entry/verdict selftest only; no callback model or target evidence','other_callback_on_targets':'UNVERIFIED (11 targets)'},indent=2)+'\n');print('entry part',a.part,'selftest rc0; NO MODEL PASS')
elif a.capture:
 source,model,tape,contract,selected,graphs,sig,producer=a.capture
 rc,d=run('capture',[producer,'--capture',str(product),source,str(O),model,tape,contract,selected,graphs,sig]);raise SystemExit(rc)
elif a.e3_capture:
 source,model,contract,selection,producer=map(P,a.e3_capture)
 if a.snapshot_runner is None:p.error('--e3-capture requires explicit native --snapshot-runner')
 runner=a.snapshot_runner.resolve(strict=True)
 for label,q in [('source',source),('model',model),('contract',contract),('selection',selection),('producer',producer),('runner',runner)]:
  q=q.resolve(strict=True);identity[label]={'path':str(q),'sha256':digest(q)}
 provider=O/'e3-provider'
 rc,d=run('e3-provider',[producer,product,source,provider,model,contract,selection],55)
 if rc:raise SystemExit(rc)
 shutil.copyfile(provider/'input.bytes',O/'input.bytes')
 label='e3:lnx/x86_64/tape/O0'
 # v7 captures only a new ledger; O has observation metadata, never a ledger.
 rc,d,captured=snapshot_consume('e3-capture',runner,provider/'provider-snapshot',label,label,model,provider/'input.bytes',O)
 for key,path in [('ledger',O/'resources.tsv'),('input',O/'input.bytes')]:
  if path.is_file():identity[key]={'path':str(path),'sha256':digest(path)}
 (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 raise SystemExit(rc)
elif a.auth:
 probe,capture,sa,sb=map(P,a.auth)
 for label,q in [('probe',probe),('signature_capture',capture),('source_A',sa),('source_B',sb)]:
  q=q.resolve(strict=True);identity[label]={'path':str(q),'sha256':digest(q)}
 (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 for label,source in [('A',sa),('B',sb)]:
  rc,d=run('sig-'+label,[capture,product,'lnx/x86_64',source,O/(label+'.sig')],55)
  if rc:raise SystemExit(rc)
  identity['signature_'+label]={'path':str(O/(label+'.sig')),'sha256':digest(O/(label+'.sig'))}
  # Signature digest is explicit in identity; step artifact remains captured stdout.
 (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 rc,positive=run('auth-AA',[probe,product,O/'A.sig',O/'A.sig','positive'],55)
 if rc:raise SystemExit(rc)
 rc,negative=run('auth-AB',[probe,product,O/'A.sig',O/'B.sig','negative'],55)
 if rc not in (0,1):raise SystemExit(rc)
 judge('auth-verdict',[str(O)])
 print('auth component witness only; no runtime/ABI PASS')
elif a.run:
 if not a.cache:raise SystemExit('INTERFACE_ERROR --cache required')
 C=a.cache.resolve();fixture=(R/a.cases if a.cases and not a.cases.is_absolute() else a.cases) if a.cases else R/'tests/seals/librarycallbacklower/cases.json'
 try:cases=json.loads(fixture.read_text())
 except FileNotFoundError:print('NEED_PREPARE callback lower cases/producer, not an empty PASS',file=sys.stderr);raise SystemExit(75)
 identity['cases']={'path':str(fixture.resolve(strict=True)),'sha256':digest(fixture)}; (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 if cases.get('format')!='CALLBACK_LOWER_CASES1' or not cases.get('cases'):raise SystemExit('INTERFACE_ERROR malformed/empty callback cases')
 # Fixture hashes are declared, never inferred from a missing key. Artifact
 # source/model identity is verified by existing sha256sum manifests beforehand.
 def path(item):
  if not isinstance(item,dict) or item.get('state')!='PRESENT':raise ValueError('explicit PRESENT artifact required')
  q=(R/item['path']).resolve() if not P(item['path']).is_absolute() else P(item['path']).resolve()
  if digest(q)!=item['sha256']:raise ValueError('artifact hash mismatch: '+str(q))
  return q
 selected=[x for x in cases['cases'] if x['part']==a.part]
 if not selected:raise SystemExit('INTERFACE_ERROR empty callback shard')
 identity['part']=a.part
 identity['selected_ids']=[x['id'] for x in selected]
 if len(set(identity['selected_ids']))!=len(identity['selected_ids']):raise SystemExit('INTERFACE_ERROR duplicate case ID')
 (O/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
 plan=[]
 for i,case in enumerate(selected):
  name=case['id'];kind=case['kind']
  # Construction equivalence is a byte-artifact verdict, not a model replay.
  # The producer supplies both artifacts explicitly; no baseline is inferred.
  if kind=='artifact-bytes':
   plan.append(['bytes',name,str(path(case['actual'])),str(path(case['reference'])),'-','-','-','-'])
   continue
  model=C/case['model'];input_path=path(case['input']);ledger=path(case['ledger'])
  if case.get('model_sha256') and digest(model)!=case['model_sha256']:raise SystemExit('model hash mismatch')
  verify=subprocess.run(['sha256sum','-c','--status','artifacts.sha256'],cwd=model.parent)
  if verify.returncode:raise SystemExit('NEED_PREPARE model artifact digest')
  # ledger item points at resources.tsv; shared parser checks blobs+SC1. The
  # stage comes from the frozen case, never guessed from input magic.
  rc,d=run('case-%03d'%i,[str(C/'tools/runner'),'--replay',str(model),str(input_path),str(ledger.parent),case['stage'],str(O/('case-%03d'%i))])
  if kind=='refusal':
   if rc not in (0,1):raise SystemExit(rc)
   plan.append(['refusal',name,str(d/'status'),str(d/'output.bytes'),str(d/'stderr'),str(path(case['reference_error'])),case['reason'],'-'])
   named_rc,named_dir=run('named-%03d'%i,['sh',str(product),'-run',str(R/'tests/librarycallback-refusalcheck.cx'),'--',str(d/'stderr'),str(path(case['reference_error'])),case['reason']])
   if named_rc:raise SystemExit(named_rc)
  elif kind in ('bytes','projection'):
   if rc:raise SystemExit(rc)
   plan.append([kind,name,str(d/'output.bytes'),str(path(case['reference'])),'-','-','-','-'])
  else:raise SystemExit('INTERFACE_ERROR unknown case kind')
 plan_path=O/'verdict-plan.tsv';plan_path.write_text(''.join('\t'.join(row)+'\n' for row in plan));judge('verdict',['--plan',str(plan_path)]);print('callback part',a.part,'checked',len(plan),'rows; other 11 on targets UNVERIFIED')
else:p.error('choose --selftest, --run, --capture or --auth')
