#!/usr/bin/env python3
"""Trusted private broker with actual controlling TTY; not a production launcher."""
import hashlib, json, os, pathlib, pty, select, signal, socket, struct, subprocess, sys, tempfile, termios, time
APP=pathlib.Path(__file__).resolve().parents[1]; ROOT=APP.parents[1]
SRC="tui.c render.c term.c chat.c clock.c tools.c cols.cx home.cx file.c shell.c edit.c gate.c json.cx session.c agent.c plugin.c net.c reload_state.c reload_session_decode.c reload_session_encode.c reload_io.c reload_load.c reload_consume.c journal_checkpoint.c reload_owner.c csih_message.c csih_message_io.c context_index.c".split()
INCLUDES=["-include",str(APP/"csih_cols.h"),"-include",str(APP/"csih_home.h"),"-include",str(APP/"json.h")]
def hashes():
    paths=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
def bounded(argv,cwd,env):
    p=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:o,e=p.communicate(timeout=14)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid,signal.SIGKILL);o,e=p.communicate();raise AssertionError(('timeout',argv,o,e))
    return dict(rc=p.returncode,stdout=o.decode(errors='replace'),stderr=e.decode(errors='replace'))
def broker(binary,root,env):
    baseline=termios.tcgetattr(0); actors=[]; events=[]
    def spawn(folder,standby=False):
        a,b=socket.socketpair();args=['standby-managed',str(folder),'sess','next','b'*64] if standby else ['agent-managed',str(folder),'sess','a'*64]
        p=subprocess.Popen([str(binary),*args,str(b.fileno())],pass_fds=(b.fileno(),),env=env)
        b.close();actors.append((p,a));return p,a
    def recv(sock,expected):
        data=b'';deadline=time.monotonic()+5
        while not data.endswith(b'\n'):
            assert time.monotonic()<deadline,('waiting',expected,data)
            if select.select([sock],[],[],.05)[0]:
                c=sock.recv(1);assert c,('EOF',expected);data+=c
        msg=json.loads(data);events.append(msg);assert msg['op']==expected,msg;return msg
    def send(sock,op,handoff='initial',hash='a'*64,session='sess'):
        sock.sendall((json.dumps(dict(op=op,session=session,handoff=handoff,hash=hash))+'\n').encode())
    def stop(p,s):
        s.close();p.wait(timeout=3)
    try:
        # Both native actors inherit this broker's controlling TTY and foreground group.
        assert os.tcgetpgrp(0)==os.getpgrp()
        for rollback in (False,True):
            folder=root/('rollback' if rollback else 'commit');folder.mkdir(mode=0o700)
            old,oc=spawn(folder);recv(oc,'ACK');send(oc,'ACTIVATE');time.sleep(.15)
            new,nc=spawn(folder,True);recv(nc,'READY')
            # standby has not acquired the lock, journal or terminal mode.
            import fcntl
            fd=os.open(folder/'owner.lock',os.O_RDWR)
            try:
                try:fcntl.lockf(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);raise AssertionError('old owner absent')
                except BlockingIOError:pass
            finally:os.close(fd)
            os.write(1,b'')
            # Broker cannot write to its own stdin as input; parent master injects on event.
            (root/'inject').write_text('rollback' if rollback else 'commit')
            time.sleep(.45)
            send(oc,'FREEZE','next','b'*64,session='wrong');recv(oc,'REJECT')
            send(oc,'FREEZE','next','b'*64);recv(oc,'FROZEN')
            snapshot=json.loads((folder/'handoff-next.json').read_text())
            assert snapshot['input']=='中文未发稿',snapshot['input']
            send(oc,'RELEASE','next','b'*64);recv(oc,'RELEASED')
            send(nc,'COMMIT','next','b'*64);recv(nc,'ACK')
            assert not (folder/'handoff-next.json').is_file()
            if rollback:
                # ACK deliberately not accepted: terminate and wait before RESUME.
                new.terminate();new.wait(timeout=3);nc.close()
                send(oc,'RESUME','next','b'*64);recv(oc,'RESUMED')
                send(oc,'FREEZE','check','c'*64);recv(oc,'FROZEN')
                saved=json.loads((folder/'handoff-check.json').read_text());assert saved['input']=='中文未发稿'
                send(oc,'RESUME','check','c'*64);recv(oc,'RESUMED');stop(old,oc)
            else:
                send(nc,'ACTIVATE','next','b'*64);send(oc,'RETIRE','next','b'*64);old.wait(timeout=3);oc.close()
                time.sleep(.15)
                # New actor has sole owner; retiring old did not restore cooked mode.
                assert not (termios.tcgetattr(0)[3]&termios.ICANON)
                fd=os.open(folder/'owner.lock',os.O_RDWR)
                try:
                    try:fcntl.lockf(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);raise AssertionError('new owner absent')
                    except BlockingIOError:pass
                finally:os.close(fd)
                send(nc,'FREEZE','check','c'*64);recv(nc,'FROZEN')
                saved=json.loads((folder/'handoff-check.json').read_text());assert saved['input']=='中文未发稿'
                send(nc,'RESUME','check','c'*64);recv(nc,'RESUMED');stop(new,nc)
        return dict(passed=True,events=events)
    except Exception as error:return dict(passed=False,error=repr(error),events=events)
    finally:
        for p,s in actors:
            s.close()
            if p.poll() is None:p.terminate()
            try:p.wait(timeout=2)
            except subprocess.TimeoutExpired:p.kill();p.wait()
        termios.tcsetattr(0,termios.TCSANOW,baseline)
