#!/usr/bin/env python3
"""Stable CTTY supervisor; trusted private candidates, native actors own user input."""
import fcntl, json, os, pathlib, re, select, signal, socket, stat, subprocess, sys, termios, time, uuid
import reload_candidate as candidate

class Blocked(RuntimeError): pass

def acquire_supervisor_lock(directory):
    directory=pathlib.Path(directory).absolute()
    candidate.trusted(directory,True,True)
    fd=os.open(directory/'supervisor.lock',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW|os.O_CLOEXEC,0o600)
    try:
        sb=os.fstat(fd)
        if not stat.S_ISREG(sb.st_mode) or sb.st_uid!=os.getuid() or stat.S_IMODE(sb.st_mode)!=0o600:
            raise ValueError('supervisor lock not owned regular0600')
        os.set_inheritable(fd,False)
        fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        return fd
    except BaseException:
        os.close(fd);raise


class Actor:
    def __init__(self, info, directory, sid, handoff, standby):
        info=candidate.verify(info['candidate_dir']) # immediately before native spawn
        self.identity=dict(session=sid,handoff=handoff,hash=info['hash'])
        self.socket,child=socket.socketpair();self.socket.setblocking(False);self.buffer=b'';self.messages=[]
        argv=[info['binary'],'standby-managed' if standby else 'agent-managed',str(directory),sid]
        if standby:argv.append(handoff)
        argv += [info['hash'],str(child.fileno())]
        try:self.process=subprocess.Popen(argv,pass_fds=(child.fileno(),)) # same CTTY foreground group
        finally:child.close()
    def send(self,op,identity=None):
        data=(json.dumps(dict(op=op,**(identity or self.identity)),separators=(',',':'))+'\n').encode()
        if self.socket.send(data)!=len(data):raise OSError('partial control send')
    def read(self):
        try:data=self.socket.recv(4096)
        except BlockingIOError:return
        if not data:raise EOFError('actor control EOF')
        self.buffer+=data
        while b'\n' in self.buffer:
            line,self.buffer=self.buffer.split(b'\n',1)
            if len(line)>1023:raise ValueError('oversize control line')
            obj=json.loads(line,object_pairs_hook=candidate.unique_object)
            if set(obj)!={'op','session','handoff','hash'} or any(type(v) is not str or '\0' in v for v in obj.values()):raise ValueError('control schema')
            if obj['op'] not in ('ACK','READY','REQUEST','REJECT','FROZEN','RELEASED','BLOCKED','RESUMED'):raise ValueError('unknown actor op')
            if not re.fullmatch(r'[A-Za-z0-9:_-]{1,64}',obj['session']) or not re.fullmatch(r'[A-Za-z0-9:_-]{1,64}',obj['handoff']) or not re.fullmatch(r'[0-9a-fA-F]{64}',obj['hash']):raise ValueError('control identity syntax')
            self.messages.append(obj)
        if len(self.buffer)>1023:raise ValueError('oversize control frame')
    def stop(self):
        if self.process.poll() is None:self.process.terminate()
        try:self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            try:self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:raise Blocked('candidate exit not confirmed')
        self.socket.close()

