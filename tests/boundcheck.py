#!/usr/bin/env python3
"""Exit status and timeout cleanup, including a child in another session."""
import os, pathlib, signal, subprocess, sys, tempfile, time
root = pathlib.Path(__file__).resolve().parent
native = subprocess.check_output([str(root/'bound'), '--helper'], text=True, timeout=60).strip()
for native_code in (0, 2, 142):
    with tempfile.TemporaryDirectory(prefix='bound-status-') as td:
        status = pathlib.Path(td)/'status'
        rc = subprocess.run([native, '--status', str(status), '5', sys.executable, '-c', f'raise SystemExit({native_code})'], timeout=7).returncode
        assert rc == native_code and int(status.read_text()) == native_code << 8
for bound in ([native], [sys.executable, str(root/'bound.py')]):
    for code in (0, 2, 142):
        assert subprocess.run(bound + ['5', sys.executable, '-c', f'raise SystemExit({code})'], timeout=7).returncode == code
    assert subprocess.run(bound + ['5', sys.executable, '-c', 'import os,signal;os.kill(os.getpid(),signal.SIGTERM)'], timeout=7).returncode == 143
    with tempfile.TemporaryDirectory(prefix='bound-check-') as td:
        marker=pathlib.Path(td)/'child'
        child='import os,time;os.setsid();time.sleep(30)'
        code='import subprocess,time,pathlib,sys;c=subprocess.Popen([sys.executable,"-c",sys.argv[2]]);pathlib.Path(sys.argv[1]).write_text(str(c.pid));time.sleep(30)'
        result=subprocess.run(bound + ['1',sys.executable,'-c',code,str(marker),child],timeout=5)
        assert result.returncode == 142
        time.sleep(.1)
        state=subprocess.run(['/bin/ps','-p',marker.read_text(),'-o','stat='],capture_output=True,text=True,timeout=2).stdout.strip()   # ps exits 1 once the pid is gone: that is the pass case
        assert not state or state.startswith('Z'), state
    print('bound: normal/signal exits and detached descendant timeout pass')

# Signals delivered to the watchdog itself must also reap a stopped,
# detached descendant. Repeated termination must not interrupt cleanup.
for bound in ([native], [sys.executable, str(root/'bound.py')]):
    for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        with tempfile.TemporaryDirectory(prefix='bound-signal-') as td:
            marker = pathlib.Path(td)/'child'
            child = 'import os,signal,time;os.setsid();os.kill(os.getpid(),signal.SIGSTOP);time.sleep(30)'
            code = 'import subprocess,time,pathlib,sys;c=subprocess.Popen([sys.executable,"-c",sys.argv[2]]);pathlib.Path(sys.argv[1]).write_text(str(c.pid));time.sleep(30)'
            process = subprocess.Popen(bound + ['5',sys.executable,'-c',code,str(marker),child])
            try:
                deadline = time.monotonic()+2
                while True:
                    try: pid = int(marker.read_text()); break
                    except FileNotFoundError:
                        assert time.monotonic()<deadline
                        time.sleep(.02)
                time.sleep(.1)
                process.send_signal(sig)
                assert process.wait(timeout=3)==128+sig
                state=subprocess.run(['/bin/ps','-p',str(pid),'-o','stat='],capture_output=True,text=True,timeout=2).stdout.strip()   # ps exits 1 once the pid is gone
                assert not state or state.startswith('Z'), (bound,sig,state)
            finally:
                if process.poll() is None:
                    process.kill(); process.wait()
    # A command that succeeds must not leave its ordinary process group alive.
    with tempfile.TemporaryDirectory(prefix='bound-background-') as td:
        marker=pathlib.Path(td)/'child'
        code='import subprocess,pathlib,sys;c=subprocess.Popen([sys.executable,"-c","import time;time.sleep(30)"]);pathlib.Path(sys.argv[1]).write_text(str(c.pid))'
        assert subprocess.run(bound+['5',sys.executable,'-c',code,str(marker)],timeout=7).returncode==0
        state=subprocess.run(['/bin/ps','-p',marker.read_text(),'-o','stat='],capture_output=True,text=True,timeout=2).stdout.strip()   # ps exits 1 once the pid is gone: that is the pass case
        assert not state or state.startswith('Z'),state
    print('bound: TERM/INT/HUP stopped detached cleanup and successful background cleanup pass')

