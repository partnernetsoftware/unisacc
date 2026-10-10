#!/usr/bin/env python3
"""Focused real-process host flock and live CTTY second-host isolation."""
import fcntl, hashlib, importlib.util, json, os, pathlib, pty, select, signal, stat, struct, subprocess, sys, tempfile, termios, time
APP=pathlib.Path(__file__).resolve().parents[1];ROOT=APP.parents[1];sys.path.insert(0,str(APP))
spec=importlib.util.spec_from_file_location('host_launcher',APP/'reload_launcher.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def hashes():
    files=sorted(p for p in APP.rglob('*') if p.suffix in ('.c','.h','.inc','.cx'))+[ROOT/'unisacc.com',APP/'reload_launcher.py',pathlib.Path(__file__)]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
def command(argv,timeout=3,env=None):
    p=subprocess.Popen(argv,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    try:o,e=p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);o,e=p.communicate();raise AssertionError(('timeout',argv,o,e))
    return dict(rc=p.returncode,stdout=o.decode(errors='replace'),stderr=e.decode(errors='replace'))
def helper(directory):
    code="import sys,os;sys.path.insert(0,sys.argv[1]);import reload_launcher as m;fd=m.acquire_supervisor_lock(sys.argv[2]);print('LOCKED',flush=True);sys.stdin.read();os.close(fd)"
    return subprocess.Popen([sys.executable,'-c',code,str(APP),str(directory)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
def attempt(directory):
    code="import sys,os;sys.path.insert(0,sys.argv[1]);import reload_launcher as m;fd=m.acquire_supervisor_lock(sys.argv[2]);assert not os.get_inheritable(fd);os.close(fd);print('LOCKED')"
    return command([sys.executable,'-c',code,str(APP),str(directory)])
def main():
    result=dict(before=hashes(),cases={});pid=None;holder=None
    try:
        with tempfile.TemporaryDirectory(prefix='csih-supervisor-lock-') as tmp:
            root=pathlib.Path(tmp).resolve();unit=root/'unit';unit.mkdir(mode=0o700)
            holder=helper(unit)
            assert select.select([holder.stdout],[],[],3)[0] and holder.stdout.readline()==b'LOCKED\n'
            lock=unit/'supervisor.lock';inode=lock.stat().st_ino
            assert stat.S_IMODE(lock.stat().st_mode)==0o600
            result['cases']['held-denied']=attempt(unit);assert result['cases']['held-denied']['rc']==1
            holder.kill();holder.wait(timeout=3);holder=None
            result['cases']['death-release']=attempt(unit);assert result['cases']['death-release']['rc']==0 and lock.stat().st_ino==inode
            lock.chmod(0o644);before=lock.read_bytes();result['cases']['bad-permission']=attempt(unit);assert result['cases']['bad-permission']['rc']==1 and lock.read_bytes()==before and stat.S_IMODE(lock.stat().st_mode)==0o644
            # Only our private fixture is replaced, never production/others' locks.
            lock.unlink();target=unit/'target';target.write_bytes(b'UNCHANGED');target.chmod(0o600);lock.symlink_to(target)
            result['cases']['symlink']=attempt(unit);assert result['cases']['symlink']['rc']==1 and lock.is_symlink() and target.read_bytes()==b'UNCHANGED'
            lock.unlink();lock.mkdir(mode=0o700);result['cases']['directory']=attempt(unit);assert result['cases']['directory']['rc']==1 and lock.is_dir()
            candidates=root/'candidates';candidates.mkdir(mode=0o700);session=root/'session';session.mkdir(mode=0o700)
            result['build']=command([sys.executable,str(APP/'reload_candidate.py'),'build',str(APP),str(ROOT/'unisacc.com'),str(candidates)],55);assert result['build']['rc']==0,result['build']
            info=json.loads(result['build']['stdout']);module.candidate.verify(info['candidate_dir'])
            # __init__ and run exceptions close only this object's descriptors.
            invalid=root/'invalid';invalid.mkdir(mode=0o700);events=invalid/'launcher-events.jsonl';events.write_bytes(b'KEEP');events.chmod(0o644)
            try:module.Launcher(APP,ROOT/'unisacc.com',candidates,invalid,'sess',info['candidate_dir']);raise AssertionError('bad events accepted')
            except ValueError:pass
            assert attempt(invalid)['rc']==0 and events.read_bytes()==b'KEEP'
            no_tty=root/'no-tty';no_tty.mkdir(mode=0o700);obj=module.Launcher(APP,ROOT/'unisacc.com',candidates,no_tty,'sess',info['candidate_dir'])
            try:obj.run();raise AssertionError('nonTTY accepted')
            except ValueError:pass
            assert obj.lock_fd==-1 and attempt(no_tty)['rc']==0
            home=root/'home';home.mkdir(mode=0o700);proof=root/'live.json';child_result=root/'child.json'
            pid,master=pty.fork()
            if pid==0:
                os.environ.update(HOME=str(home),DEEPSEEK_API_KEY='LOCAL_STUB_ONLY',CSIH_ENDPOINT='http://127.0.0.1:1/unreachable',CSIH_ROLE='write',CSIH_PEER='',CSIH_CWD=str(root))
                baseline=termios.tcgetattr(0)
                class LiveLauncher(module.Launcher):
                    def event(self,name,**fields):
                        super().event(name,**fields)
                        if name=='activated':
                            attrs=termios.tcgetattr(0);data=(session/'launcher-events.jsonl').read_bytes();st=(session/'supervisor.lock').stat()
                            artifacts={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in candidates.rglob('*') if p.is_file()}
                            p=subprocess.Popen([sys.executable,str(APP/'reload_launcher.py'),'run',str(APP),str(ROOT/'unisacc.com'),str(candidates),str(session),'other-session',info['candidate_dir']],stderr=subprocess.PIPE)
                            try:_,err=p.communicate(timeout=3)
                            except subprocess.TimeoutExpired:p.kill();p.wait();raise AssertionError('second host hung')
                            assert p.returncode==1 and b'Resource temporarily unavailable' in err,err
                            assert termios.tcgetattr(0)==attrs and (session/'launcher-events.jsonl').read_bytes()==data
                            assert (session/'supervisor.lock').stat().st_ino==st.st_ino and (session/'supervisor.lock').stat().st_mode==st.st_mode
                            assert artifacts=={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in candidates.rglob('*') if p.is_file()}
                            assert self.active.process.poll() is None
                            fd=os.open(session/'owner.lock',os.O_RDWR)
                            try:
                                try:fcntl.lockf(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);raise AssertionError('first actor owner absent')
                                except BlockingIOError:pass
                            finally:os.close(fd)
                            # Host lock must not survive exec into actor.
                            assert not os.get_inheritable(self.lock_fd)
                            proof.write_text(json.dumps(dict(second_rc=p.returncode,stderr=err.decode(),events_unchanged=True,termios_unchanged=True,artifacts_unchanged=True,owner_held=True)))
                try:
                    launcher=LiveLauncher(APP,ROOT/'unisacc.com',candidates,session,'sess',info['candidate_dir']);rc=launcher.run()
                    child_result.write_text(json.dumps(dict(rc=rc,baseline_restored=termios.tcgetattr(0)==baseline,closed_lock=launcher.lock_fd==-1)));os._exit(0 if rc==0 else 1)
                except Exception as error:child_result.write_text(json.dumps(dict(error=repr(error))));os._exit(1)
            fcntl.ioctl(master,termios.TIOCSWINSZ,struct.pack('HHHH',35,120,0,0));output=b'';deadline=time.monotonic()+14
            def pump():
                nonlocal output
                assert time.monotonic()<deadline,'live CTTY exceeded14s'
                if select.select([master],[],[],.03)[0]:
                    try:output+=os.read(master,65536)
                    except OSError:pass
            while True:
                try:result['live']=json.loads(proof.read_text());break
                except FileNotFoundError:pump()
            os.write(master,'第二宿主拒绝后仍编辑'.encode());end=time.monotonic()+.4
            while time.monotonic()<end:pump()
            assert '第二宿主拒绝后仍编辑' in output.decode(errors='replace')
            os.write(master,b'\x03');end=time.monotonic()+.15
            while time.monotonic()<end:pump()
            os.write(master,b'/exit');end=time.monotonic()+.15
            while time.monotonic()<end:pump()
            os.write(master,b'\r')
            while True:
                pump();done,status=os.waitpid(pid,os.WNOHANG)
                if done:pid=None;break
            os.close(master);result['child']=json.loads(child_result.read_text());result['output']=output.decode(errors='replace')
            assert result['child']==dict(rc=0,baseline_restored=True,closed_lock=True),result['child']
            assert attempt(session)['rc']==0 # normal final release, same inode remains
            result['passed']=True
    except Exception as error:result.update(passed=False,error=repr(error))
    finally:
        if holder:holder.kill();holder.wait(timeout=3)
        if pid:os.killpg(pid,signal.SIGKILL);os.waitpid(pid,0)
        result['after']=hashes();assert result['before']==result['after']
        pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '/tmp/csih-launcher-lock-review.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
    print('PASS supervisor lock and second-host CTTY isolation' if result.get('passed') else 'FAIL '+result.get('error','unknown'),flush=True)
    return 0 if result.get('passed') else 1
if __name__=='__main__':raise SystemExit(main())