def main():
    result=dict(before=hashes());pid=None
    try:
        with tempfile.TemporaryDirectory(prefix='csih-managed-') as tmp:
            root=pathlib.Path(tmp).resolve();home=root/'home';home.mkdir(mode=0o700);(home/'env.jsonl').write_text('{"DEEPSEEK_API_KEY":"LOCAL_STUB_ONLY"}\n')
            env=dict(os.environ,HOME=str(home),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ENDPOINT='http://127.0.0.1:1/unreachable',CSIH_ROLE='write',CSIH_PEER='',CSIH_CWD=str(root))
            for key in ('OPENAI_API_KEY','HTTP_PROXY','HTTPS_PROXY','http_proxy','https_proxy','ALL_PROXY','all_proxy'):env.pop(key,None)
            binary=root/'candidate';result['build']=bounded(['/bin/sh',str(ROOT/'unisacc.com'),*INCLUDES,'-o',str(binary),*SRC],APP,env);assert result['build']['rc']==0,result['build']
            gateenv=dict(env,CSIH_ROLE='',CSIH_PEER='');result['selftest']=bounded([str(binary),'selftest'],APP,gateenv);assert result['selftest']['rc']==0,result['selftest']
            pid,master=pty.fork()
            if pid==0:
                r=broker(binary,root,env);(root/'broker.json').write_text(json.dumps(r));os._exit(0 if r['passed'] else 1)
            import fcntl
            fcntl.ioctl(master,termios.TIOCSWINSZ,struct.pack('HHHH',35,120,0,0))
            output=b'';sent=set();deadline=time.monotonic()+14
            while True:
                assert time.monotonic()<deadline,'broker exceeded14s'
                try:
                    kind=(root/'inject').read_text()
                    if kind not in sent:os.write(master,'中文未发稿'.encode());sent.add(kind)
                except FileNotFoundError:pass
                if select.select([master],[],[],.02)[0]:
                    try:chunk=os.read(master,65536)
                    except OSError:chunk=b''
                    output+=chunk
                done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result['broker']=json.loads((root/'broker.json').read_text());result['output']=output.decode(errors='replace')
            assert result['broker']['passed'],result['broker']
            assert 'b'*64 in result['output'] and '中文未发稿' in result['output'],'new UI identity/draft absent'
            result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error))
    finally:
        if pid:
            os.killpg(pid,signal.SIGKILL);os.waitpid(pid,0)
        result['after']=hashes();assert result['before']==result['after']
        pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '/tmp/csih-managed-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS managed CTTY commit and rollback' if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
