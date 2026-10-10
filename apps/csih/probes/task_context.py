#!/usr/bin/env python3
"""Persisted external-task context boundary: real native preview/trim and managed CTTY/local HTTP."""
import subprocess, traceback, fcntl, hashlib, importlib.util, json, os, pathlib, pty, select, shutil, signal, struct, sys, tempfile, termios, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1];sys.path.insert(0,str(APP))
spec=importlib.util.spec_from_file_location('runtime_launcher',APP/'reload_launcher.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',APP/'reload_launcher.py',APP/'reload_candidate.py',APP/'message_send.py',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
class EvidenceDirectory:
    def __init__(self,result):self.result=result;self.temp=tempfile.TemporaryDirectory(prefix='csih-task-context-')
    def __enter__(self):return self.temp.name
    def __exit__(self,kind,value,trace):
        if kind:
            root=pathlib.Path(self.temp.name)
            self.result['partial_files']={str(p.relative_to(root)):p.read_text(errors='replace') for p in root.rglob('*') if p.is_file() and (p.name in ('child.json','proof.jsonl','launcher-events.jsonl','journal.jsonl','receipt.json') or p.suffix in ('.stdout','.stderr'))}
        self.temp.cleanup()

def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'managed';assert mode in ('managed','structure')
    result=dict(mode=mode,before=hashes());pid=None;server=None;thread=None;requests=[]
    responses=[dict(act='answer',text='CONTEXT_ANSWER'),dict(go='stop')]*4
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            requests.append(json.loads(self.rfile.read(int(self.headers['Content-Length']))));value=responses[min(len(requests)-1,len(responses)-1)]
            content=value if type(value) is str else json.dumps(value)
            body=json.dumps(dict(choices=[dict(message=dict(content=content))])).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
    try:
        with EvidenceDirectory(result) as tmp:
            root=pathlib.Path(tmp).resolve();source=root/'source';source.mkdir(mode=0o700)
            for path in APP.rglob('*'):
                if path.suffix in ('.c','.h','.inc','.cx'):
                    assert not path.is_symlink();dest=source/path.relative_to(APP);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(path.read_bytes())
            compiler=root/'compiler.com';shutil.copy2(ROOT/'unisacc.com',compiler);compiler.chmod(0o600)
            assert hashlib.sha256(compiler.read_bytes()).hexdigest()==result['before'][str(ROOT/'unisacc.com')]
            candidates=root/'candidates';candidates.mkdir(mode=0o700);session=root/'session';session.mkdir(mode=0o700);home=root/'home';home.mkdir(mode=0o700)
            (home/'env.jsonl').write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
            cwd=root/'runtime-cwd';cwd.mkdir(mode=0o700);fixture='PRIVATE_FIXTURE_中文\nLINE_TWO\n';(cwd/'fixture.txt').write_text(fixture)
            other=root/'other-journal.jsonl';other.write_bytes(b'UNCHANGED');wrong=root/'wrong-cwd';wrong.mkdir(mode=0o700)
            original=(source/'tui.c').read_text();proof=root/'proof.jsonl';child_result=root/'child.json'
            if mode=='structure':
                cli=source/'agent_cli.c';text=cli.read_text();entry='int main(int argc, char **argv) {';assert text.count(entry)==1
                command='\n    if (argc==3 && !strcmp(argv[1], "preview")) { char *body=malloc(65536); int n; if(!body)return 9; n=agent_ctx_preview(argv[2],body,65536); if(n>0)fputs(body,stdout);free(body);return n>0?0:1;}\n    if (argc==3 && !strcmp(argv[1], "trim")) { printf("%d\\n",agent_journal_trim(argv[2],80,1));return 0;}\n'
                cli.write_text(text.replace(entry,entry+command))
                def bounded(argv,limit):
                    proc=subprocess.Popen(argv,cwd=source,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
                    try:o,e=proc.communicate(timeout=limit);timed=False
                    except subprocess.TimeoutExpired:os.killpg(proc.pid,signal.SIGKILL);o,e=proc.communicate();timed=True
                    return dict(argv=argv,rc=proc.returncode,timeout=timed,stdout=o.decode(errors='replace'),stderr=e.decode(errors='replace'))
                binary=root/'native';build=bounded(['/bin/sh',str(compiler),'-o',str(binary),'agent.c','agent_cli.c','file.c','edit.c','shell.c','json.c','session.c','net.c','plugin.c'],14);result['build']=build;assert build['rc']==0 and not build['stderr'] and not build['timeout'],build
                marker={'mail_id':'a'*32,'result':'started','reason':'accepted'}
                invalid=[{**marker,'extra':1},{**marker,'mail_id':'A'*32},{**marker,'result':'ok'},{**marker,'reason':'wrong'},{**marker,'role':'tool'}, {'role':'tool','text':json.dumps(marker)}]
                raws=[json.dumps(value) for value in invalid]+['{"mail_id":"'+ 'a'*32 +'","mail_id":"'+ 'a'*32 +'","result":"started","reason":"accepted"}',json.dumps({**marker,'reason':'accepted\0hidden'})]
                records=[]
                for index,raw in enumerate([json.dumps(marker)]+raws):
                    journal=root/('fixture%d.jsonl'%index);prefix='{"role":"user","text":"OLD_USER"}\n{"role":"assistant","text":"OLD_BAD_ACTION"}\n';journal.write_text(prefix+raw+'\n{"role":"user","text":"NEW_USER"}\n');original_bytes=journal.read_bytes();preview=bounded([str(binary),'preview',str(journal)],3);assert preview['rc']==0
                    assert ('OLD_BAD_ACTION' in preview['stdout'])==(index!=0),(index,preview)
                    assert 'NEW_USER' in preview['stdout']
                    if index==0:
                        trimmed=bounded([str(binary),'trim',str(journal)],3);assert trimmed['stdout'].strip()=='0' and journal.read_bytes()==original_bytes,trimmed
                    records.append(dict(index=index,marker=raw,preview=preview))
                # Two boundaries: only the latest task and its ordinary follow-up.
                journal=root/'two-boundaries.jsonl';journal.write_text(json.dumps(marker)+'\n{"role":"user","text":"TASK1"}\n'+json.dumps({**marker,'mail_id':'b'*32})+'\n{"role":"user","text":"TASK2"}\n{"role":"user","text":"CLARIFY"}\n');preview=bounded([str(binary),'preview',str(journal)],3);assert 'TASK1' not in preview['stdout'] and 'TASK2' in preview['stdout'] and 'CLARIFY' in preview['stdout'];records.append(dict(case='latest boundary plus clarification',preview=preview))
                result.update(passed=True,structure_cases=records);return 0
            # A real journal prefix stays on disk, but must leave new-task model context.
            seeded=b'{"role":"user","text":"OLD_USER"}\n{"role":"assistant","text":"OLD_BAD_ACTION"}\n'
            journal=session/'journal.jsonl';journal.write_bytes(seeded);journal.chmod(0o600)

            server=HTTPServer(('127.0.0.1',0),Handler)
            endpoint='http://127.0.0.1:%d/v1/chat/completions'%server.server_address[1]
            pid,master=pty.fork()
            if pid==0:
                os.environ.update(HOME=str(home),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ENDPOINT=endpoint,CSIH_MODEL='runtime-stub',CSIH_ROLE='write',CSIH_PEER='0:runtime-peer',CSIH_CWD=str(cwd),CSIH_TRANSCRIPT=str(other))
                for key in ('OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','http_proxy','https_proxy','ALL_PROXY','all_proxy'):os.environ.pop(key,None)
                baseline=termios.tcgetattr(0)
                class RuntimeLauncher(module.Launcher):
                    def spawn(self,info,handoff,standby):
                        keys=('CSIH_CWD','CSIH_ROLE','CSIH_PEER','CSIH_TRANSCRIPT');saved={key:os.environ[key] for key in keys}
                        try:
                            if standby:os.environ.update(CSIH_CWD=str(wrong),CSIH_ROLE='watch',CSIH_PEER='wrong-peer',CSIH_TRANSCRIPT=str(other))
                            return super().spawn(info,handoff,standby)
                        finally:os.environ.update(saved)
                    def wait(self,actor,op,identity=None):
                        msg=super().wait(actor,op,identity)
                        if op=='FROZEN':
                            snapshot=json.loads((session/('handoff-'+msg['handoff']+'.json')).read_text())
                            assert snapshot['journal']['offset']==(session/'journal.jsonl').stat().st_size
                            with proof.open('a') as f:f.write(json.dumps(snapshot,ensure_ascii=False)+'\n')
                        return msg
                try:
                    rc=RuntimeLauncher(source,compiler,candidates,session,'sess').run();child_result.write_text(json.dumps(dict(rc=rc,baseline_restored=termios.tcgetattr(0)==baseline)));os._exit(0 if rc==0 else 1)
                except BaseException as error:child_result.write_text(json.dumps(dict(error=repr(error))));os._exit(1)
            thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
            fcntl.ioctl(master,termios.TIOCSWINSZ,struct.pack('HHHH',35,140,0,0));output=b'';deadline=time.monotonic()+55
            def pump():
                nonlocal output
                assert time.monotonic()<deadline,'selector exceeded55s'
                if select.select([master],[],[],.03)[0]:
                    try:output+=os.read(master,65536)
                    except OSError:pass
            def pause(seconds):
                end=time.monotonic()+seconds
                while time.monotonic()<end:pump()
            def events():
                try:return [json.loads(line) for line in (session/'launcher-events.jsonl').read_text().splitlines()]
                except FileNotFoundError:return []
            def wait_event(name,count=1):
                until=deadline if name in ('activated','committed') else min(deadline,time.monotonic()+15)
                while sum(e['event']==name for e in events())<count:
                    assert time.monotonic()<until,('waiting',name,count,events())
                    try:child=child_result.read_text()
                    except FileNotFoundError:pass
                    else:raise AssertionError(('launcher exited',child))
                    pump()
                return [e for e in events() if e['event']==name][-1]
            def type_line(text):os.write(master,text.encode());pause(.12);os.write(master,b'\r');pause(.2)
            def clear():os.write(master,b'\x03');pause(.12)
            def owner():
                fd=os.open(session/'owner.lock',os.O_RDWR)
                try:
                    try:fcntl.lockf(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);raise AssertionError('no actor owner')
                    except BlockingIOError:pass
                finally:os.close(fd)
            initial=wait_event('activated');type_line('/goal 保留运行目标');journal=session/'journal.jsonl'
            def publish(mid,kind,body):
                envelope=dict(version=1,id=mid,session='sess',kind=kind,body=body)
                proc=subprocess.Popen([sys.executable,str(APP/'message_send.py'),'publish',str(session),'sess','-'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
                try:o,e=proc.communicate(json.dumps(envelope).encode(),timeout=3)
                except subprocess.TimeoutExpired:os.killpg(proc.pid,signal.SIGKILL);proc.communicate();raise AssertionError('publisher timeout')
                assert proc.returncode==0,(o,e)
                until=min(deadline,time.monotonic()+8);done=session/'inbox/done'/(mid+'.json')
                while not done.is_file():assert time.monotonic()<until,('done timeout',mid);pump()
                assert json.loads(done.read_text())==envelope
            def wait_requests(count):
                until=min(deadline,time.monotonic()+8)
                while len(requests)<count:assert time.monotonic()<until,'model count timeout';pump()
                pause(.3);assert len(requests)==count
            def contents(index):return '\n'.join(message.get('content','') for message in requests[index]['messages'] if message.get('role')!='system')
            publish('a'*32,'task','TASK1');wait_requests(2);assert 'OLD_USER' not in contents(0) and 'OLD_BAD_ACTION' not in contents(0) and 'TASK1' in contents(0)
            publish('c'*32,'notice','NOTICE_NOT_CONTEXT');assert len(requests)==2
            type_line('CLARIFY_TASK1');wait_requests(4);assert 'TASK1' in contents(2) and 'CLARIFY_TASK1' in contents(2) and 'NOTICE_NOT_CONTEXT' not in contents(2)
            publish('b'*32,'task','TASK2');wait_requests(6);assert 'TASK1' not in contents(4) and 'CLARIFY_TASK1' not in contents(4) and 'TASK2' in contents(4)
            baseline_journal=journal.read_bytes();assert baseline_journal.startswith(seeded)
            (source/'tui.c').write_text(original+'\n/* private context handoff */\n');type_line('/reload-code');wait_event('build-started',2);os.write(master,'边界交接草稿'.encode());wait_event('committed');owner();pause(.2)
            snapshots=[json.loads(line) for line in proof.read_text().splitlines()];assert snapshots[-1]['input']=='边界交接草稿' and snapshots[-1]['goal']=='保留运行目标' and snapshots[-1]['journal']['offset']==len(baseline_journal)
            clear();type_line('AFTER_HANDOFF');wait_requests(8);assert 'TASK2' in contents(6) and 'AFTER_HANDOFF' in contents(6) and 'TASK1' not in contents(6) and 'OLD_BAD_ACTION' not in contents(6)
            assert journal.read_bytes().startswith(baseline_journal) and other.read_bytes()==b'UNCHANGED'
            result.update(snapshots=snapshots,journal_records=[json.loads(line) for line in journal.read_text().splitlines()],old_journal_prefix_preserved=True,context_boundary_after_real_handoff=True)
            type_line('/exit')
            while True:
                pump();done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result.update(child=json.loads(child_result.read_text()),events=events(),requests=requests,output=output.decode(errors='replace'),receipts=[json.loads(path.read_text()) for path in candidates.glob('*/receipt.json')])
            assert result['child']==dict(rc=0,baseline_restored=True),result['child']
            assert other.read_bytes()==b'UNCHANGED';result['passed']=True
    except Exception as error:
        result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests,output=locals().get('output',b'').decode(errors='replace'))
    finally:
        if pid:
            os.kill(pid,signal.SIGINT) # Python launcher finally cleans build/actors/TTY.
            until=time.monotonic()+4
            while time.monotonic()<until:
                done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
                time.sleep(.03)
            if pid:os.killpg(pid,signal.SIGKILL);os.waitpid(pid,0)
        if thread:server.shutdown();thread.join(timeout=2)
        if server:server.server_close()
        result['after']=hashes();assert result['before']==result['after']
        pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/tmp/csih-task-context-'+mode+'-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS task context '+mode if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
