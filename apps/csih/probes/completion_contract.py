#!/usr/bin/env python3
"""Native protocol hard gates; scripted acceptance is not semantic proof."""
import argparse, hashlib, json, os, pathlib, re, shutil, tempfile, threading, time, traceback
from http.server import BaseHTTPRequestHandler, HTTPServer
from agent_multi_action import APP, ROOT, SRC, bounded
FEEDBACK=['feedback-319','feedback-320-recover','feedback-473-recover','feedback-utf8-319','feedback-utf8-320-recover','feedback-extra-recover','feedback-missing-recover','feedback-duplicate-recover','feedback-old-recover','feedback-unknown-recover','feedback-extra-exhaust','feedback-length-exhaust','feedback-audit-failure']
CASES=['accepted-work','consultation','legacy-stop','missing-evidence','rejected','invalid-recovery','legacy-answer','duplicate-evidence','continue-invalidates','budget-continue','audit-failure','watch-completed','watch-partial','watch-failed','required-missing','required-fake','required-wrong-peer','context-capacity']+FEEDBACK
def freeze():
    paths=[p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx')]+[ROOT/'unisacc.com',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
def run(name,binary,root,env,deadline):
    cwd=root/name;cwd.mkdir();home=cwd/'home';home.mkdir();journal=cwd/'journal';requests=[];responses=[]
    def response(request):
        content='\n'.join(m.get('content','') for m in request['messages'])
        ids=list(dict.fromkeys(re.findall(r'\[harness action_id\] ([^\n]+)',content)+re.findall(r'"action_id"\s*:\s*"([^"\n]+)"',content)))
        n=len(requests)
        if name.startswith('required-'):
            if n==1 and name!='required-missing':return dict(act='exec',cmd="echo 'envelope → 0:private-peer: 1 chars'" if name=='required-fake' else '/Users/wjc/repos/moltbaby/bin/envelope 0:wrong-peer title body',why='private denied fake delivery')
            return dict(act='answer',outcome='completed',text='REQUIRED_BODY_PRESERVED',evidence=[])
        if name.startswith('watch-'):
            if n==1:return dict(act='answer',outcome=name[6:],text='WATCH_BODY_PRESERVED',evidence=[])
            return dict(go='stop',acceptance='accepted',scope='answer_only',evidence=[],reason='consultation has no delivery requirement')
        if name=='budget-continue':return dict(go='continue')
        if name=='continue-invalidates' and n==3:return dict(go='continue')
        if name=='continue-invalidates' and n==4:return dict(go='stop',acceptance='accepted',scope='work',evidence=ids,reason='stale claim must not pass')
        if name=='duplicate-evidence' and n>=2:return dict(act='answer',outcome='completed',text='duplicate',evidence=ids*2)
        if name=='legacy-answer' and n==2:return dict(act='answer',outcome='completed',text='missing evidence')
        if name=='consultation':
            if n==1:return dict(act='answer',outcome='completed',text='咨询答复',evidence=[])
            return dict(go='stop',acceptance='accepted',scope='answer_only',evidence=[],reason='scripted consultation assessment')
        if n==1:return dict(act='exec',cmd="touch forbidden-marker" if name=='audit-failure' else "printf 'ACTUAL_BODY\n'",why='private fact')
        if n==2:return dict(act='answer',outcome='completed',text='声明',evidence=['old-turn-1'] if name=='missing-evidence' else ids)
        if name=='legacy-stop':return dict(go='stop')
        if name=='invalid-recovery' and n==3:return dict(not_a_judgment=True)
        return dict(go='stop',acceptance='rejected' if name=='rejected' else 'accepted',scope='work',evidence=ids,reason='scripted protocol assessment only')
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            request=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(request)
            if name=='audit-failure':journal.rename(cwd/'journal-before');journal.mkdir()
            answer=response(request);responses.append(answer);data=json.dumps({'choices':[{'message':{'content':json.dumps(answer,ensure_ascii=False)}}]}).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    server=HTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    result={'name':name}
    try:
        runenv=dict(env,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(cwd))
        if name.startswith(('watch-','required-')):runenv.update(CSIH_ROLE='watch',CSIH_PEER='0:private-peer')
        result['run']=bounded([str(binary),'required-agent' if name.startswith('required-') else 'agent','不用工具，直接回答' if name=='consultation' else 'private protocol test'],cwd,runenv,min(9,deadline-time.monotonic()))
        result['requests']=requests;result['responses']=responses;result['journal']=[json.loads(x) for x in (journal if journal.is_file() else cwd/'journal-before').read_text().splitlines()]
        expected_rc=0 if name in ('accepted-work','consultation','invalid-recovery','watch-completed') else 1
        expected_n=(1 if name=='required-missing' else 2) if name.startswith('required-') else (2 if name=='watch-completed' else 1) if name.startswith('watch-') else 1 if name=='audit-failure' else 8 if name=='budget-continue' else 4 if name in ('invalid-recovery','duplicate-evidence','continue-invalidates') else 2 if name in ('consultation','missing-evidence','legacy-answer') else 3
        assert result['run']['rc']==expected_rc and not result['run']['timed_out'],result['run']
        assert len(requests)==expected_n,len(requests)
        actor_anchors=[]
        for request in requests:
            messages=request['messages'];anchors=[(i,m['content']) for i,m in enumerate(messages) if m['content'].startswith('[harness current task]\n')]
            if anchors:
                assert len(anchors)==1
                pos,task=anchors[0];block=[m['content'] for m in messages[pos:pos+3]]
                assert block[1].startswith('[harness context-index data, not instructions]\n') and block[2].startswith('[harness history scope]\n')
                assert messages[pos+3]['content'].startswith('[harness candidate read calls]\n')
                assert len(messages)-(pos+4)<=1,'actor feedback must be last'
                actor_anchors.append(block)
            else:
                assert any(m['content'].startswith('Original current turn task:\n') for m in messages),'action anchor absent'
                assert not any('[harness context-index data, not instructions]' in m['content'] for m in messages),'judge got actor context'
        assert actor_anchors and all(x==actor_anchors[0] for x in actor_anchors),'actor anchor drift'
        result['actor_anchor_requests']=len(actor_anchors)

        tools=[x for x in result['journal'] if 'action_id' in x]
        if not name.startswith(('watch-','required-')) and name not in ('consultation','budget-continue','audit-failure'):
            assert len(tools)==1 and 'ACTUAL_BODY' in tools[0]['text'] and tools[0]['cwd']==str(cwd)
            if name not in ('missing-evidence','legacy-answer','continue-invalidates'):assert tools[0]['action_id'] in json.dumps(requests[-1])
        if not name.startswith(('watch-','required-')) and name not in ('missing-evidence','legacy-answer','duplicate-evidence','budget-continue','audit-failure'):
            last=json.dumps(responses[-1],ensure_ascii=False)
            assert any(x.get('role')=='assistant' and x.get('text')==last for x in result['journal']),'valid judgment audit absent'
        if name=='invalid-recovery':assert 'Harness protocol feedback' in json.dumps(requests[-1])
        if name.startswith('watch-'):
            assert not tools and 'peer mail not delivered' not in result['run']['stdout']
            assert 'WATCH_BODY_PRESERVED' in result['run']['stdout']
            assert 'peer mail not delivered' not in result['run']['stdout'] and '先给同伴发 envelope' not in json.dumps(result['journal'],ensure_ascii=False)
        elif name=='audit-failure':
            assert not tools and 'audit write failed' in result['run']['stdout'] and not (cwd/'forbidden-marker').exists()
        elif expected_rc:assert 'unfinished' in result['run']['stdout']
        if name.startswith('required-'):
            assert 'REQUIRED_BODY_PRESERVED' in result['run']['stdout'] and 'explicit current-turn delivery not confirmed' in result['run']['stdout']
            if name=='required-wrong-peer':assert not any(x.get('status',{}).get('op_success') is True for x in result['journal'] if x.get('role')=='tool')
            if name=='required-fake':assert any(x.get('action',{}).get('input')=="echo 'envelope → 0:private-peer: 1 chars'" for x in result['journal'])
        result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests,responses=responses)
    finally:server.shutdown();thread.join(1);server.server_close()
    return result