class Launcher:
    def __init__(self,source,compiler,root,directory,sid,initial=None):
        self.source=pathlib.Path(source).absolute();self.compiler=pathlib.Path(compiler).absolute();self.root=pathlib.Path(root).absolute();self.directory=pathlib.Path(directory).absolute();self.sid=sid
        candidate.trusted(self.root,True,True);candidate.trusted(self.directory,True,True)
        if not re.fullmatch(r'[A-Za-z0-9:_-]{1,64}',sid):raise ValueError('invalid session')
        self.initial=initial;self.active=None;self.build=None;self.build_started=0;self.actors=[];self.pending=None
        self.lock_fd=-1;self.event_fd=-1
        try:
            # Must precede event writes, TTY capture, builds and native spawn.
            self.lock_fd=acquire_supervisor_lock(self.directory)
            self.event_fd=os.open(self.directory/'launcher-events.jsonl',os.O_WRONLY|os.O_APPEND|os.O_CREAT|os.O_NOFOLLOW|os.O_CLOEXEC,0o600)
            candidate.trusted(self.directory/'launcher-events.jsonl',False,True)
        except BaseException as primary:
            try:self.close_host()
            except OSError as cleanup:raise primary from cleanup
            raise
    def close_host(self):
        error=None
        for name in ('event_fd','lock_fd'):
            fd=getattr(self,name,-1);setattr(self,name,-1)
            if fd>=0:
                try:os.close(fd)
                except OSError as cause:
                    if error is None:error=cause
        if error:raise error
    def event(self,name,**fields):
        os.write(self.event_fd,(json.dumps(dict(event=name,**fields),sort_keys=True)+'\n').encode())
    def start_build(self):
        if self.build: self.event('request-rejected',reason='build already running');return
        self.build=subprocess.Popen([sys.executable,str(pathlib.Path(candidate.__file__)),'build',str(self.source),str(self.compiler),str(self.root)],stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
        self.build_started=time.monotonic();self.event('build-started')
    def end_build(self,kill=False):
        p=self.build
        if kill and p.poll() is None:os.killpg(p.pid,signal.SIGKILL)
        try:out,err=p.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            os.killpg(p.pid,signal.SIGKILL);out,err=p.communicate(timeout=2)
        self.build=None
        if kill or p.returncode!=0:raise ValueError('build failed rc=%s: %s %s'%(p.returncode,out.decode(errors='replace'),err.decode(errors='replace')))
        info=json.loads(out);return candidate.verify(info['candidate_dir'])
    def wait(self,actor,op,identity=None):
        identity=identity or actor.identity;deadline=time.monotonic()+8
        while time.monotonic()<deadline:
            if actor.messages:
                msg=actor.messages.pop(0)
                if any(msg[k]!=identity[k] for k in identity):raise ValueError('stale actor identity')
                if msg['op']!=op:raise ValueError('expected '+op+' received '+msg['op'])
                self.event('received',op=op,**identity);return msg
            if select.select([actor.socket],[],[],.1)[0]:actor.read()
            if actor.process.poll() is not None:raise EOFError('actor exited before '+op)
        raise TimeoutError('waiting '+op)
    def spawn(self,info,handoff,standby):
        actor=Actor(info,self.directory,self.sid,handoff,standby);self.actors.append(actor)
        self.event('spawn',pid=actor.process.pid,standby=standby,**actor.identity);return actor
    def handoff(self,info):
        old=self.active;new=None;frozen=False;freeze_sent=False;activated=False
        identity=dict(session=self.sid,handoff=uuid.uuid4().hex,hash=info['hash'])
        try:
            new=self.spawn(info,identity['handoff'],True);self.wait(new,'READY')
            freeze_sent=True;old.send('FREEZE',identity)
            try:self.wait(old,'FROZEN',identity)
            except ValueError as error:
                if 'received REJECT' in str(error):freeze_sent=False
                raise
            frozen=True;old.send('RELEASE',identity);self.wait(old,'RELEASED',identity)
            new.send('COMMIT');self.wait(new,'ACK')
            new.send('ACTIVATE');activated=True;self.active=new
            self.event('activated',pid=new.process.pid,**new.identity)
            old.send('RETIRE',identity)
            try:old.process.wait(timeout=3);old.socket.close()
            except (subprocess.TimeoutExpired,OSError):old.stop();self.event('retire-forced',pid=old.process.pid)
            self.event('committed',pid=new.process.pid,**new.identity)
        except Exception as error:
            if activated:
                # New state already committed: never advertise rollback/reapply.
                self.event('committed-warning',reason=str(error),**new.identity)
                old.stop();return
            self.event('handoff-failed',reason=str(error),freeze_sent=freeze_sent)
            if new:
                new.stop();self.event('candidate-exited',pid=new.process.pid,rc=new.process.returncode)
            if freeze_sent:
                old.send('RESUME',identity)
                try:self.wait(old,'RESUMED',identity)
                except Exception as resume_error:raise Blocked('old paused; resume unconfirmed: '+str(resume_error))
                self.event('resumed',pid=old.process.pid,**old.identity)
    def run(self):
        baseline=None
        try:
            if self.lock_fd<0:raise ValueError('supervisor ownership absent')
            if not os.isatty(0) or not os.isatty(1) or os.tcgetpgrp(0)!=os.getpgrp():raise ValueError('launcher requires foreground controlling TTY')
            baseline=termios.tcgetattr(0);self.baseline=baseline
            if self.initial:info=candidate.verify(self.initial)
            else:
                self.start_build()
                self.build.wait(timeout=55);info=self.end_build()
            self.active=self.spawn(info,'initial',False);self.wait(self.active,'ACK');self.active.send('ACTIVATE');self.event('activated',pid=self.active.process.pid,**self.active.identity)
            while self.active.process.poll() is None:
                if select.select([self.active.socket],[],[],.05)[0]:
                    self.active.read()
                    while self.active.messages:
                        msg=self.active.messages.pop(0)
                        if msg!=dict(op='REQUEST',**self.active.identity):raise ValueError('unexpected/stale active message')
                        self.start_build()
                if self.build:
                    if time.monotonic()-self.build_started>55:
                        try:self.end_build(True)
                        except Exception as error:self.event('build-failed',reason=str(error))
                    elif self.build.poll() is not None:
                        try:info=self.end_build()
                        except Exception as error:self.event('build-failed',reason=str(error));continue
                        self.handoff(info)
            return self.active.process.returncode
        except EOFError:
            # Native normal exit closes the stream just before waitpid observes exit.
            if self.active:
                try:return self.active.process.wait(timeout=3)
                except subprocess.TimeoutExpired:pass
            raise
        finally:
            primary_error=sys.exc_info()[1];cleanup_error=None
            if self.build:
                try:self.end_build(True)
                except ValueError:pass # expected cancellation after confirmed process-group cleanup
                except Exception as error:cleanup_error=error
            for actor in self.actors:
                try:
                    if actor.process.poll() is None:actor.stop()
                    else:actor.socket.close()
                except Exception as error:
                    if cleanup_error is None:cleanup_error=error
                    try:self.event('BLOCKED',reason=str(error))
                    except Exception:pass
            # Even failed cleanup must attempt final TTY restoration while locked.
            try:
                if baseline is not None:
                    termios.tcsetattr(0,termios.TCSAFLUSH,baseline);self.event('terminal-restored')
            except Exception as error:
                if cleanup_error is None:cleanup_error=error
            finally:
                try:self.close_host()
                except Exception as error:
                    if cleanup_error is None:cleanup_error=error
            if cleanup_error is not None and primary_error is None:
                raise Blocked('cleanup unconfirmed: '+str(cleanup_error))

def main():
    try:
        if len(sys.argv) not in (7,8) or sys.argv[1]!='run':raise ValueError('usage: run SOURCE_APP COMPILER PRIVATE_CANDIDATE_ROOT PRIVATE_SESSION_DIR SESSION [VERIFIED_INITIAL_DIR]')
        return Launcher(*sys.argv[2:7],initial=sys.argv[7] if len(sys.argv)==8 else None).run()
    except (ValueError,OSError,RuntimeError,EOFError,TimeoutError,subprocess.SubprocessError) as error:
        print('reload launcher: '+str(error),file=sys.stderr);return 1
if __name__=='__main__':raise SystemExit(main())
