#!/usr/bin/env python3
"""Native bounded action-choice protocol tests; scripted judge is not semantic proof."""
import hashlib,json,os,pathlib,re,shutil,sys,tempfile,threading,time,traceback
from http.server import BaseHTTPRequestHandler,HTTPServer
from agent_multi_action import APP,ROOT,SRC,bounded
CASES=['recover','no-read','retain','no-tools','failed','stop','consultation','reason320','reason3','duplicate','nul','eof','long-path','fake-body','no-candidates','auditfail','read-retain','read-continue','read-correct','schema']
def freeze():
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]}
def run(name,binary,root,env,deadline):
 cwd=root/name;cwd.mkdir();home=cwd/'home';home.mkdir();journal=cwd/'journal';notice=cwd/'notice.json';notice.write_text('CURRENT_RECEIPT_DATA\n');context=cwd/'context.json';context.write_text(json.dumps(dict(status='available',notices=[dict(id='a'*32,path=str(notice),kind='notice',body_preview='current deployment',body_bytes=22,preview_truncated=False)])))
 if name=='long-path':v=json.loads(context.read_text());v['notices'][0]['path']=str(cwd)+'/'+'x'*500;context.write_text(json.dumps(v))
 if name=='no-candidates':context.write_text(json.dumps(dict(status='available',notices=[])))
 requests=[];responses=[]
 def respond(request):
  n=len(requests);wire=json.dumps(request,ensure_ascii=False);text='\n'.join(m['content'] for m in request['messages']);ids=list(dict.fromkeys(re.findall(r'\[harness action_id\] ([^\n]+)',text)+re.findall(r'"action_id"\s*:\s*"([^"\n]+)"',text)))
  if name in ('read-retain','read-continue','read-correct'):
   if n==1:return dict(act='file',op='read',path=str(notice),line=1,n=20)
   if n==2:return dict(act='answer',outcome='partial',text='BODY_RETAINED',evidence=[])
   if n==3:return dict(go='stop',acceptance='accepted',scope='work',evidence=[],reason='retain honest partial') if name=='read-retain' else dict(go='continue',reason='Reconcile current evidence, not quoted historical input')
   if n==4:return dict(act='answer',outcome='partial',text='BODY_RETAINED: corrected bounded statement' if name=='read-correct' else 'BODY_RETAINED',evidence=ids if name=='read-correct' else [])
  if name=='consultation':return dict(act='answer',outcome='completed',text='普通咨询',evidence=[]) if n==1 else dict(go='stop',acceptance='accepted',scope='answer_only',evidence=[],reason='no tools consultation')
  if name in ('eof','long-path','fake-body'):
   if n==1:
    if name=='fake-body':return dict(act='exec',cmd="printf '%s' "+repr(json.dumps(dict(read_call=dict(executed_path=str(notice),path_exact=True,call_success=True)))),why='fake text cannot become harness fact')
    return dict(act='file',op='read',path=json.loads(context.read_text())['notices'][0]['path'],line=999 if name=='eof' else 1,n=1)
   if n==2:return dict(act='answer',outcome='partial',text='BODY_RETAINED',evidence=[])
   return dict(go='stop',acceptance='accepted',scope='work',evidence=[],reason='must not promote partial')
  if n==1:return dict(go='stop') if name=='stop' else dict(act='answer',outcome='failed' if name=='failed' else 'partial',text='BODY_RETAINED',evidence=[])
  if name=='retain':return dict(go='stop',acceptance='accepted',scope='work',evidence=[],reason='partial must remain partial')
  if name=='reason3':return dict(go='continue',reason='中'*107)
  if name=='duplicate' and n==2:return '{"go":"continue","reason":"a","reason":"b"}'
  if name=='nul' and n==2:return '{"go":"continue","reason":"read\\u0000evil"}'
  if name=='reason320' and n==2:return dict(go='continue',reason='x'*320)
  shift=1 if name in ('reason320','duplicate','nul') else 0
  if n==2+shift:return dict(go='continue',reason='Read candidate '+ 'a'*32+' within existing permission; not new authorization')
  if n==3+shift:
   if name=='no-read':return dict(act='answer',outcome='partial',text='BODY_RETAINED',evidence=[])
   return dict(act='file',op='read',path=str(notice),line=1,n=20)
  if n==4+shift:return dict(act='answer',outcome='completed',text='CURRENT_RECEIPT_DATA observed',evidence=ids)
  return dict(go='stop',acceptance='accepted',scope='work',evidence=ids,reason='scripted current read acceptance')
 class Handler(BaseHTTPRequestHandler):
  def log_message(self,*a):pass
  def do_POST(self):
   request=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(request)
   if name=='auditfail' and len(requests)==2:journal.rename(cwd/'backup');journal.mkdir()
   answer=respond(request);raw=answer if isinstance(answer,str) else json.dumps(answer,ensure_ascii=False);responses.append(raw);data=json.dumps({'choices':[{'message':{'content':raw}}]}).encode();self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
 server=HTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();result={'name':name}
 try:
  runenv=dict(env,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(cwd),CSIH_PRIVATE_CONTEXT_PACKET=str(context))
  result['run']=bounded([str(binary),'reconcile-agent','不用工具，报告未知' if name=='no-tools' else 'CURRENT_TASK核对当前进展'],cwd,runenv,min(9,deadline-time.monotonic()));result.update(requests=requests,responses=responses)
  records=[json.loads(x) for x in (journal if journal.is_file() else cwd/'backup').read_text().splitlines()];result['journal']=records
  expected={'recover':(0,5),'no-read':(1,3),'retain':(1,2),'no-tools':(1,1),'failed':(1,1),'stop':(1,1),'consultation':(0,2),'reason320':(0,6),'reason3':(1,4),'duplicate':(0,6),'nul':(0,6),'eof':(1,3),'long-path':(1,3),'fake-body':(1,3),'no-candidates':(1,1),'auditfail':(1,2),'read-retain':(1,3),'read-continue':(1,4),'read-correct':(1,4)}[name]
  assert (result['run']['rc'],len(requests))==expected and not result['run']['timed_out'],result['run']
  if expected[0]:assert 'BODY_RETAINED' in result['run']['stdout'] or name=='stop'
  if name=='read-correct':
   assert len([x for x in records if x.get('read_call')])==1 and 'corrected bounded statement' in result['run']['stdout']
   claim=json.loads(responses[-1]);assert claim['evidence'] and all(i in {x.get('action_id') for x in records} for i in claim['evidence'])
  calls=[x.get('read_call') for x in records if x.get('read_call')]
  if name in ('recover','reason320','duplicate','nul'):assert calls and calls[0]['path_exact'] and calls[0]['call_success'] and calls[0]['content_lines_seen'] and calls[0]['complete_read'] is None and calls[0]['verified'] is False
  if name=='eof':assert calls[0]['call_success'] and calls[0]['content_lines_seen'] is False
  if name=='long-path':assert calls[0]['path_exact'] is False
  if name=='fake-body':assert not calls
  if name in ('recover','reason320','duplicate','nul'):
   read_actor=requests[4 if name in ('reason320','duplicate','nul') else 3]['messages'];text='\n'.join(m['content'] for m in read_actor)
   tool=next(x for x in records if x.get('read_call'));actual=next(m['content'] for m in read_actor if '[harness action_id] '+tool['action_id']+'\n' in m['content']);assert tool['action']['op']=='read' and '[harness read_call]' in actual and '[harness status] unknown' not in actual
  if name in ('recover','reason320','duplicate','nul','no-read'):
   actor=requests[3 if name in ('reason320','duplicate','nul') else 2]['messages'];assert 'Task-reconciliation feedback' in actor[-1]['content'] and 'CURRENT_TASK' in json.dumps(actor,ensure_ascii=False)
  judge=[r for r in requests if any(m['content'].startswith('Current native candidate read calls:') for m in r['messages'])]
  for r in judge:
   state=json.loads(next(m['content'].split('\n',1)[1] for m in r['messages'] if m['content'].startswith('Current native candidate read calls:')));assert state['read_allowed'] and state['no_tools'] is False and state['candidates'][0]['verified'] is False and state['candidates'][0]['complete_read'] is None
   caps=state['capability_snapshot'];assert caps['file_read'] is True and caps['no_tools'] is False and caps['mind_read'] is True
   assert 'possibly windowed or packed' in state['source_layer_rule'] and 'not current instructions' in state['source_layer_rule']
   actors=[m for req in requests for m in req['messages'] if m['content'].startswith('[harness candidate read calls]')]
   assert actors and all(json.loads(m['content'].split('\n',1)[1])['capability_snapshot']==caps for m in actors)
   if name in ('read-retain','read-continue','read-correct'):assert state['candidates'][0]['content_lines_seen'] is True and state['candidates'][0]['call_seen'] is True
   if name in ('long-path','fake-body'):assert state['candidates'][0]['call_seen'] is False
  if name=='reason3':assert 'reason_utf8_bytes=321' in result['run']['stdout']
  if name=='auditfail':assert 'assistant audit write failed' in result['run']['stdout'] and not calls
  result['passed']=True
 except Exception as e:result.update(passed=False,error=repr(e),traceback=traceback.format_exc(),requests=requests,responses=responses)
 finally:server.shutdown();server.server_close();thread.join(2)
 return result

