#!/usr/bin/env python3
"""Freeze observed E3 capture+independent control references; existing shared producer owns execution/SC1.
No resource parser, signature serializer or model evaluation here. Incomplete/failed captures never emit cases.
"""
import argparse,hashlib,json,pathlib,shutil,subprocess
P=pathlib.Path
# a partial cases.json goes through the one deletion guard's argv CLI (include/unisa_safedel.h); --out is argv
SAFEDEL=['sh',str(P(__file__).resolve().parents[1]/'tests/cxbin'),str(P(__file__).resolve().parents[1]/'tests/safedel.cx'),'--purge','--']
p=argparse.ArgumentParser();p.add_argument('--spec',type=P,required=True);p.add_argument('--out',type=P,required=True);p.add_argument('--seal-prefix',default='tests/seals/librarycallbacke3');p.add_argument('--family',choices=('e3','lower','control'),default='e3');a=p.parse_args();spec=json.loads(a.spec.read_text());O=a.out.resolve();O.mkdir(parents=True,exist_ok=False)
if a.family=='e3' and (spec.get('format')!='CALLBACK_E3_FREEZE_SPEC1' or not spec.get('cases')):raise SystemExit('INTERFACE_ERROR empty/malformed freeze specification')
def sha(q):return hashlib.sha256(q.read_bytes()).hexdigest()
def checked(item):
 q=P(item['path']).resolve(strict=True)
 if item.get('state')!='PRESENT' or sha(q)!=item['sha256']:raise ValueError('artifact identity mismatch '+str(q))
 return q
def desc(q):return {'state':'PRESENT','path':a.seal_prefix+'/'+str(q.relative_to(O)),'sha256':sha(q)}
def copy_capture(src,name):
 assert name.replace('-','').replace('_','').isalnum(),'unsafe case ID'
 target=O/name;shutil.copytree(src,target);return target

if a.family=='control':
 assert spec.get('format')=='CALLBACK_CONTROL_SOURCE1'
 root=P(__file__).resolve().parents[1]
 target=O/'source';shutil.copytree(root,target,ignore=shutil.ignore_patterns('.git','__pycache__'))
 for item in spec['overlay']:
  rel=P(item['path']);assert not rel.is_absolute() and '..' not in rel.parts
  raw=(a.spec.resolve().parent/item['blob']).read_bytes();assert hashlib.sha256(raw).hexdigest()==item['sha256']
  (target/rel).write_bytes(raw)
 print('private control source restored; no model generated or validated')
 raise SystemExit(0)

