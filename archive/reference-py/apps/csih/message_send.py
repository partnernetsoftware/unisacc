#!/usr/bin/env python3
"""Publish bounded messages, not delivery/identity authentication.

Only the final existing session directory and opened children are checked;
trusted ancestors/single-writer private directory are caller prerequisites.
The future consumer must share inbox/mailbox.lock POSIX record locking and
retain the original five-key envelope in ready/started/done id.json files.
No TTY, model, consumer, automatic reply or stale-file cleanup is involved.
"""
import fcntl, json, os, pathlib, re, stat, sys, uuid
from reload_candidate import trusted, unique_object

LIMIT=32768
STATES=('ready','started','done')

class MessageError(ValueError):pass

def require(condition,cause):
    if not condition:raise MessageError(cause)

def decode(raw,expected):
    require(type(raw) is bytes and len(raw)<=LIMIT,'input exceeds32768')
    require(type(expected) is str and re.fullmatch(r'[A-Za-z0-9:_-]{1,64}',expected),'invalid expected session')
    try:text=raw.decode('utf-8',errors='strict')
    except UnicodeError:raise MessageError('invalid UTF8')
    try:
        obj=json.loads(text,object_pairs_hook=unique_object,parse_constant=lambda _: (_ for _ in ()).throw(MessageError('invalid JSON number')))
    except (ValueError,UnicodeError):raise MessageError('invalid JSON or duplicate keys')
    require(type(obj) is dict and set(obj)=={'version','id','session','kind','body'},'message schema mismatch')
    require(all('\0' not in key for key in obj),'NUL key')
    require(all('\0' not in value for value in obj.values() if type(value) is str),'NUL value')
    require(type(obj['version']) in (int,float) and obj['version']==1,'version must be numeric1')
    require(type(obj['id']) is str and re.fullmatch(r'[0-9a-f]{32}',obj['id']),'invalid message id')
    require(type(obj['session']) is str and obj['session']==expected,'session mismatch')
    require(type(obj['kind']) is str and obj['kind'] in ('task','notice'),'invalid message kind')
    require(type(obj['body']) is str and obj['body'],'empty/nonstring body')
    try:body=obj['body'].encode('utf-8',errors='strict')
    except UnicodeError:raise MessageError('body is not UTF8 representable')
    require(len(body)<=4095,'body exceeds4095 UTF8 bytes')
    obj['version']=1 # numeric1 and1.0 have the same normalized envelope
    canonical=json.dumps(obj,sort_keys=True,separators=(',',':'),ensure_ascii=False,allow_nan=False).encode('utf-8')
    require(len(canonical)<=LIMIT,'normalized message exceeds32768')
    return obj,canonical

def checked(fd,directory=False):
    sb=os.fstat(fd)
    require(sb.st_uid==os.getuid() and (stat.S_ISDIR(sb.st_mode) if directory else stat.S_ISREG(sb.st_mode)),'untrusted opened file kind/owner')
    require(stat.S_IMODE(sb.st_mode)==(0o700 if directory else 0o600),'wrong private permissions')
    return sb

def directory(parent,name):
    try:os.mkdir(name,0o700,dir_fd=parent)
    except FileExistsError:pass
    fd=os.open(name,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW|os.O_CLOEXEC,dir_fd=parent)
    try:checked(fd,True)
    except BaseException:os.close(fd);raise
    return fd

def read_envelope(parent,name,expected):
    fd=os.open(name,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC,dir_fd=parent)
    try:
        sb=checked(fd);require(sb.st_size<=LIMIT,'stored envelope oversized')
        chunks=[];remaining=LIMIT+1
        while remaining:
            part=os.read(fd,remaining)
            if not part:break
            chunks.append(part);remaining-=len(part)
        obj,canonical=decode(b''.join(chunks),expected)
        require(name==obj['id']+'.json','stored id/filename mismatch')
        return canonical
    finally:os.close(fd)

def scan(inbox,statefds,expected,wanted):
    require(set(os.listdir(inbox))==set(STATES)|{'mailbox.lock'},'unknown inbox entry')
    ids=set();unfinished=0;existing=None
    for state in STATES:
        with os.scandir(statefds[state]) as entries:
            for entry in entries:
                require(re.fullmatch(r'[0-9a-f]{32}\.json',entry.name),'unknown state directory entry')
                mid=entry.name[:-5];require(mid not in ids,'id present in multiple states')
                require(len(ids)<1024,'retained message capacity exceeded')
                canonical=read_envelope(statefds[state],entry.name,expected)
                ids.add(mid)
                if state!='done':unfinished+=1
                require(unfinished<=32,'unfinished message capacity exceeded')
                if mid==wanted:existing=canonical
    return len(ids),unfinished,existing

