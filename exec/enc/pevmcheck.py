"""Explicit opt-in: verify existing PE artifacts in an already-running UTM VM.
Run under a 60 s outer alarm; each guest program has its own 30 s timeout.
Does not start/stop VMs. Files and completion receipts are unique per run.
"""
import pathlib,subprocess,hashlib,time,uuid,sys,os
utm='/Applications/UTM.app/Contents/MacOS/utmctl';vm=os.environ.get('WINVM','minicon-win-arm-64');root=pathlib.Path(sys.argv[1])
def tool(*args,data=None):
 r=subprocess.run([utm,*args],input=data,capture_output=True,timeout=45)
 if r.returncode:raise RuntimeError((args,r.returncode,r.stdout,r.stderr))
 return r.stdout
for name in ('hello','fib'):
 exe='C:\\u\\delta-pe-'+name+'-'+uuid.uuid4().hex[:8]+'.exe';out=exe+'.out';err=exe+'.err';rc=exe+'.rc';ps=exe+'.ps1'
 data=(root/(name+'.exe')).read_bytes();tool('file','push',vm,exe,data=data)
 assert tool('file','pull',vm,exe)==data
 script=f"$p = New-Object System.Diagnostics.Process; $p.StartInfo.FileName = 'cmd.exe'; $p.StartInfo.Arguments = '/c {exe} >{out} 2>{err}'; $p.StartInfo.UseShellExecute = $false; $p.Start() | Out-Null; if (-not $p.WaitForExit(30000)) {{ & taskkill.exe /PID $p.Id /T /F | Out-Null; 'TIMEOUT' | Set-Content '{rc}' }} else {{ $p.ExitCode | Set-Content '{rc}' }}"
 tool('file','push',vm,ps,data=script.encode())
 tool('exec',vm,'--cmd','cmd.exe','--','/c','powershell.exe -NoProfile -ExecutionPolicy Bypass -File '+ps)
 deadline=time.monotonic()+35
 code=''
 while time.monotonic()<deadline:
  code=tool('file','pull',vm,rc).decode().strip()
  if code:break
  time.sleep(0.25)
 got=tool('file','pull',vm,out);stderr=tool('file','pull',vm,err)
 assert code=='0',(name,code,stderr)
 assert got.replace(b'\r\n',b'\n')==(root/(name+'.want')).read_bytes(),(name,got)
 print(name,'Windows ARM64 exit 0, output equals host cc; sha256',hashlib.sha256(data).hexdigest(),flush=True)