if a.family=='lower':
 # Same capture-copy/relative descriptor mechanism; semantic ownership stays lower.
 if spec.get('format')!='CALLBACK_LOWER_FREEZE_SPEC1':raise SystemExit('INTERFACE_ERROR lower freeze specification')
 manifest=checked(spec['cases_manifest']);cases=json.loads(manifest.read_text())
 replay=P(spec['replay']).resolve(strict=True);identity=json.loads((replay/'identity.json').read_text());steps=json.loads((replay/'steps.json').read_text())
 assert identity['cases']['sha256']==sha(manifest) and steps and steps[-1]['step']=='verdict' and steps[-1]['rc']==0,'missing strict replay verdict for this manifest'
 assert cases.get('format')=='CALLBACK_LOWER_CASES1' and cases.get('cases'),'empty lower captures'
 assert a.seal_prefix!='tests/seals/librarycallbacke3','lower requires explicit lower seal-prefix'
 assert identity.get('selected_ids')==[x['id'] for x in cases['cases']] and all(x['part']==identity.get('part') for x in cases['cases']),'replay coverage does not match entire freeze manifest'
 models=spec['cache_models'];assert set(models)=={'tbl','net'}
 rows=[];origins=[];seen=set();positive=False;reasons=set()
 try:
  for case in cases['cases']:
   name=case['id'];assert name not in seen;seen.add(name)
   assert case['stage']=='lower:lnx/x86_64/run/O0','unverified lower target'
   assert case['kind'] in ('projection','refusal'),'lower positive must keep projection boundary'
   ledger=checked(case['ledger']);src=ledger.parent
   assert (src/'status').read_text().strip()==('1' if case['kind']=='refusal' else '0')
   target=copy_capture(src,name)
   inp=target/'input.bytes';shutil.copyfile(checked(case['input']),inp)
   refkey='reference_error' if case['kind']=='refusal' else 'reference';reference=checked(case[refkey]);ref=target/'expected.bytes';shutil.copyfile(reference,ref)
   if case['kind']=='refusal':assert not (src/'output.bytes').read_bytes();reasons.add(case['reason'])
   else:positive=True
   for fmt,model in models.items():
    assert not P(model).is_absolute() and '..' not in P(model).parts
    row={'id':name+'-'+fmt,'part':case['part'],'kind':case['kind'],'stage':case['stage'],'model':model,'input':desc(inp),'ledger':desc(target/'resources.tsv'),refkey:desc(ref)}
    if case['kind']=='refusal':row['reason']=case['reason']
    rows.append(row)
   origins.append({'case':name,'capture_manifest_sha256':sha(manifest),'input_sha256':sha(inp),'reference_sha256':sha(reference),'reference_kind':'shared-reference','replay_identity':identity})
  assert positive and len(reasons)>=2,'lower positive plus cycle/graph obligations'
  (O/'provenance.json').write_text(json.dumps(origins,indent=2)+'\n')
  (O/'cases.json').write_text(json.dumps({'format':'CALLBACK_LOWER_CASES1','cases':rows},indent=2)+'\n')
  print('frozen lower relative cases',len(rows),'rows; no full-lower/ABI claim')
 except Exception:
  subprocess.run(SAFEDEL+[str(O/'cases.json')],check=True);raise
 raise SystemExit(0)

rows=[];index=[];seen=set();reasons=set();accepted=False
try:
 for case in spec['cases']:
  name=case['id'];assert name not in seen and name.replace('-','').replace('_','').isalnum();seen.add(name)
  src=P(case['capture']).resolve(strict=True);identity=json.loads((src/'identity.json').read_text());steps=json.loads((src/'steps.json').read_text());want=case['expected_rc'];assert want in (0,1)
  assert steps and steps[-1]['rc']==want,'failed or incomplete capture'
  for key in ('input','ledger','model','source','contract','selection','producer'):
   assert key in identity;item=identity[key];assert sha(P(item['path']))==item['sha256'],'capture input changed'
  reference=checked(case['reference'])
  # The reference is explicitly external/frozen-control (or the shared semantic oracle), never candidate output self-reference.
  assert reference!=src/'output.bytes' and case['reference_kind'] in ('frozen-control','shared-reference')
  target=copy_capture(src,name)
  ref=target/'expected.bytes';shutil.copyfile(reference,ref)
  if want==0:
   assert (src/'output.bytes').read_bytes()==reference.read_bytes(),'accepted mismatch';kind='bytes';accepted=True
  else:
   assert not (src/'output.bytes').read_bytes(),'refusal produced main output'
   raw=(src/'diagnostic.bytes').read_bytes();assert raw==reference.read_bytes() and case['reason'].encode() in raw
   kind='refusal';reasons.add(case['reason'])
  for fmt in ('tbl','net'):
   row={'id':name+'-'+fmt,'part':1,'kind':kind,'stage':'e3:lnx/x86_64/tape/O0','model':'models/on/model.'+fmt,'input':desc(target/'input.bytes'),'ledger':desc(target/'resources.tsv')}
   row['reference' if want==0 else 'reference_error']=desc(ref)
   if want:row['reason']=case['reason']
   rows.append(row)
  index.append({'case':name,'identity':identity,'reference_kind':case['reference_kind'],'reference_sha256':sha(reference)})
 assert accepted and len(reasons)>=2,'positive and cycle/graph named refusal obligations required'
 (O/'provenance.json').write_text(json.dumps(index,indent=2)+'\n')
 (O/'cases.json').write_text(json.dumps({'format':'CALLBACK_LOWER_CASES1','cases':rows},indent=2)+'\n')
 print('frozen case plan',len(rows),'rows; replay still required, not runtime/ABI')
except Exception:
 subprocess.run(SAFEDEL+[str(O/'cases.json')],check=True)
 raise