def publish(session_dir,expected,raw):
    obj,canonical=decode(raw,expected)
    receipt=dict(kind=obj['kind'],id=obj['id'],session=obj['session'],status='rejected')
    fds=[];statefds={};temporary=None;tempfd=-1;visible=False;error=None;status=None
    try:
        path=pathlib.Path(session_dir)
        require(path.is_absolute(),'session directory must be absolute')
        trusted(path,True,True)
        parent=os.open(path,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW|os.O_CLOEXEC);fds.append(parent);checked(parent,True)
        inbox=directory(parent,'inbox');fds.append(inbox)
        lock=os.open('mailbox.lock',os.O_RDWR|os.O_CREAT|os.O_NOFOLLOW|os.O_CLOEXEC,0o600,dir_fd=inbox);fds.append(lock);checked(lock)
        try:fcntl.lockf(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:raise MessageError('mailbox busy')
        for state in STATES:statefds[state]=directory(inbox,state);fds.append(statefds[state])
        retained,unfinished,existing=scan(inbox,statefds,expected,obj['id'])
        if existing is not None:
            require(existing==canonical,'id conflicts with existing content')
            visible=True;status='already-published'
        else:
            require(unfinished<32,'unfinished capacity32 reached')
            require(retained<1024,'retained capacity1024 reached')
            # Persist any newly created namespace before exposing an envelope.
            os.fsync(lock);os.fsync(inbox);os.fsync(parent)
            ready=statefds['ready'];tempname='.publish-'+uuid.uuid4().hex+'.tmp'
            tempfd=os.open(tempname,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW|os.O_CLOEXEC,0o600,dir_fd=ready)
            temporary=tempname;checked(tempfd)
            position=0
            while position<len(canonical):
                n=os.write(tempfd,canonical[position:]);require(n>0,'short temporary write');position+=n
            os.fsync(tempfd);fd=tempfd;tempfd=-1;os.close(fd)
            # link is atomic, fails on existing target, and exposes complete bytes.
            os.link(temporary,obj['id']+'.json',src_dir_fd=ready,dst_dir_fd=ready,follow_symlinks=False)
            visible=True;status='published'
            os.unlink(temporary,dir_fd=ready);temporary=None
            os.fsync(ready)
    except (OSError,ValueError) as cause:error=cause
    finally:
        if tempfd>=0:
            try:os.close(tempfd)
            except OSError as cause:
                if error is None:error=cause
        if temporary is not None:
            try:os.unlink(temporary,dir_fd=statefds['ready'])
            except OSError as cause:
                if error is None:error=cause
        # Lock is retained during our own temp cleanup; never unlink a target/lock.
        for fd in reversed(fds):
            try:os.close(fd)
            except OSError as cause:
                if error is None:error=cause
    if error is not None:
        receipt['status']='published-unconfirmed' if visible else 'rejected'
        cause=str(error) if isinstance(error,MessageError) else ('filesystem error errno='+str(getattr(error,'errno',None)))
        return (3 if visible else 1),receipt,cause
    receipt['status']=status
    return 0,receipt,None

def read_input(filename):
    if filename=='-':return sys.stdin.buffer.read(LIMIT+1)
    fd=os.open(filename,os.O_RDONLY|os.O_NOFOLLOW|os.O_CLOEXEC)
    try:
        require(stat.S_ISREG(os.fstat(fd).st_mode),'JSON input must be regular')
        data=[];remaining=LIMIT+1
        while remaining:
            part=os.read(fd,remaining)
            if not part:break
            data.append(part);remaining-=len(part)
        return b''.join(data)
    finally:os.close(fd)

def main():
    receipt=dict(kind=None,id=None,session=None,status='rejected')
    try:
        require(len(sys.argv)==5 and sys.argv[1]=='publish','usage: publish PRIVATE_SESSION_DIR EXPECTED_SESSION JSON_FILE (- for stdin)')
        raw=read_input(sys.argv[4]);rc,receipt,cause=publish(sys.argv[2],sys.argv[3],raw)
    except (OSError,ValueError) as error:
        rc=1;cause=str(error) if isinstance(error,MessageError) else 'input/directory error'
    print(json.dumps(receipt,sort_keys=True,separators=(',',':')))
    if cause:print('message publisher: '+cause,file=sys.stderr)
    return rc
if __name__=='__main__':raise SystemExit(main())
