#!/usr/bin/env python3
"""Opt-in actual Windows -run checks; caller owns VM lifecycle.
Use an outer 60 s limit. Each guest process has a 15 s tree watchdog.
"""
import hashlib,os,pathlib,subprocess,sys,time,uuid
p=pathlib.Path(sys.argv[1]);driver=pathlib.Path(sys.argv[2]);target=sys.argv[3]
assert target in ('win/arm64','win/x86_64')
utm='/Applications/UTM.app/Contents/MacOS/utmctl';vm=os.environ.get('WINVM','minicon-win-arm-64')
prefix='C:\\u\\mem-'+uuid.uuid4().hex[:8]
def tool(*args,data=None):
 r=subprocess.run([utm,*args],input=data,capture_output=True,timeout=20)
 if r.returncode or b'Error from event:' in r.stderr:raise RuntimeError((args,r.returncode,r.stdout,r.stderr))
 return r.stdout
def push(path,data):
 tool('file','push',vm,path,data=data);assert tool('file','pull',vm,path)==data,path
(p/'native-memory-prefix.txt').write_text(prefix)
embedded=os.environ.get('MEMORY_EMBEDDED','0')=='1'
image=driver.read_bytes()
if embedded: assert image[-16:-8]==b'UNIPKG1\n', 'embedded test requires a package footer'
push(prefix+'.exe',image)
if not embedded: push(prefix+'.pkg',(p/'compiler.pkg').read_bytes())
cases=[('hello',pathlib.Path('examples/hello.c').read_bytes(),'',0,b'hello from C99\r\n'),
       ('pointer',pathlib.Path('tests/c/b_funcptr.c').read_bytes(),'',0,None),
       ('args',b'#include <stdio.h>\nint main(int n,char **v){printf("%d %s\\n",n,v[1]);return 7;}\n',' -- sample',7,b'2 sample\r\n')]
# The expected pointer output is the host compiler's execution, not a generated image.
cc=p/'pointer-host'
r=subprocess.run(['cc','-include','stdio.h','tests/c/b_funcptr.c','-o',str(cc)],capture_output=True,timeout=60);assert r.returncode==0,r.stderr
r=subprocess.run([str(cc)],capture_output=True,timeout=10);assert r.returncode==0
cases[1]=(*cases[1][:4],r.stdout.replace(b'\n',b'\r\n'))
group=os.environ.get('MEMORY_CASES','basic');assert group in ('basic','io')
if group=='io':
 cases=[]
 for name in ('file','malloc'):
  source=pathlib.Path('tests/c/b_'+name+'.c').read_bytes();host=p/('host-'+name)
  r=subprocess.run(['cc','tests/c/b_'+name+'.c','-o',str(host)],capture_output=True,timeout=60);assert r.returncode==0,r.stderr
  r=subprocess.run([str(host)],cwd=p,capture_output=True,timeout=10);assert r.returncode==0,r.stderr
  if name=='file':source=source.replace(b'u_probe.txt',(prefix.replace('\\','/')+'.data').encode())
  cases.append((name,source,'',0,r.stdout))
 cases.append(('missing',b'', '',2,b''))
receipts=[]
for name,source,args,code,want in cases:
 path=prefix+'-'+name+'.c'
 if name!='missing':push(path,source)
 for level in ([2] if group=='io' else [1] if name=='args' else [0,2]):
  stem=prefix+'-'+name+str(level);log=stem+'.out';err=stem+'.err';rc=stem+'.rc';script=stem+'.ps1'
  package='' if embedded else f' --models {prefix}.pkg'
  command=f'{prefix}.exe{package} -O{level} -run {path}{args}'
  ps=f"$p = New-Object System.Diagnostics.Process; $p.StartInfo.FileName = 'cmd.exe'; $p.StartInfo.Arguments = '/c {command} >{log} 2>{err}'; $p.StartInfo.UseShellExecute = $false; $p.Start() | Out-Null; if (-not $p.WaitForExit(15000)) {{ & taskkill.exe /PID $p.Id /T /F | Out-Null; 'TIMEOUT' | Set-Content '{rc}' }} else {{ $p.ExitCode | Set-Content '{rc}' }}"
  push(script,ps.encode());tool('exec',vm,'--cmd','cmd.exe','--','/c','powershell.exe -NoProfile -ExecutionPolicy Bypass -File '+script)
  deadline=time.monotonic()+20;gotrc='';last_error=None
  while time.monotonic()<deadline:
   try:gotrc=tool('file','pull',vm,rc).decode().strip()
   except RuntimeError as error:last_error=error
   if gotrc:break
   time.sleep(.25)
  assert gotrc,(target,name,level,'missing exit receipt',last_error)
  out=tool('file','pull',vm,log);stderr=tool('file','pull',vm,err)
  # Our stdio writes LF directly; host Windows redirection does not translate it.
  err_ok=(b'run: cannot open ' in stderr) if name=='missing' else not stderr
  assert gotrc==str(code) and out.replace(b'\r\n',b'\n')==want.replace(b'\r\n',b'\n') and err_ok,(target,name,level,gotrc,out,stderr)
  receipts.append(f'{target} {name} O{level}: output and exit {code} passed')
  print(receipts[-1],flush=True)
print('driver sha256',hashlib.sha256(image).hexdigest(),'embedded' if embedded else 'external model package',flush=True)
(p/('native-memory-'+group+'-result.txt')).write_text('\n'.join(receipts)+'\n')
