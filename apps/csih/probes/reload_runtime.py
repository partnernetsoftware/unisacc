#!/usr/bin/env python3
"""Actual post-handoff tools and automatic-goal failure, private CTTY/local HTTP only."""
import fcntl, hashlib, importlib.util, json, os, pathlib, pty, select, shutil, signal, struct, sys, tempfile, termios, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1];sys.path.insert(0,str(APP))
spec=importlib.util.spec_from_file_location('runtime_launcher',APP/'reload_launcher.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',APP/'reload_launcher.py',APP/'reload_candidate.py',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
class EvidenceDirectory:
    def __init__(self,result):self.result=result;self.temp=tempfile.TemporaryDirectory(prefix='csih-reload-runtime-')
    def __enter__(self):return self.temp.name
    def __exit__(self,kind,value,trace):
        if kind:
            root=pathlib.Path(self.temp.name)
            self.result['partial_files']={str(p.relative_to(root)):p.read_text(errors='replace') for p in root.rglob('*') if p.is_file() and (p.name in ('child.json','proof.jsonl','launcher-events.jsonl','journal.jsonl','receipt.json') or p.suffix in ('.stdout','.stderr'))}
        self.temp.cleanup()

def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'tools';assert mode in ('tools','goal-failure')
    result=dict(mode=mode,before=hashes());pid=None;server=None;thread=None;requests=[]
    responses=[dict(act='exec',cmd='pwd',why='actual restored cwd'),dict(act='file',op='read',path='fixture.txt',line=1,n=2),dict(act='answer',outcome='completed',text='RUNTIME_DONE'),dict(go='stop')] if mode=='tools' else ['UNPARSEABLE']
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
            initial=wait_event('activated');type_line('/goal 保留运行目标')
            if mode=='tools':
                (source/'tui.c').write_text(original+'\n/* private runtime version1 */\n');type_line('/reload-code');wait_event('build-started',2);os.write(master,'中文第一草稿'.encode())
                first=wait_event('committed');owner();assert first['hash']!=initial['hash']
                pause(.25);assert '中文第一草稿' in output.decode(errors='replace')
                clear();type_line('RUN_RUNTIME')
                until=time.monotonic()+10
                while len(requests)<4 or '本轮结束' not in output.decode(errors='replace'):
                    assert time.monotonic()<until,'runtime turn deadline';pump()
                pause(.4);assert len(requests)==4
                journal=session/'journal.jsonl';records=[json.loads(line) for line in journal.read_text().splitlines()];tool=[r for r in records if r.get('role')=='tool']
                assert any(r.get('text')=='cwd='+str(cwd)+'\nexit=0\n'+str(cwd)+'\n' for r in tool),tool
                expected_file=fixture+'第 1-2 行，共 2 行。'
                assert any(r.get('text')==expected_file for r in tool),tool
                system='\n'.join(m.get('content','') for m in requests[0]['messages'] if m.get('role')=='system');assert '写手。同伴 0:runtime-peer' in system,system
                assert any(m.get('content')=='RUN_RUNTIME' for m in requests[0]['messages'] if m.get('role')=='user')
                actual_journal=journal.read_bytes();assert len(actual_journal)>0 and other.read_bytes()==b'UNCHANGED'
                (source/'tui.c').write_text(original+'\n/* private runtime version2 */\n');type_line('/reload-code');wait_event('build-started',3);os.write(master,'中文日志后草稿'.encode())
                second=wait_event('committed',2);owner();assert second['hash']!=first['hash']
                pause(.25);assert '中文日志后草稿' in output.decode(errors='replace') and len(requests)==4 and journal.read_bytes()==actual_journal
                snapshots=[json.loads(line) for line in proof.read_text().splitlines()]
                assert snapshots[0]['input']=='中文第一草稿' and snapshots[0]['journal']['offset']==0
                assert snapshots[1]['input']=='中文日志后草稿' and snapshots[1]['journal']['offset']==len(actual_journal)
                assert 'RUN_RUNTIME' in snapshots[1]['history'] and '/goal 保留运行目标' in snapshots[1]['history'] and snapshots[1]['goal']=='保留运行目标'
                result.update(tool_records=tool,snapshots=snapshots,journal_size=len(actual_journal),system=system)
                clear()
            else:
                type_line('/loop on') # actual original automatic goal, no manual prompt
                until=time.monotonic()+10
                while len(requests)<3 or 'unfinished: too many unparseable steps' not in output.decode(errors='replace'):
                    assert time.monotonic()<until,'goal failure deadline';pump()
                pause(1.3);assert len(requests)==3,'failed automatic goal was redispatched';owner()
                type_line('/export-state failure')
                saved=json.loads((session/'state-v2.json').read_text());assert saved['loop_on'] is True and saved['loop_left']==7 and saved['pending_queue']==[] and saved['goal']=='保留运行目标',saved
                records=[json.loads(line) for line in (session/'journal.jsonl').read_text().splitlines()]
                users=[r for r in records if r.get('role')=='user'];assert len(users)==1 and users[0]['text'].startswith('目标：保留运行目标'),users
                result.update(snapshot=saved,observed_idle_seconds=1.3,journal_records=records)
            type_line('/exit')
            while True:
                pump();done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result.update(child=json.loads(child_result.read_text()),events=events(),requests=requests,output=output.decode(errors='replace'),receipts=[json.loads(path.read_text()) for path in candidates.glob('*/receipt.json')])
            assert result['child']==dict(rc=0,baseline_restored=True),result['child']
            assert other.read_bytes()==b'UNCHANGED';result['passed']=True
    except Exception as error:
        result.update(passed=False,error=repr(error),requests=requests,output=locals().get('output',b'').decode(errors='replace'))
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
        pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/tmp/csih-reload-runtime-'+mode+'-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS runtime '+mode if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
