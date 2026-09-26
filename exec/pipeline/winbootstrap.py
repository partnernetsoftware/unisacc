"""Opt-in Windows bootstrap of a delta-produced compiler.
An already-running VM is required; this script does not start/stop it.
Run with an outer 60 s alarm. Guest compile steps each have a 30 s watchdog.
"""
import pathlib,subprocess,hashlib,time,uuid,sys,os
utm='/Applications/UTM.app/Contents/MacOS/utmctl';vm=os.environ.get('WINVM','minicon-win-arm-64')
target=os.environ.get('TARGET','win/arm64');assert target in ('win/arm64','win/x86_64')
root=pathlib.Path(sys.argv[1]);prefix='C:\\u\\peboot-'+uuid.uuid4().hex[:8]
def tool(*args,data=None):
 r=subprocess.run([utm,*args],input=data,capture_output=True,timeout=45)
 if r.returncode:raise RuntimeError((args,r.returncode,r.stdout,r.stderr))
 return r.stdout
source=pathlib.Path(sys.argv[2]).read_bytes();n1=(root/'unisacc.exe').read_bytes()
peoff=int.from_bytes(n1[60:64],'little');assert n1[peoff:peoff+4]==b'PE\0\0'
assert int.from_bytes(n1[peoff+4:peoff+6],'little')=={'win/arm64':0xaa64,'win/x86_64':0x8664}[target]
for path,data in [(prefix+'.c',source),(prefix+'-n1.exe',n1)]:
 tool('file','push',vm,path,data=data);assert tool('file','pull',vm,path)==data
for i in (1,2):
 exe=prefix+'-n'+str(i)+'.exe';output=prefix+'-n'+str(i+1)+'.exe';ps=prefix+'-'+str(i)+'.ps1';rc=ps+'.rc';log=ps+'.log'
 command=exe+' -O2 -b '+target+' '+prefix+'.c -o '+output+' >'+log+' 2>&1'
 script=f"$p = New-Object System.Diagnostics.Process; $p.StartInfo.FileName = 'cmd.exe'; $p.StartInfo.Arguments = '/c {command}'; $p.StartInfo.UseShellExecute = $false; $p.Start() | Out-Null; if (-not $p.WaitForExit(30000)) {{ & taskkill.exe /PID $p.Id /T /F | Out-Null; 'TIMEOUT' | Set-Content '{rc}' }} else {{ $p.ExitCode | Set-Content '{rc}' }}"
 tool('file','push',vm,ps,data=script.encode());tool('exec',vm,'--cmd','cmd.exe','--','/c','powershell.exe -NoProfile -ExecutionPolicy Bypass -File '+ps)
 deadline=time.monotonic()+35;code='';pull_error=None
 while time.monotonic()<deadline:
  try:
   code=tool('file','pull',vm,rc).decode().strip()
  except RuntimeError as error:
   pull_error=error
   time.sleep(.25)
   continue
  if code:break
  time.sleep(.25)
 assert code,(i,'no exit receipt before polling deadline',pull_error)
 assert code=='0',(i,code,tool('file','pull',vm,log))
 data=tool('file','pull',vm,output);assert data==n1,(i,len(data),len(n1))
 (root/('n'+str(i+1)+'.exe')).write_bytes(data)
 print(target+' N'+str(i+1),len(data),'bytes equals N1; sha256',hashlib.sha256(data).hexdigest(),flush=True)
print('N1=N2=N3; source sha256',hashlib.sha256(source).hexdigest(),flush=True)
