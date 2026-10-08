#!/usr/bin/env python3
"""Native working-set/page coverage protocol proof, not semantic-model proof."""
import json,os,pathlib,shutil,tempfile,hashlib,threading,time,traceback,re
from http.server import HTTPServer,BaseHTTPRequestHandler
from agent_multi_action import APP,ROOT,SRC,bounded

def freeze():return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]}
def main():
 import argparse
 a=argparse.ArgumentParser();a.add_argument('case',choices=['pages','invalid','gap','duplicate','nul','unknown-concern','late-invalid','auditfail','old-id','escaped','continue-version']);a.add_argument('--receipt',required=True);args=a.parse_args();r={'before':freeze()};root=pathlib.Path(tempfile.mkdtemp(prefix='csih-working-proof-'));r['retained_root']=str(root);deadline=time.monotonic()+55
 try:
  source=root/'source';source.mkdir()
  for p in APP.rglob('*'):
   if p.suffix in ('.c','.h','.inc'):
    d=source/p.relative_to(APP);d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(p.read_bytes())
  compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler);binary=root/'agent';env=dict(os.environ)
  for k in ('CSIH_ROLE','CSIH_PEER','OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','http_proxy','https_proxy','all_proxy'):env.pop(k,None)
  home=root/'home';home.mkdir();env.update(HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost')
  r['build']=bounded(['/bin/sh',str(compiler),'-o',str(binary),*SRC],source,env,14);assert r['build']['rc']==0 and not r['build']['stderr'],r['build']
  fixture=root/'body.txt';body='OP_RETURNED_ZERO_BUT_TEST_FAIL\n'+''.join('DATA%04d '%i+'x'*75+'\n' for i in range(200));fixture.write_text(body if args.case!='escaped' else '\x01'*17000);journal=root/'journal.jsonl';requests=[];responses=[];pages=[];actor_steps=0;first_id=None;finals=0;injected=False
  class H(BaseHTTPRequestHandler):
   def log_message(self,*a):pass
   def do_POST(self):
    nonlocal actor_steps,first_id,finals,injected
    q=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(q);msgs=q['messages'];texts='\n'.join(m['content'] for m in msgs)
    expected=next((m['content'].split('\n',1)[1] for m in msgs if m['content'].startswith('Expected strict page response')),None)
    records=[json.loads(x) for x in journal.read_text().splitlines()] if journal.is_file() else []
    tools=[x for x in records if x.get('action_id')];first_id=tools[0]['action_id'] if tools else first_id
    if expected:
     answer=json.loads(expected);pages.append(answer.copy());answer['observations']=[]
     if args.case=='invalid':answer['turn_id']='old-turn'
     elif args.case=='gap':answer['ranges'][0]['start']+=1
     elif args.case=='duplicate':answer['ranges'].append(answer['ranges'][0])
     elif args.case=='nul':answer['turn_id']+='\x00forged'
     elif args.case=='unknown-concern':answer['observations']=[dict(id='old-id',start=0,end=1,note='forged')]
     elif args.case=='late-invalid' and len(pages)>1:answer['page']+=1
     elif args.case=='auditfail' and not injected:
      journal.rename(root/'journal-backup');journal.mkdir();injected=True
     else:
      for span in answer['ranges']:
       source=next(x for x in tools if x['action_id']==span['id']);idx=source['text'].find('OP_RETURNED_ZERO_BUT_TEST_FAIL')
       if idx>=span['start'] and idx+len('OP_RETURNED_ZERO_BUT_TEST_FAIL')<=span['end']:
        answer['observations']=[dict(id=span['id'],start=idx,end=idx+len('OP_RETURNED_ZERO_BUT_TEST_FAIL'),note='Possible semantic counterevidence: source FAIL despite successful read')];break
    elif any(m['content'].startswith('Original current turn task:') for m in msgs):
     finals+=1;answer=dict(go='continue',reason='One bounded follow-up step') if args.case=='continue-version' and finals==1 else dict(go='stop',acceptance='accepted',scope='work',evidence=[first_id],reason='scripted mechanical coverage only')
    else:
     actor_steps+=1
     if args.case=='old-id':answer=dict(act='file',op='read',evidence_id='old-turn-1') if actor_steps==1 else dict(act='answer',outcome='failed',text='old evidence rejected',evidence=[])
     elif actor_steps<=12:answer=dict(act='file',op='read',path=str(fixture),byte_offset=(actor_steps%3)*4096,max_bytes=4096)
     elif actor_steps==13:answer=dict(act='file',op='read',evidence_id=first_id,max_bytes=4096)
     elif args.case=='continue-version' and actor_steps==15:answer=dict(act='file',op='read',path=str(fixture),max_bytes=32)
     else:answer=dict(act='answer',outcome='completed',text='bounded mechanical observations',evidence=[first_id])
    responses.append(answer);b=json.dumps({'choices':[{'message':{'content':json.dumps(answer)}}]}).encode();self.send_response(200);self.send_header('Content-Length',str(len(b)));self.end_headers();self.wfile.write(b)
  server=HTTPServer(('127.0.0.1',0),H);t=threading.Thread(target=server.serve_forever,daemon=True);t.start()
  try:r['run']=bounded([str(binary),'agent','CURRENT_TASK examine current recorded observations'],root,dict(env,CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(root)),min(35,deadline-time.monotonic()))
  finally:server.shutdown();server.server_close();t.join(2)
  r.update(requests=requests,responses=responses,pages=pages,actor_steps=actor_steps,finals=finals);r['journal']=[json.loads(x) for x in (journal if journal.is_file() else root/'journal-backup').read_text().splitlines()]
  tools=[x for x in r['journal'] if x.get('action_id')]
  assert not r['run']['timed_out'],r['run']
  if args.case=='continue-version':
   assert r['run']['rc']==0 and actor_steps==16 and finals==2 and len(tools)==14,r['run']
   for version,count in [(1,13),(2,14)]:
    coverage={x['action_id']:[] for x in tools[:count]}
    group=[p for p in pages if p['claim_version']==version];assert group and all(p['ledger_version']==count for p in group)
    for p in group:
     for z in p['ranges']:coverage[z['id']].append((z['start'],z['end']))
    for x in tools[:count]:
     end=0
     for begin,last in coverage[x['action_id']]:assert begin==end;end=last
     assert end==len(x['text'].encode())
   assert tools[-1]['action']['op']=='read' and tools[-1]['text'].startswith('OP_RETURNED_ZERO')
  elif args.case in ('pages','escaped'):
   assert r['run']['rc']==0 and actor_steps==14 and finals==1 and pages,r['run']
   assert len(tools)==13 and tools[-1]['action']['op']=='read_evidence' and tools[-1]['text']==tools[0]['text']
   assert tools[-1]['action']['evidence_source']['source_id']==first_id
   coverage={x['action_id']:[] for x in tools}
   for p in pages:
    assert p['ledger_version']==13 and p['claim_version']==1
    for z in p['ranges']:coverage[z['id']].append((z['start'],z['end']))
   for x in tools:
    end=0
    for begin,last in coverage[x['action_id']]:assert begin==end;end=last
    assert end==len(x['text'].encode())
   packet='\n'.join(m['content'] for m in requests[-1]['messages']);assert args.case=='escaped' or ('Append-only concern with original counterevidence' in packet and 'OP_RETURNED_ZERO_BUT_TEST_FAIL' in packet)
   actor_after_retrieval=next(q for q in requests if any('[harness evidence retrieval]' in m['content'] for m in q['messages']));assert actor_after_retrieval
   assert all(x['status']['op_success'] for x in tools)
  elif args.case in ('invalid','gap','duplicate','nul','unknown-concern','late-invalid'):assert r['run']['rc']==1 and len(pages)==3 and not finals
  elif args.case=='auditfail':assert r['run']['rc']==1 and len(pages)==1 and not finals and 'assistant audit write failed' in r['run']['stdout']
  else:assert r['run']['rc']==1 and actor_steps==2 and 'old or unknown current ID' in tools[0]['text']
  r['passed']=True
 except Exception as e:r.update(passed=False,error=repr(e),traceback=traceback.format_exc())
 r['after']=freeze();r['frozen']=r['before']==r['after'];r['passed']=r.get('passed',False) and r['frozen'];pathlib.Path(args.receipt).write_text(json.dumps(r,ensure_ascii=False,indent=2));print('PASS' if r['passed'] else 'FAIL');return 0 if r['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