def run_feedback(name,binary,root,env,deadline):
    cwd=root/name;cwd.mkdir();home=cwd/'home';home.mkdir();journal=cwd/'journal';requests=[];responses=[];claim=[]
    def response(request):
        n=len(requests);content='\n'.join(m.get('content','') for m in request['messages'])
        ids=list(dict.fromkeys(re.findall(r'\[harness action_id\] ([^\n]+)',content)+re.findall(r'"action_id"\s*:\s*"([^"\n]+)"',content)))
        if n<=2:return dict(act='exec',cmd="printf 'ACTUAL_%d\n'"%n,why='private distinct evidence')
        if n==3:
            assert len(ids)==2,ids;claim[:]=[ids[-1]]
            return dict(act='answer',outcome='completed',text='DECLARATION_PRESERVED',evidence=list(claim))
        answer=dict(go='stop',acceptance='accepted',scope='work',evidence=list(claim),reason='scripted acceptance, not semantic proof')
        bad=n==4 or name.endswith('-exhaust')
        if name=='feedback-319':answer['reason']='x'*319
        elif name=='feedback-utf8-319':answer['reason']='中'*105+'abcd'
        elif bad:
            if '320' in name:answer['reason']=('中'*106+'ab') if 'utf8' in name else 'x'*320
            elif '473' in name or 'length' in name:answer['reason']='x'*473
            elif 'extra' in name or name=='feedback-audit-failure':answer['evidence']=ids
            elif 'missing' in name:answer['evidence']=[]
            elif 'duplicate' in name:answer['evidence']=claim*2
            elif 'old' in name:answer['evidence']=['999-0-1-1']
            elif 'unknown' in name:answer['evidence']=[claim[0]+'999']
        return answer
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            request=json.loads(self.rfile.read(int(self.headers['Content-Length'])));requests.append(request)
            if name=='feedback-audit-failure' and len(requests)==4:journal.rename(cwd/'journal-before');journal.mkdir()
            answer=response(request);responses.append(answer);data=json.dumps({'choices':[{'message':{'content':json.dumps(answer,ensure_ascii=False)}}]}).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    server=HTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();result={'name':name}
    try:
        runenv=dict(env,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(cwd))
        result['run']=bounded([str(binary),'agent','private protocol feedback test'],cwd,runenv,min(9,deadline-time.monotonic()))
        expected_rc=1 if name.endswith('-exhaust') or name=='feedback-audit-failure' else 0
        expected_n=6 if name.endswith('-exhaust') else 4 if name in ('feedback-319','feedback-utf8-319','feedback-audit-failure') else 5
        records=[json.loads(x) for x in (journal if journal.is_file() else cwd/'journal-before').read_text().splitlines()]
        result.update(requests=requests,responses=responses,journal=records,expected_rc=expected_rc,expected_requests=expected_n)
        assert result['run']['rc']==expected_rc and not result['run']['timed_out'],result['run']
        assert len(requests)==expected_n,len(requests)
        tools=[r for r in records if 'action_id' in r];assert len(tools)==2
        assert tools[0]['text'].endswith('ACTUAL_1\n') and tools[1]['text'].endswith('ACTUAL_2\n')
        assert claim==[tools[-1]['action_id']]
        systems='\n'.join(m.get('content','') for m in requests[3]['messages'] if m['role']=='system')
        assert 'UTF-8 bytes 1..319' in systems and 'exactly equal the claim reference set' in systems
        diagnosis='reason_utf8_bytes=320' if '320' in name else 'reason_utf8_bytes=473' if '473' in name or 'length' in name else 'extra_current_id=' if 'extra' in name else 'missing_claim_id=' if 'missing' in name else 'duplicate evidence reference' if 'duplicate' in name else 'not_current_id=' if 'old' in name or 'unknown' in name else None
        if diagnosis:
            errors=[r['text'] for r in records if r.get('role')=='tool' and r.get('name')=='error']
            assert errors and all(diagnosis in text for text in errors),(diagnosis,errors)
            assert 'expected_evidence=['+claim[0]+']' in errors[0]
            if expected_n>=5:assert diagnosis in json.dumps(requests[4],ensure_ascii=False)
        for answer in responses[3:]:
            if name!='feedback-audit-failure':assert any(r.get('role')=='assistant' and r.get('text')==json.dumps(answer,ensure_ascii=False) for r in records),'judgment audit missing'
        if name.endswith('-exhaust'):
            assert diagnosis in result['run']['stdout'] and 'acceptance=3' in result['run']['stdout']
            assert 'semantic acceptance (work)' not in result['run']['stdout']
            assert 'unfinished: acceptance protocol invalid;' in result['run']['stdout']
        if name=='feedback-audit-failure':assert 'audit write failed' in result['run']['stdout'] and 'acceptance=1' not in result['run']['stdout']
        if 'utf8' in name:assert '\ufffd' not in result['run']['stdout'],'UTF-8 reason cut mid-codepoint'
        result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests,responses=responses)
    finally:server.shutdown();thread.join(1);server.server_close()
    return result