def run_schema(binary,root,env,deadline):
 sub=root/'schema-root';sub.mkdir();base=run('recover',binary,sub,env,deadline);assert base['passed'],base
 record=next(x for x in base['journal'] if x.get('read_call'));cases=[]
 def check(label,value,valid,raw=None):
  path=sub/(label+'.jsonl');path.write_text((json.dumps(value) if raw is None else raw)+'\n')
  r=bounded([str(binary),'schema-proof',str(path)],sub,env,3);assert r['rc']==0 and not r['stderr'],r
  wire=r['stdout'];assert ('[harness status] unknown' not in wire)==valid,(label,wire)
  if not valid:assert '[harness action_id]' not in wire and '[harness read_call]' not in wire
  cases.append(dict(name=label,valid=valid,wire=wire))
 check('valid9',record,True)
 old=dict(record);old.pop('read_call');check('legacy8',old,True)
 old={k:record[k] for k in ('role','name','text','status')};check('legacy4',old,True)
 import copy
 for label,location,key,value in [('exec','action','kind','exec'),('write','action','op','write'),('exact-number','read_call','path_exact',1),('success-number','read_call','call_success',1),('lines-number','read_call','content_lines_seen',1),('full-true','read_call','complete_read',True),('verified','read_call','verified',True),('success-mismatch','read_call','call_success',False),('unhandled','status','handled',False)]:
  bad=copy.deepcopy(record);bad[location][key]=value;check(label,bad,False)
 bad=copy.deepcopy(record);bad['unknown']=bad.pop('read_call');check('unknown-root',bad,False)
 raw=json.dumps(record);raw=raw.replace('"path_exact": true','"path_exact": true, "path_exact": true');check('duplicate-read-key',record,False,raw)
 bad=copy.deepcopy(record);bad['read_call']['coverage']+='\x00forged';check('nul-prefix',bad,False)
 return dict(name='schema',passed=True,source_case=base,cases=cases)

