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
        state=subprocess.run(['/bin/ps','-p',marker.read_text(),'-o','stat='],capture_output=True,text=True,timeout=2).stdout.strip()
        assert not state or state.startswith('Z'), state
    print('bound: normal/signal exits and detached descendant timeout pass')
