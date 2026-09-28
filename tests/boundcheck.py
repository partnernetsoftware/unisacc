#!/usr/bin/env python3
"""Exit status and timeout cleanup, including a child in another session."""
import os, pathlib, signal, subprocess, sys, tempfile, time
bound = [sys.executable, str(pathlib.Path(__file__).with_name('bound.py'))]
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