# Exercise the Terminal ownership protocol without opening a user's windows.
with tempfile.TemporaryDirectory(prefix='term-ownership-') as td:
    mock=pathlib.Path(td); log=mock/'closed'
    (mock/'uname').write_text('#!/bin/sh\necho Darwin\n')
    script = """import os,pathlib,re,subprocess,sys
text=' '.join(sys.argv[1:])
if 'to close' in text:
    assert 'id is 4242' in text
    pathlib.Path(os.environ['TERM_TEST_CLOSED']).write_text('4242')
else:
    match=re.search(r"'([^']+/run.sh)'",text);assert match,text
    subprocess.Popen(['/bin/sh',match.group(1)],start_new_session=True)
    print('4242')
"""
    (mock/'osascript').write_text('#!'+sys.executable+'\n'+script)
    for name in ('uname','osascript'): (mock/name).chmod(0o755)
    env=dict(os.environ,PATH=str(mock)+os.pathsep+os.environ['PATH'],TERM_TEST_CLOSED=str(log),TERM_SH='1',TERM_SH_NOFALLBACK='1',TERM_SH_ALARM='3')
    env.pop('TERM_SH_INSIDE',None)
    for command,want in (([sys.executable,'-c','print("done")'],0),([sys.executable,'-c','raise SystemExit(2)'],2),([sys.executable,'-c','import time;time.sleep(30)'],142)):
        log.unlink(missing_ok=True)
        result=subprocess.run([str(root/'term.sh'),*command],env=env,capture_output=True,text=True,timeout=10)
        assert result.returncode==want,(result.returncode,result.stderr)
        assert log.read_text()=='4242'
    log.unlink(missing_ok=True)
    process=subprocess.Popen([str(root/'term.sh'),sys.executable,'-c','import time;time.sleep(30)'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    time.sleep(.5);process.terminate()
    assert process.wait(timeout=7)==143
    assert log.read_text()=='4242'
    print('term: owned window closes on success, failure, timeout and signal (mock UI)')

# Run the real Linux wrapper against an isolated mock guest, including its tar
# transfer and shell traps. This checks deletion scope, not VM compatibility.
with tempfile.TemporaryDirectory(prefix='linux-cleanup-') as td:
    base=pathlib.Path(td); fixture=base/'repo'; guest=base/'guest'; tools=base/'bin'
    (fixture/'tests').mkdir(parents=True); (fixture/'corpus/c-testsuite/tests/single-exec').mkdir(parents=True)
    guest.mkdir(); tools.mkdir()
    (fixture/'tests/linux.sh').write_bytes((root/'linux.sh').read_bytes())
    (fixture/'tests/probe.sh').write_text('#!/bin/sh\necho latest-failure\nexit 3\n');(fixture/'tests/probe.sh').chmod(0o755)
    driver="""import os,subprocess,sys
if sys.argv[1]=='list':
    print('Running' if 'Status' in ' '.join(sys.argv) else ('aarch64' if os.uname().machine=='arm64' else os.uname().machine))
elif '/proc/meminfo' in sys.argv: print('8000000')
elif 'bash' not in sys.argv: print('yes')
else:
    env=dict(os.environ,HOME=os.environ['LINUX_TEST_HOME'])
    raise SystemExit(subprocess.run(['/bin/bash','-c',sys.argv[-1]],env=env).returncode)
"""
    (tools/'limactl').write_text('#!'+sys.executable+'\n'+driver);(tools/'limactl').chmod(0o755)
    env=dict(os.environ,PATH=str(tools)+os.pathsep+os.environ['PATH'],LINUX_TEST_HOME=str(guest),STRICT='1')
    for name in ('unisa-linux','unisa-tmp'):
        (guest/name).mkdir();(guest/name/'old-tree').write_text('old')
    (guest/'unrelated').write_text('keep')
    result=subprocess.run(['/bin/bash',str(fixture/'tests/linux.sh'),'probe'],env=env,capture_output=True,text=True,timeout=10)
    assert result.returncode==3,(result.returncode,result.stderr,result.stdout)
    assert (guest/'unisa-linux-last-failure.log').read_text()=='latest-failure\n'
    assert (guest/'unrelated').read_text()=='keep'
    for name in ('unisa-linux','unisa-tmp','.unisa-linux-lock','unisa-linux-current.log'):
        try: (guest/name).stat()
        except FileNotFoundError: pass
        else: raise AssertionError(name)
    (fixture/'tests/probe.sh').write_text('#!/bin/sh\necho success\n')
    assert subprocess.run(['/bin/bash',str(fixture/'tests/linux.sh'),'probe'],env=env,capture_output=True,text=True,timeout=10).returncode==0
    assert (guest/'unisa-linux-last-failure.log').read_text()=='latest-failure\n'
    print('linux: old scratch cleared; failure tree removed; latest failure log retained (mock guest)')
