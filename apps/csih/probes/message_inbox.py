#!/usr/bin/env python3
"""Real mailbox notice/task integration. Run only after native mailbox hooks freeze."""
import traceback, subprocess, fcntl, hashlib, importlib.util, json, os, pathlib, pty, select, shutil, signal, struct, sys, tempfile, termios, threading, time
from http.server import BaseHTTPRequestHandler, HTTPServer
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1];sys.path.insert(0,str(APP))
spec=importlib.util.spec_from_file_location('runtime_launcher',APP/'reload_launcher.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc'))+[ROOT/'unisacc.com',APP/'reload_launcher.py',APP/'reload_candidate.py',APP/'message_send.py',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
class EvidenceDirectory:
    def __init__(self,result):self.result=result;self.temp=tempfile.TemporaryDirectory(prefix='csih-message-inbox-')
    def __enter__(self):return self.temp.name
    def __exit__(self,kind,value,trace):
        if kind:
            root=pathlib.Path(self.temp.name)
            self.result['partial_files']={str(p.relative_to(root)):p.read_text(errors='replace') for p in root.rglob('*') if p.is_file() and (p.name in ('child.json','proof.jsonl','launcher-events.jsonl','journal.jsonl','receipt.json') or p.suffix in ('.stdout','.stderr') or (p.suffix=='.json' and ('inbox' in p.parts or p.name=='state-v2.json' or p.name.startswith('handoff-'))))}
        self.temp.cleanup()

def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'notice';assert mode in ('notice','task')
    result=dict(mode=mode,before=hashes());pid=None;server=None;thread=None;requests=[]
    responses=['UNPARSEABLE']*3+[dict(act='exec',cmd='pwd',why='actual mailbox cwd'),dict(act='file',op='read',path='fixture.txt',line=1,n=2),dict(act='answer',text='MAIL_TASK_DONE'),dict(go='stop')]
    task_wait=threading.Event();task_release=threading.Event()
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_POST(self):
            requests.append(json.loads(self.rfile.read(int(self.headers['Content-Length']))))
            if mode=='notice':raise AssertionError('notice issued a model request')
            if len(requests)==4:
                task_wait.set();assert task_release.wait(5),'probe task pause exceeded5s'
            value=responses[min(len(requests)-1,len(responses)-1)]
            content=value if type(value) is str else json.dumps(value)
            body=json.dumps(dict(choices=[dict(message=dict(content=content))])).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
    try:
        with EvidenceDirectory(result) as tmp:
            root=pathlib.Path(tmp).resolve();source=root/'source';source.mkdir(mode=0o700)
            for path in APP.rglob('*'):
                if path.suffix in ('.c','.h','.inc'):
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
            journal=session/'journal.jsonl'
            def publish(mid,kind,body):
                envelope=dict(version=1,id=mid,session='sess',kind=kind,body=body)
                raw=json.dumps(envelope,ensure_ascii=False).encode()
                proc=subprocess.Popen([sys.executable,str(APP/'message_send.py'),'publish',str(session),'sess','-'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
                try:stdout,stderr=proc.communicate(raw,timeout=4)
                except subprocess.TimeoutExpired:
                    os.killpg(proc.pid,signal.SIGKILL);stdout,stderr=proc.communicate();raise AssertionError('publisher exceeded4s')
                assert proc.returncode==0,(stdout,stderr)
                receipt=json.loads(stdout);assert set(receipt)=={'kind','id','session','status'} and 'body' not in receipt
                result.setdefault('publish_receipts',[]).append(receipt);return envelope
            def wait_done(mid):
                until=min(deadline,time.monotonic()+10)
                target=session/'inbox/done'/(mid+'.json')
                while not target.is_file():
                    assert time.monotonic()<until,('waiting done',mid);pump()
                return json.loads(target.read_text())
            def save_draft_browser():
                # Native Up/Down saves the genuine current editor into history_draft.
                os.write(master,b'\x1b[A');pause(.12);os.write(master,b'\x1b[B');pause(.12)
            def checkpoint_handoff(comment,draft):
                # Control input is deliberately cleared; history_draft retains
                # the actual prior editor. Input typed during build is separately
                # labelled, never presented as preservation of cleared input.
                clear();(source/'tui.c').write_text(original+'\n/* '+comment+' */\n')
                type_line('/reload-code');wait_event('build-started',2);os.write(master,draft.encode())
            notice_id='b'*32;task_id='a'*32;notice_body='NOTICE_中文完整正文'
            if mode=='notice':
                type_line('/export-state before');baseline=json.loads((session/'state-v2.json').read_text())
                initial_journal=journal.read_bytes()
                checkpoint_handoff('private mailbox notice version','中文未发送草稿')
                save_draft_browser();expected=publish(notice_id,'notice',notice_body)
                assert wait_done(notice_id)==expected
                first=wait_event('committed');owner();pause(.3)
                snapshots=[json.loads(line) for line in proof.read_text().splitlines()];saved=snapshots[-1]
                assert saved['input']=='中文未发送草稿' and saved['history_draft']=='中文未发送草稿'
                for key in ('history','pending_queue','goal','loop_on','loop_left'):
                    assert saved[key]==baseline[key],(key,saved,baseline)
                assert saved['history_pos']==len(saved['history']) and not saved['history_browsing']
                assert requests==[] and journal.read_bytes()==initial_journal
                assert notice_body in output.decode(errors='replace'),'no visible notice marker/body'
                assert first['hash']!=initial['hash']
                result.update(baseline_snapshot=baseline,snapshot=saved,journal_before_hex=initial_journal.hex(),journal_after_hex=journal.read_bytes().hex(),model_POST_increment=0)
                clear()
            else:
                type_line('/loop on')
                until=min(deadline,time.monotonic()+10)
                while len(requests)<3 or 'unfinished: too many unparseable steps' not in output.decode(errors='replace'):
                    assert time.monotonic()<until,'automatic failure deadline';pump()
                pause(1.1);assert len(requests)==3
                type_line('/export-state failure');baseline=json.loads((session/'state-v2.json').read_text())
                assert baseline['loop_on'] is True and baseline['loop_left']==7 and baseline['pending_queue']==[]
                task_envelope=publish(task_id,'task','EXTERNAL_TASK_中文')
                until=min(deadline,time.monotonic()+5)
                while not task_wait.is_set():assert time.monotonic()<until,'task did not dispatch';pump()
                assert (session/'inbox/started'/(task_id+'.json')).is_file()
                os.write(master,'忙时中文草稿'.encode());pause(.15);save_draft_browser()
                journal_before_notice=journal.read_bytes();notice_envelope=publish(notice_id,'notice',notice_body)
                assert wait_done(notice_id)==notice_envelope
                until=min(deadline,time.monotonic()+1)
                while notice_body not in output.decode(errors='replace') and time.monotonic()<until:pump()
                result['busy_notice_checks']=dict(request_count=len(requests),journal_unchanged=journal.read_bytes()==journal_before_notice,visible=notice_body in output.decode(errors='replace'))
                assert len(requests)==4 and journal.read_bytes()==journal_before_notice,result['busy_notice_checks']
                assert notice_body in output.decode(errors='replace'),result['busy_notice_checks']
                task_release.set()
                assert wait_done(task_id)==task_envelope
                pause(1.1);assert len(requests)==7,'task caused additional goal dispatch'
                # A new suffix proves the preserved busy-time draft is still
                # the active editor after the turn, rather than an old frame.
                tail='恢复后唯一后缀';os.write(master,tail.encode());pause(.2)
                assert '忙时中文草稿'+tail in output.decode(errors='replace')
                save_draft_browser()
                records=[json.loads(line) for line in journal.read_text().splitlines()]
                all_tool=[r for r in records if r.get('role')=='tool']
                parse_errors=[r for r in all_tool if r.get('name')=='error'];assert len(parse_errors)==3,parse_errors
                tool=[r for r in all_tool if r.get('name')!='error'];assert len(tool)==2,tool
                assert not any(notice_body in message.get('content','') for request in requests for message in request.get('messages',[]))
                assert any(r.get('text')=='cwd='+str(cwd)+'\nexit=0\n'+str(cwd)+'\n' for r in tool),tool
                assert any(r.get('text')==fixture+'第 1-2 行，共 2 行。' for r in tool),tool
                system='\n'.join(m.get('content','') for m in requests[3]['messages'] if m.get('role')=='system')
                assert '写手。同伴 0:runtime-peer' in system
                assert any(m.get('content')=='EXTERNAL_TASK_中文' for m in requests[3]['messages'] if m.get('role')=='user')
                metadata=[r for r in records if r.get('mail_id')==task_id]
                assert len(metadata)==2 and all(set(record)=={'mail_id','result','reason'} for record in metadata) and [record['result'] for record in metadata]==['started','ok'],metadata
                assert not any(r.get('mail_id')==notice_id or notice_body in r.get('text','') for r in records)
                assert publish(task_id,'task','EXTERNAL_TASK_中文')==task_envelope
                assert result['publish_receipts'][-1]['status']=='already-published'
                pause(.4);assert len(requests)==7
                actual_journal=journal.read_bytes();owner()
                checkpoint_handoff('private mailbox task version','交接时单独输入')
                wait_event('committed');owner();pause(.25)
                saved=json.loads(proof.read_text().splitlines()[-1])
                assert saved['history_draft']=='忙时中文草稿'+tail
                assert saved['input']=='交接时单独输入' # explicitly reentered control-phase input
                assert saved['goal']==baseline['goal'] and saved['loop_on'] is True and saved['loop_left']==7 and saved['pending_queue']==baseline['pending_queue']
                assert saved['history']==baseline['history'],'external task entered user history'
                assert saved['journal']['offset']==len(actual_journal) and journal.read_bytes()==actual_journal
                assert len(requests)==7 and other.read_bytes()==b'UNCHANGED'
                result.update(baseline_snapshot=baseline,snapshot=saved,tool_records=tool,mail_metadata=metadata,system=system,journal_records=records,model_POST_count=7,observed_idle_seconds=1.1,live_editor='忙时中文草稿'+tail)
                clear()
            type_line('/exit')
            while True:
                pump();done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result.update(child=json.loads(child_result.read_text()),events=events(),requests=requests,output=output.decode(errors='replace'),receipts=[json.loads(path.read_text()) for path in candidates.glob('*/receipt.json')])
            assert result['child']==dict(rc=0,baseline_restored=True),result['child']
            for committed in [e for e in result['events'] if e['event']=='committed']:
                received=[e['op'] for e in result['events'] if e['event']=='received' and e.get('handoff')==committed['handoff'] and e.get('hash')==committed['hash']]
                assert received==['READY','FROZEN','RELEASED','ACK'],received
                assert any(e['event']=='activated' and e.get('pid')==committed['pid'] and e.get('hash')==committed['hash'] for e in result['events'])
            assert not any(e['event'] in ('handoff-failed','retire-forced','BLOCKED') for e in result['events'])
            assert other.read_bytes()==b'UNCHANGED';result['passed']=True
    except Exception as error:
        result.update(passed=False,error=repr(error),traceback=traceback.format_exc(),requests=requests,output=locals().get('output',b'').decode(errors='replace'))
        task_release.set()
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
        pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/tmp/csih-message-inbox-'+mode+'-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS message inbox '+mode if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