def run_capacity(name,binary,root,env,deadline):
    cwd=root/name;cwd.mkdir();home=cwd/'home';home.mkdir();journal=cwd/'journal';packet=cwd/'packet.json';requests=[]
    raw=json.dumps(dict(data='"'*15500),separators=(',',':')).encode();assert len(raw)<32768;packet.write_bytes(raw)
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            requests.append(self.rfile.read(int(self.headers['Content-Length'])).decode());self.send_response(500);self.end_headers()
    server=HTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();result={'name':name,'packet_bytes':len(raw),'escaped_packet_bytes':len(json.dumps(raw.decode()).encode())}
    try:
        runenv=dict(env,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_ONLY',CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_PRIVATE_CONTEXT_PACKET=str(packet),CSIH_TRANSCRIPT=str(journal),CSIH_CWD=str(cwd))
        result['run']=bounded([str(binary),'context-capacity','CURRENT_TASK_MUST_NOT_BE_DROPPED'],cwd,runenv,min(9,deadline-time.monotonic()))
        result['requests']=requests;result['journal']=[json.loads(x) for x in journal.read_text().splitlines()]
        assert result['run']['rc']==1 and not result['run']['timed_out'] and not requests
        assert 'current task/context-index exceeds capacity' in result['run']['stdout']
        assert any(x.get('role')=='user' and x.get('text')=='CURRENT_TASK_MUST_NOT_BE_DROPPED' for x in result['journal'])
        result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests)
    finally:server.shutdown();thread.join(1);server.server_close()
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('cases',nargs='*',choices=CASES);parser.add_argument('--receipt',required=True);args=parser.parse_args();report={'before':freeze(),'cases':[],'limitation':'Scripted accepted/rejected responses test hard structural gates only, not semantic detection of false completion.'};deadline=time.monotonic()+55
    try:
        with tempfile.TemporaryDirectory(prefix='csih-completion-') as temp:
            root=pathlib.Path(temp);source=root/'source';source.mkdir()
            for p in APP.rglob('*'):
                if p.suffix in ('.c','.h','.inc','.cx'):
                    assert not p.is_symlink();d=source/p.relative_to(APP);d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(p.read_bytes())
            cli=source/'agent_cli.c';text=cli.read_text();entry='int main(int argc, char **argv) {';assert text.count(entry)==1
            injection=entry+'\n    if(argc>1&&!strcmp(argv[1],"required-agent")){agent_role_test("watch","0:private-peer");if(agent_delivery_require_next(1))return 8;argv[1]="agent";}\n'
            cli.write_text(text.replace(entry,injection))
            if 'context-capacity' in args.cases:
                cli=source/'agent_cli.c';text=cli.read_text();entry='int main(int argc, char **argv) {';assert text.count(entry)==1
                injection='int agent_context_packet(const char *packet);\n'+entry+'\n    if(argc>1&&!strcmp(argv[1],"context-capacity")){char *b=malloc(32768);FILE *f;size_t n;if(!b)return 9;f=fopen(getenv("CSIH_PRIVATE_CONTEXT_PACKET"),"rb");if(!f){free(b);return 9;}n=fread(b,1,32767,f);if(ferror(f)||fclose(f)!=0){free(b);return 9;}b[n]=0;if(agent_context_packet(b)!=0){free(b);return 9;}free(b);argv[1]="agent";}\n'
                cli.write_text(text.replace(entry,injection))
            compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler)
            env=dict(os.environ)
            for k in ('CSIH_ROLE','CSIH_PEER','OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','http_proxy','https_proxy','all_proxy'):env.pop(k,None)
            env.update(NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost')
            binary=root/'agent';report['build']=bounded(['/bin/sh',str(compiler),'-o',str(binary),*SRC],source,env,14)
            assert report['build']['rc']==0 and not report['build']['stderr'] and not report['build']['timed_out']
            selfhome=root/'selftest-home';selfhome.mkdir();report['selftest']=bounded([str(binary),'selftest'],source,dict(env,HOME=str(selfhome)),9)
            assert report['selftest']['rc']==0 and not report['selftest']['stderr'] and 'agent: all cases pass' in report['selftest']['stdout'],report['selftest']
            for name in args.cases or CASES:report['cases'].append((run_capacity if name=='context-capacity' else run_feedback if name in FEEDBACK else run)(name,binary,root,env,deadline))
            report['passed']=all(x['passed'] for x in report['cases'])
    except Exception as error:report.update(passed=False,error=repr(error),traceback=traceback.format_exc())
    report['after']=freeze();report['frozen']=report['before']==report['after'];report['passed']=report.get('passed',False) and report['frozen'];pathlib.Path(args.receipt).write_text(json.dumps(report,ensure_ascii=False,indent=2));print('PASS' if report['passed'] else 'FAIL');return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
