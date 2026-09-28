#!/usr/bin/env python3
"""Execute an actual Windows probe through UTM; guest and RPC are bounded.
VM must already be running. The caller owns its startup/shutdown.
"""
import argparse,hashlib,json,pathlib,subprocess,time

def run(vm,exe,args,uploads):
 records=[]; prefix="C:\\u\\r10native"+str(time.time_ns())
 def rpc(argv,data=None,missing=False):
  t=time.monotonic();r=subprocess.run(["utmctl"]+argv,input=data,capture_output=True,timeout=15)
  records.append(dict(argv=argv,rc=r.returncode,stdout=r.stdout.decode(errors="replace"),stderr=r.stderr.decode(errors="replace"),seconds=time.monotonic()-t))
  failed=r.returncode or b"Error from event:" in r.stderr
  if failed and not missing:raise RuntimeError(records[-1])
  return None if failed else r.stdout
 remote=prefix+".exe";out=prefix+".out";err=prefix+".err";rc=prefix+".rc";script=prefix+".ps1"
 try:
  rpc(["file","push",vm,remote],exe.read_bytes());paths={}
  for name,p in uploads.items():
   paths[name]=prefix+"-"+p.name;rpc(["file","push",vm,paths[name]],p.read_bytes())
  guestargs=" ".join(paths.get(a,a) for a in args)
  assert "'" not in remote+guestargs
  ps=("$ErrorActionPreference='Stop'; try { $s=New-Object System.Diagnostics.ProcessStartInfo; "
      +"$s.FileName='"+remote+"'; $s.Arguments='"+guestargs+"'; "
      +"$s.UseShellExecute=$false; $s.RedirectStandardOutput=$true; $s.RedirectStandardError=$true; "
      +"$p=New-Object System.Diagnostics.Process; $p.StartInfo=$s; [void]$p.Start(); "
      +"if(-not $p.WaitForExit(10000)) { $p.Kill(); Set-Content '"+rc+"' 'TIMEOUT' } else { "
      +"Set-Content '"+out+"' $p.StandardOutput.ReadToEnd(); Set-Content '"+err+"' $p.StandardError.ReadToEnd(); Set-Content '"+rc+"' ([string]$p.ExitCode) }; $p.Dispose() "
      +"} catch { Set-Content '"+err+"' ([string]$_); Set-Content '"+rc+"' 'RPC-FAIL' }")
  rpc(["file","push",vm,script],ps.encode())
  rpc(["exec",vm,"--cmd","powershell.exe","-NoProfile","-ExecutionPolicy","Bypass","-File",script])
  deadline=time.monotonic()+20; result=None
  while time.monotonic()<deadline:
   value=rpc(["file","pull",vm,rc],missing=True)
   if value and value.strip():result=value.decode().strip();break
   time.sleep(.25)
  stdout=rpc(["file","pull",vm,out],missing=True);stderr=rpc(["file","pull",vm,err],missing=True)
  return dict(guest_rc=result,stdout=(stdout or b"").decode(errors="replace"),stderr=(stderr or b"").decode(errors="replace"),probe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),uploaded={k:hashlib.sha256(v.read_bytes()).hexdigest() for k,v in uploads.items()},rpc=records)
 except Exception as e:return dict(guest_rc=None,error=str(e),rpc=records)
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("--vm",default="minicon-win-arm-64");p.add_argument("--exe",required=True,type=pathlib.Path);p.add_argument("--dll",type=pathlib.Path);p.add_argument("--package",type=pathlib.Path);p.add_argument("--target");p.add_argument("--input",type=pathlib.Path);p.add_argument("--extra-input",type=pathlib.Path,action="append",default=[]);p.add_argument("--owned-library",type=pathlib.Path,action="append",default=[]);p.add_argument("--evidence",required=True,type=pathlib.Path);a=p.parse_args()
 uploads={};args=[]
 if a.dll or a.package or a.target:
  if not(a.dll and a.package and a.target):p.error("DLL mode needs dll, package, target")
  uploads=dict(dll=a.dll,package=a.package);args=["dll","package",a.target]
  for i,lib in enumerate(a.owned_library):
   key="owned"+str(i);uploads[key]=lib;args.append(key)
 if a.owned_library and not uploads:p.error("owned libraries require DLL mode")
 if a.input:
  if uploads:p.error("input mode cannot be combined with DLL mode")
  uploads=dict(input=a.input);args=["input"]
  for i,path in enumerate(a.extra_input):
   key="input"+str(i);uploads[key]=path;args.append(key)
 if a.extra_input and not a.input:p.error("extra input requires input mode")
 r=run(a.vm,a.exe,args,uploads);a.evidence.write_text(json.dumps(r,indent=2)+"\n");print(json.dumps({k:v for k,v in r.items() if k!="rpc"}));raise SystemExit(0 if r.get("guest_rc")=="0" else 1)