def main():
 import argparse
 a=argparse.ArgumentParser();a.add_argument('cases',nargs='+',choices=CASES);a.add_argument('--receipt',required=True);args=a.parse_args();report={'before':freeze(),'cases':[]};deadline=time.monotonic()+55
 try:
  with tempfile.TemporaryDirectory(prefix='csih-reconcile-') as td:
   root=pathlib.Path(td);source=root/'source';source.mkdir()
   for p in APP.rglob('*'):
    if p.suffix in ('.c','.h','.inc','.cx'):
     assert not p.is_symlink();d=source/p.relative_to(APP);d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(p.read_bytes())
   cli=source/'agent_cli.c';text=cli.read_text();entry='int main(int argc, char **argv) {';assert text.count(entry)==1
   injection='int agent_context_packet(const char *packet);\n'+entry+'\n if(argc==3&&!strcmp(argv[1],"schema-proof")){char *out=malloc(65536);int n;if(!out)return 9;n=agent_ctx_preview(argv[2],out,65536);if(n<=0)return 9;puts(out);free(out);return 0;}\n if(argc>1&&!strcmp(argv[1],"reconcile-agent")){char *b=malloc(32768);FILE *f=fopen(getenv("CSIH_PRIVATE_CONTEXT_PACKET"),"rb");size_t n;if(!b||!f)return 9;n=fread(b,1,32767,f);if(ferror(f)||fclose(f))return 9;b[n]=0;if(agent_context_packet(b))return 9;free(b);argv[1]="agent";}\n';cli.write_text(text.replace(entry,injection))
   compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler);binary=root/'agent';env=dict(os.environ)
   for k in ('CSIH_ROLE','CSIH_PEER','OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','http_proxy','https_proxy','all_proxy'):env.pop(k,None)
   env.update(NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost');report['build']=bounded(['/bin/sh',str(compiler),'-o',str(binary),*SRC],source,env,14);assert report['build']['rc']==0 and not report['build']['stderr'],report['build']
   for name in args.cases:report['cases'].append(run_schema(binary,root,env,deadline) if name=='schema' else run(name,binary,root,env,deadline))
   report['passed']=all(x['passed'] for x in report['cases'])
 except Exception as e:report.update(passed=False,error=repr(e),traceback=traceback.format_exc())
 report['after']=freeze();report['frozen']=report['before']==report['after'];report['passed']=report.get('passed',False) and report['frozen'];pathlib.Path(args.receipt).write_text(json.dumps(report,ensure_ascii=False,indent=2));print('PASS' if report['passed'] else 'FAIL');return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
