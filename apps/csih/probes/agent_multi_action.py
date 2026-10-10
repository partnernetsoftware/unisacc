#!/usr/bin/env python3
"""Real default-native/local-HTTP multiple-action side effects, private only.
Run after agent.c and agent_cli.c freeze. A rejected multi-exec followed by
answer may return0; that row proves rejection/zero side effects, not honesty
of completion or a fix to every terminal-result policy.
"""
import argparse, hashlib, json, os, pathlib, shutil, signal, subprocess, sys, tempfile, threading, time, traceback
from http.server import BaseHTTPRequestHandler, HTTPServer
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1]
SRC=['agent.c','agent_cli.c','file.c','edit.c','shell.c','json.c','session.c','net.c','plugin.c']
def hashes():
    paths=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
def bounded(argv,cwd,env,limit):
    p=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:out,err=p.communicate(timeout=limit);timed=False
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);out,err=p.communicate();timed=True
    return dict(argv=argv,cwd=str(cwd),rc=p.returncode,timed_out=timed,stdout=out.decode(errors='replace'),stderr=err.decode(errors='replace'))
def action(**value):return json.dumps(value,ensure_ascii=False)
ANSWER=action(act='answer',text='done');STOP=action(go='stop')
WRITE1=action(act='file',op='write',path='first.txt',text='第一份中文\n')
WRITE2=action(act='file',op='write',path='second.txt',text='第二份中文\n')
DOUBLE_WRITE=WRITE1+'\n'+WRITE2
DOUBLE_EXEC=action(act='exec',cmd='touch first.txt',why='private first')+'\n'+action(act='exec',cmd='touch second.txt',why='private second')
CASES={
 'double-write-repeat':dict(responses=[DOUBLE_WRITE]*3,prompt='write the files',requests=3,rc=1,files={},cause='too many unparseable steps'),
 'double-fenced-leading':dict(responses=['说明：只发一件。\n```json\n'+DOUBLE_WRITE+'\n```']*3,prompt='write the files',requests=3,rc=1,files={},cause='too many unparseable steps'),
 'double-after-fence':dict(responses=['```json\n'+WRITE1+'\n```\n'+WRITE2]*3,prompt='write the files',requests=3,rc=1,files={},cause='too many unparseable steps'),
 'double-english-leading':dict(responses=['next step\n'+DOUBLE_WRITE]*3,prompt='write the files',requests=3,rc=1,files={},cause='too many unparseable steps'),
 'malformed-array-shell':dict(responses=['[ BROKEN '+WRITE1+' ]']*3,prompt='write one file',requests=3,rc=1,files={},cause='too many unparseable steps'),
 'double-write-recovery':dict(responses=[DOUBLE_WRITE,WRITE1,WRITE2,ANSWER,STOP],prompt='write the files',requests=5,rc=0,files={'first.txt':'第一份中文\n','second.txt':'第二份中文\n'}),
 'double-exec-rejected':dict(responses=[DOUBLE_EXEC,ANSWER,STOP],prompt='do the actions',requests=3,rc=0,files={},limit='rc0 after a separate answer is not proof that rejected work completed'),
 'single-fenced-braces':dict(responses=['说明：只执行一个对象。\n```json\n'+action(act='file',op='write',path='first.txt',text='字符串内 {中文} 与 \"引号\"\n')+'\n```',ANSWER,STOP],prompt='write one file',requests=3,rc=0,files={'first.txt':'字符串内 {中文} 与 "引号"\n'}),
 'no-tools-braces':dict(responses=['中文说明，示例代码 if (x) { return; }，只需直接答复。',STOP],prompt='不用工具，直接回答测试',requests=2,rc=0,files={}),
}
def run_case(name,definition,binary,root,env,deadline):
    cwd=root/name;cwd.mkdir(mode=0o700);home=cwd/'home';home.mkdir(mode=0o700);trace=cwd/'trace.jsonl';requests=[];server=None;thread=None
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            requests.append(json.loads(self.rfile.read(int(self.headers['Content-Length']))))
            content=definition['responses'][min(len(requests)-1,len(definition['responses'])-1)]
            data=json.dumps({'choices':[{'message':{'content':content}}]}).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    result=dict(name=name,expected_requests=definition['requests'],expected_rc=definition['rc'],responses=definition['responses'])
    try:
        server=HTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        runenv=dict(env,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ENDPOINT='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1],CSIH_MODEL='multi-action-stub',CSIH_CWD=str(cwd),CSIH_TRANSCRIPT=str(trace))
        (home/'env.jsonl').write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
        remaining=deadline-time.monotonic();assert remaining>0,'selector deadline before run'
        result.update(run=bounded([str(binary),'agent',definition['prompt']],cwd,runenv,min(9,remaining)))
        result['requests']=requests
        result['trace']=[json.loads(line) for line in trace.read_text().splitlines()] if trace.is_file() else []
        result['files_hex']={p.name:p.read_bytes().hex() for p in (cwd/'first.txt',cwd/'second.txt') if p.is_file()}
        expected={name:text.encode().hex() for name,text in definition['files'].items()}
        assert not result['run']['timed_out'],result['run']
        assert result['run']['rc']==definition['rc'] and len(requests)==definition['requests'],result
        assert result['files_hex']==expected,('actual side effects',result['files_hex'],expected)
        if 'cause' in definition:assert 'unfinished' in result['run']['stdout'] and definition['cause'] in result['run']['stdout']
        if name.startswith('double-'):
            assert any(record.get('name')=='error' for record in result['trace']),'no actual rejected-action error trace'
        if name=='no-tools-braces':assert any(record.get('role')=='assistant' and record.get('text')==definition['responses'][0] for record in result['trace'])
        result.update(passed=True,limitation=definition.get('limit'))
    except Exception as error:result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests)
    finally:
        if thread:server.shutdown();thread.join(timeout=1)
        if server:server.server_close()
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('cases',nargs='*',choices=list(CASES));parser.add_argument('--receipt',default='/tmp/csih-agent-multi-action-review.json');args=parser.parse_args()
    result=dict(before=hashes(),cases=[],scope='real native CLI/local HTTP, private side effects only');deadline=time.monotonic()+55
    with tempfile.TemporaryDirectory(prefix='csih-agent-multi-') as temporary:
        root=pathlib.Path(temporary);source=root/'source';source.mkdir(mode=0o700)
        try:
            for path in APP.rglob('*'):
                if path.suffix in ('.c','.h','.inc','.cx'):
                    assert not path.is_symlink();dest=source/path.relative_to(APP);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(path.read_bytes())
            compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler);compiler.chmod(0o600)
            assert hashlib.sha256(compiler.read_bytes()).hexdigest()==result['before'][str(ROOT/'unisacc.com')]
            env=dict(os.environ)
            for key in ('CSIH_ROLE','CSIH_PEER','OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','ALL_PROXY','http_proxy','https_proxy','all_proxy'):env.pop(key,None)
            env.update(NO_PROXY='127.0.0.1,localhost',no_proxy='127.0.0.1,localhost')
            binary=root/'agent-native';build=bounded(['/bin/sh',str(compiler),'-o',str(binary),*SRC],source,env,14);result['build']=build
            assert build['rc']==0 and not build['timed_out'] and not build['stderr'],build
            result['private_sources']={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
            for name in args.cases or list(CASES):result['cases'].append(run_case(name,CASES[name],binary,root,env,deadline))
            assert result['private_sources']=={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
            result['passed']=all(case['passed'] for case in result['cases'])
        except Exception as error:result.update(passed=False,error=repr(error),traceback=traceback.format_exc())
        finally:
            result['after']=hashes();result['frozen']=result['before']==result['after']
            if not result['frozen']:result['passed']=False;result['freeze_error']='production input changed during run'
            pathlib.Path(args.receipt).write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print(('PASS' if result.get('passed') else 'FAIL')+' agent multi-action',flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
