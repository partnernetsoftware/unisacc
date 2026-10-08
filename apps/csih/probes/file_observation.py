#!/usr/bin/env python3
"""Real native file windows and HTTP presentation; no semantic-model claim."""
import json,os,pathlib,shutil,tempfile,hashlib,threading,time,traceback,sys
from http.server import HTTPServer,BaseHTTPRequestHandler
from agent_multi_action import APP,ROOT,SRC,bounded

def freeze():return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]}
def main():
 import argparse
 a=argparse.ArgumentParser();a.add_argument('--receipt',required=True);args=a.parse_args();r={'before':freeze(),'cases':[]};deadline=time.monotonic()+55
 root=pathlib.Path(tempfile.mkdtemp(prefix='csih-file-observation-'));r['retained_root']=str(root)
 try:
  source=root/'source';source.mkdir()
  for p in APP.rglob('*'):
   if p.suffix in ('.c','.h','.inc'):
    d=source/p.relative_to(APP);d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(p.read_bytes())
  cli=source/'agent_cli.c';s=cli.read_text();entry='int main(int argc, char **argv) {';assert s.count(entry)==1
  s=s.replace(entry,'int csih_private_read(const char*,const char*);\n'+entry+'\n if(argc==3&&!strcmp(argv[1],"read-proof"))return csih_private_read(argv[2],".");\n');cli.write_text(s)
  with (source/'agent.c').open('a') as f:f.write('\nint csih_private_read(const char *raw,const char *cwd){agent_step s=agent_parse(raw);char out[16384],rec[60000];if(s.kind!=ACT_READ){puts("{\\"parse_rejected\\":true}");return 0;}agent_tool_reset();agent_exec(&s,cwd,out,sizeof out);if(!json_rec(rec,sizeof rec,"body",out,"observation",agent_read_observation,NULL,NULL))return 9;printf("%s\\n",rec);return 0;}\n')
  compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler);binary=root/'agent';env=dict(os.environ)
  for k in ('CSIH_ROLE','CSIH_PEER','OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','http_proxy','https_proxy','all_proxy'):env.pop(k,None)
  home=root/'home';home.mkdir();env.update(HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost')
  r['build']=bounded(['/bin/sh',str(compiler),'-o',str(binary),*SRC],source,env,14);assert r['build']['rc']==0 and not r['build']['stderr'],r['build']
  r['selftest']=bounded([str(binary),'selftest'],root,env,9);assert r['selftest']['rc']==0 and 'agent: all cases pass' in r['selftest']['stdout'],r['selftest']
  def read(name,path,**kw):
   cmd=dict(act='file',op='read',path=str(path),**kw);z=bounded([str(binary),'read-proof',json.dumps(cmd)],root,env,3);assert z['rc']==0,z;v=json.loads(z['stdout']);r['cases'].append(dict(name=name,command=cmd,result=v));return v
  long=root/'long.txt';long.write_bytes(b'A'*5000+b'\r\nSECOND\r\nTHIRD');assert read('physical-line2',long,line=2,n=1)['body']=='SECOND\r\n'
  v=read('first-long-window',long,max_bytes=4096);joined=v['body'];o=json.loads(v['observation']);assert o['byte_end']==4096 and not o['line_complete'] and o['total_lines'] is None
  while not o['eof']:
   v=read('cursor-continuation',long,byte_offset=o['next_byte_offset'],max_bytes=4096);joined+=v['body'];o=json.loads(v['observation'])
  assert joined.encode()==long.read_bytes()
  utf=root/'utf.txt';utf.write_bytes('中\r\n文'.encode());assert 'read failed' in read('no-progress',utf,max_bytes=1)['body'];assert 'read failed' in read('middle-char',utf,byte_offset=1)['body'];assert read('utf-boundary',utf,max_bytes=3)['body']=='中'
  large=root/'large.txt';large.write_bytes(b'CURRENT_DEPLOYMENT_ACTIVATED\n'+b'A'*(9*1024*1024)+b'\xff');v=read('large-prefix',large);assert v['body'].startswith('CURRENT_DEPLOYMENT_ACTIVATED') and len(v['body'].encode())==4096
  fifo=root/'fifo';os.mkfifo(fifo);assert 'read failed' in read('fifo',fifo)['body']
  for name,kw in [('fraction',dict(line=1.5)),('dual',dict(line=1,byte_offset=0)),('max-over',dict(max_bytes=8193))]:assert read(name,long,**kw)['parse_rejected']
  middle=root/'middle.txt';middle.write_text(''.join('LINE%03d '%i+('ACTIVATED_COMMITTED' if i==150 else 'x'*30)+'\n' for i in range(1,301)))
  requests=[];journal=root/'journal.jsonl'
  class H(BaseHTTPRequestHandler):
   def log_message(self,*a):pass
   def do_POST(self):
    q=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(q)
    answer=dict(act='file',op='read',path=str(middle),line=130,n=40,max_bytes=8192) if len(requests)==1 else dict(act='answer',outcome='partial',text='bounded observation',evidence=[])
    b=json.dumps({'choices':[{'message':{'content':json.dumps(answer)}}]}).encode();self.send_response(200);self.send_header('Content-Length',str(len(b)));self.end_headers();self.wfile.write(b)
  server=HTTPServer(('127.0.0.1',0),H);t=threading.Thread(target=server.serve_forever,daemon=True);t.start()
  try:r['wire_run']=bounded([str(binary),'agent','read relevant window'],root,dict(env,CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(root)),min(9,deadline-time.monotonic()))
  finally:server.shutdown();server.server_close();t.join(2)
  r['requests']=requests;r['journal']=[json.loads(x) for x in journal.read_text().splitlines()];record=next(x for x in r['journal'] if x.get('read_call'));o=record['read_call']['observation'];assert 'ACTIVATED_COMMITTED' in record['text'] and o['presentation_complete'];wire='\n'.join(m['content'] for m in requests[1]['messages']);assert record['text'] in wire and '[harness action_id] '+record['action_id'] in wire and '[harness read_call]' in wire and '[harness status] unknown' not in wire
  assert r['wire_run']['rc']==1 and len(requests)==2
  r['extra_wire']=[]
  control=root/'control.txt';control.write_bytes(b'\x01'*20000)
  for mode in ('judge','capacity'):
   requests=[];journal=root/(mode+'.jsonl')
   class More(BaseHTTPRequestHandler):
    def log_message(self,*a):pass
    def do_POST(self):
     q=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(q);n=len(requests)
     import re
     text='\n'.join(m['content'] for m in q['messages']);ids=list(dict.fromkeys(re.findall(r'\[harness action_id\] ([^\n]+)',text)+re.findall(r'"action_id"\s*:\s*"([^"\n]+)"',text)))
     if mode=='capacity':answer=dict(act='file',op='read',path=str(control),max_bytes=8192) if n<=2 else dict(act='answer',outcome='partial',text='bounded escaped bodies preserved',evidence=[])
     elif n==1:answer=dict(act='file',op='read',path=str(middle),line=130,n=40,max_bytes=8192)
     elif n==2:answer=dict(act='answer',outcome='completed',text='observed bounded marker',evidence=ids)
     else:answer=dict(go='stop',acceptance='accepted',scope='work',evidence=ids or [record['action_id']],reason='scripted window only')
     b=json.dumps({'choices':[{'message':{'content':json.dumps(answer)}}]}).encode();self.send_response(200);self.send_header('Content-Length',str(len(b)));self.end_headers();self.wfile.write(b)
   server=HTTPServer(('127.0.0.1',0),More);t=threading.Thread(target=server.serve_forever,daemon=True);t.start()
   try:result=bounded([str(binary),'agent','observe relevant bounded data'],root,dict(env,CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(root)),min(9,deadline-time.monotonic()))
   finally:server.shutdown();server.server_close();t.join(2)
   records=[json.loads(x) for x in journal.read_text().splitlines()];entry=dict(mode=mode,run=result,requests=requests,journal=records);r['extra_wire'].append(entry)
   if mode=='judge':
    assert result['rc']==0 and len(requests)==3,result
    tools=[x for x in records if x.get('read_call')];assert len(tools)==1
    packet='\n'.join(m['content'] for m in requests[2]['messages']);assert tools[0]['action_id'] in packet and 'ACTIVATED_COMMITTED' in packet and 'presentation_complete' in packet
    assert tools[0]['text']==record['text'] and tools[0]['read_call']['observation']==record['read_call']['observation']
   else:
    assert result['rc']==1 and len(requests)==3 and not result['timed_out'],result
    tools=[x for x in records if x.get('read_call')];assert len(tools)==2 and all(x['text']=='\x01'*8192 for x in tools)
    assert all(x['read_call']['observation']['presentation_complete'] for x in tools)
    actual='\n'.join(m['content'] for m in requests[-1]['messages']);assert 'ledger_directory' in actual and all(x['action_id'] in actual for x in tools)
    entry['contract_migration']='B working-set preserves both raw windows and current IDs; old immediate capacity failure expectation superseded by bounded omission, final partial remains non-success.'
  r['passed']=True
 except Exception as e:r.update(passed=False,error=repr(e),traceback=traceback.format_exc())
 r['after']=freeze();r['frozen']=r['before']==r['after'];r['passed']=r.get('passed',False) and r['frozen'];pathlib.Path(args.receipt).write_text(json.dumps(r,ensure_ascii=False,indent=2));print('PASS' if r['passed'] else 'FAIL');return 0 if r['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
