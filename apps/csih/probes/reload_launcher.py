#!/usr/bin/env python3
"""Private stable-CTTY launcher matrix; selector success or rollback, no network/mail."""
import hashlib, importlib.util, json, os, pathlib, pty, select, signal, struct, sys, tempfile, termios, time
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1]
sys.path.insert(0,str(APP))
spec=importlib.util.spec_from_file_location('native_launcher',APP/'reload_launcher.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc'))+[ROOT/'unisacc.com',APP/'reload_launcher.py',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
def main():
    mode=sys.argv[1] if len(sys.argv)>1 else 'success';assert mode in ('success','rollback')
    result=dict(before=hashes(),mode=mode);pid=None
    try:
        with tempfile.TemporaryDirectory(prefix='csih-launcher-') as tmp:
            root=pathlib.Path(tmp).resolve();source=root/'source';source.mkdir(mode=0o700)
            for path in APP.rglob('*'):
                if path.suffix in ('.c','.h','.inc'):
                    assert not path.is_symlink();dest=source/path.relative_to(APP);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(path.read_bytes())
            candidates=root/'candidates';candidates.mkdir(mode=0o700);session=root/'session';session.mkdir(mode=0o700);home=root/'home';home.mkdir(mode=0o700)
            (home/'env.jsonl').write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
            original=(source/'tui.c').read_text();proof=root/'proof.jsonl';child_result=root/'child.json'
            pid,master=pty.fork()
            if pid==0:
                os.environ.update(HOME=str(home),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ENDPOINT='http://127.0.0.1:1/unreachable',CSIH_ROLE='write',CSIH_PEER='0:private-peer',CSIH_CWD=str(root))
                for key in ('OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','http_proxy','https_proxy','ALL_PROXY','all_proxy'):os.environ.pop(key,None)
                baseline=termios.tcgetattr(0)
                class TestLauncher(module.Launcher):
                    def wait(self,actor,op,identity=None):
                        msg=super().wait(actor,op,identity)
                        if op=='ACK' and actor.identity['handoff']=='initial':
                            data=dict(op='ACTIVATE\0ignored',**actor.identity)
                            actor.socket.send((json.dumps(data)+'\n').encode());super().wait(actor,'REJECT')
                        if op=='FROZEN':
                            snapshot=json.loads((session/('handoff-'+msg['handoff']+'.json')).read_text())
                            with proof.open('a') as f:f.write(json.dumps(snapshot,ensure_ascii=False)+'\n')
                        if mode=='rollback' and op=='ACK' and actor.identity['handoff']!='initial':raise TimeoutError('PRIVATE_PROBE_IGNORE_ACK')
                        return msg
                try:
                    launcher=TestLauncher(source,ROOT/'unisacc.com',candidates,session,'sess');rc=launcher.run()
                    child_result.write_text(json.dumps(dict(rc=rc,baseline_restored=termios.tcgetattr(0)==baseline,before=repr(baseline),captured=repr(launcher.baseline),after=repr(termios.tcgetattr(0)))))
                    os._exit(0 if rc==0 else 1)
                except Exception as error:child_result.write_text(json.dumps(dict(error=repr(error))));os._exit(1)
            import fcntl
            fcntl.ioctl(master,termios.TIOCSWINSZ,struct.pack('HHHH',35,120,0,0))
            output=b'';deadline=time.monotonic()+55
            def pump():
                nonlocal output
                assert time.monotonic()<deadline,'outer case exceeded55s'
                if select.select([master],[],[],.03)[0]:
                    try:output+=os.read(master,65536)
                    except OSError:pass
            def events():
                try:return [json.loads(line) for line in (session/'launcher-events.jsonl').read_text().splitlines()]
                except FileNotFoundError:return []
            def wait_event(name,count=1):
                while sum(e['event']==name for e in events())<count:pump()
                return [e for e in events() if e['event']==name][-1]
            def send(data):os.write(master,data.encode());end=time.monotonic()+.25
            def type_line(line):
                os.write(master,line.encode());end=time.monotonic()+.15
                while time.monotonic()<end:pump()
                os.write(master,b'\r');end=time.monotonic()+.2
                while time.monotonic()<end:pump()
            first=wait_event('activated');type_line('/goal 保留目标')
            (source/'tui.c').write_text(original+'\n/* private candidate version1 */\n')
            type_line('/reload-code');wait_event('build-started',2);os.write(master,'中文换版草稿'.encode())
            if mode=='success':
                second=wait_event('committed');assert second['hash']!=first['hash']
                os.write(master,b'\x03')
                (source/'tui.c').write_text(original+'\n/* private candidate version2 */\n')
                type_line('/reload-code');wait_event('build-started',3);os.write(master,'中文第二草稿'.encode())
                third=wait_event('committed',2);assert third['hash']!=second['hash']
                os.write(master,b'\x03');(source/'tui.c').write_text(original+'\nBAD_PRIVATE_C_SOURCE\n');type_line('/reload-code');wait_event('build-failed')
                os.write(master,'坏构建后仍可编辑'.encode());end=time.monotonic()+.4
                while time.monotonic()<end:pump()
                assert '坏构建后仍可编辑' in output.decode(errors='replace')
                os.write(master,b'\x03');type_line('/export-state inspect')
                saved=json.loads((session/'state-v2.json').read_text());assert saved['goal']=='保留目标' and '/goal 保留目标' in saved['history'] and '/reload-code' not in saved['history']
                assert saved['cwd']==str(root) and saved['role']=='write' and saved['peer']=='0:private-peer' and saved['journal']['path']==str(session/'journal.jsonl')
                result['export']=saved
            else:
                wait_event('resumed');log=events();exited=next(i for i,e in enumerate(log) if e['event']=='candidate-exited');resumed=next(i for i,e in enumerate(log) if e['event']=='resumed');assert exited<resumed and log[exited]['rc'] is not None
                assert '中文换版草稿' in output.decode(errors='replace')
                os.write(master,'-RESUMED_EDIT'.encode());end=time.monotonic()+.4
                while time.monotonic()<end:pump()
                assert '中文换版草稿-RESUMED_EDIT' in output.decode(errors='replace'),'old did not serve input after RESUME'
                fd=os.open(session/'owner.lock',os.O_RDWR)
                try:
                    try:fcntl.lockf(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);raise AssertionError('resumed old owner absent')
                    except BlockingIOError:pass
                finally:os.close(fd)
                os.write(master,b'\x03')
            type_line('/exit')
            while True:
                pump();done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result.update(child=json.loads(child_result.read_text()),events=events(),output=output.decode(errors='replace'),snapshots=[json.loads(line) for line in proof.read_text().splitlines()])
            assert result['child']['rc']==0 and result['child']['baseline_restored'],result['child']
            assert result['snapshots'][0]['input']=='中文换版草稿',result['snapshots'][0]['input']
            if mode=='success':assert result['snapshots'][-1]['input']=='中文第二草稿'
            result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error))
    finally:
        if pid:os.killpg(pid,signal.SIGKILL);os.waitpid(pid,0)
        result['after']=hashes();assert result['before']==result['after']
        pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/tmp/csih-launcher-'+mode+'-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS launcher '+mode if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
