#!/usr/bin/env python3
"""Public union chain acceptance. Current rejection is a failure, never green."""
import argparse, hashlib, json, pathlib, platform, shutil, struct, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',default=str(ROOT/'unisacc.com'));ap.add_argument('--arch',choices=['arm64','x86_64']);ap.add_argument('--evidence',type=pathlib.Path);ap.add_argument('--export-only',action='store_true');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--factory',action='store_true');ap.add_argument('--relay',action='store_true');a=ap.parse_args();assert sum([a.factory,a.export_only,a.relay])<=1
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if arch!=host and platform.system()!='Darwin':raise RuntimeError('cross-native execution requires platform runner')
 package=pathlib.Path(a.package).resolve();target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch
 record={'schema':1,'scope':'actual public native->script union->typed native union; rejection is failure','target':target,'mode':'script-relay' if a.relay else 'native-factory' if a.factory else 'export-only' if a.export_only else 'bidirectional','native_or_emulated':'Rosetta' if host!=arch else 'native','package_path':str(package),'package_sha256':sha(package),'commands':[],'events':[],'status':'failed'}
 with tempfile.TemporaryDirectory(prefix='r10-union-native-') as name:
  td=pathlib.Path(name);runtime=td/'exec/c';runtime.mkdir(parents=True);(td/'src').mkdir();inputs={}
  for p in sorted((ROOT/'exec/c').iterdir()):
   if p.is_file() and p.suffix in ('.h','.c','.S'):shutil.copy2(p,runtime/p.name);inputs[str(p.relative_to(ROOT))]=sha(runtime/p.name)
  shutil.copy2(ROOT/'src/host_dl.h',td/'src/host_dl.h');inputs['src/host_dl.h']=sha(td/'src/host_dl.h')
  shutil.copy2(ROOT/'tests/libraryabi/union_native.c',td/'probe.c');inputs['tests/libraryabi/union_native.c']=sha(td/'probe.c')
  frozen=td/'models.pkg';shutil.copy2(package,frozen);assert sha(frozen)==record['package_sha256'],'package changed while snapshotting'
  record['runtime_inputs']=inputs;flags=['-arch',arch] if platform.system()=='Darwin' else []
  sanitizers=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else [];record['sanitize']=a.sanitize
  def run(stage,args):
   cmd=[str(ROOT/'tests/bound'),'20',*map(str,args)];p=subprocess.run(cmd,capture_output=True,timeout=25)
   item={'stage':stage,'command':cmd,'rc':p.returncode,'stdout':p.stdout.decode(errors='replace'),'stderr':p.stderr.decode(errors='replace')};record['commands'].append(item);return item
  try:
   lib=td/'library.dylib';item=run('runtime_build',['cc',*flags,*sanitizers,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',runtime/'libunisacc.c',runtime/('librarycall_'+arch+'.S'),'-lffi','-o',lib])
   if item['rc']:return finish(record,a.evidence,item['rc'])
   record['runtime_sha256']=sha(lib);exe=td/'probe';item=run('probe_build',['cc',*flags,*sanitizers,'-std=c11','-O2','-Wall','-Wextra','-I',runtime,td/'probe.c',lib,'-o',exe])
   if item['rc']:return finish(record,a.evidence,item['rc'])
   u=lambda n:struct.pack('<Q',n)
   def desc(kind,width,align,uns=0,tag=0,payload=b''):
    return b''.join(u(n) for n in (0,0,0,kind,width,uns,align))+bytes([tag])+u(len(payload))+payload
   D=desc(3,8,8);U64=desc(1,8,8,uns=1)
   union=desc(5,8,8,tag=2,payload=u(2)+u(0)+u(0)+u(0)+u(8)+D+u(0)+u(0)+u(0)+u(8)+U64)
   nm=b'host_flip';sig=b'USLSIG2\n'+u(1)+u(len(nm))+nm+bytes([0,1,0,0])+u(1)+union+u(1)+union+bytes([0]);signature=td/'host_flip.sig';signature.write_bytes(sig)
   if a.factory:
    child=bytes([1,0])+u(1)+bytes([0,0])+u(1)+union+u(1)+union+bytes([0])
    pointer=b''.join(u(n) for n in (1,0,0,4,8,0,8))+bytes([4])+u(len(child))+child
    nm=b'host_pick';sig=b'USLSIG2\n'+u(1)+u(len(nm))+nm+bytes([0,1,0,0])+u(0)+pointer+u(0)+bytes([0]);signature.write_bytes(sig)
   if a.relay:
    child=bytes([1,0])+u(1)+bytes([0,0])+u(1)+union+u(1)+union+bytes([0]);pointer=b''.join(u(n) for n in (1,0,0,4,8,0,8))+bytes([4])+u(len(child))+child
    ref=bytes([1,1])+u(1);param=b''.join(u(n) for n in (1,0,0,4,8,0,8))+bytes([4])+u(len(ref))+ref
    nm=b'host_echo';sig=b'USLSIG2\n'+u(1)+u(len(nm))+nm+bytes([0,1,0,0])+u(1)+pointer+u(1)+param+bytes([0]);signature.write_bytes(sig)
   record['signature_sha256']=sha(signature);record['signature_hex']=sig.hex();record['declaration']={'signature':'callback parameter and result' if a.relay else 'callback result' if a.factory else 'union parameter and result','union_tag':2,'union_size':8,'union_alignment':8,'members':['double','unsigned long long'],'supported':0,'callback_support':0 if a.factory or a.relay else None}
   item=run('public_probe',[*(['arch','-x86_64'] if host!=arch and platform.system()=='Darwin' else []),exe,frozen,target,signature,*(['relay'] if a.relay else ['factory'] if a.factory else ['export-only'] if a.export_only else [])])
   for line in item['stdout'].splitlines():
    try:event=json.loads(line)
    except json.JSONDecodeError:continue
    if isinstance(event,dict) and 'stage' in event:record['events'].append(event)
   failed=next((e for e in record['events'] if e['rc']),None);record['first_failure']=failed
   if failed and failed['stage']=='compile' and any(e['stage']=='unused_binding_compile' and e['rc']==0 for e in record['events']):
    record['localization']='registration and unused-binding resolver freeze/compile passed; referenced union fails during model compilation; exact diagnostic retained'
   rc=item['rc'];complete=[e['opt'] for e in record['events'] if e['stage']=='calls_100' and e['rc']==0]
   if rc==0 and complete!=[0,1,2]:rc=1;record['acceptance_error']='missing mandatory O0/O1/O2 actual calls'
   record['status']='passed' if rc==0 else 'failed';return finish(record,a.evidence,rc)
  except Exception as e:
   record['exception']=repr(e);return finish(record,a.evidence,1)
def finish(record,path,rc):
 record['actual_runner_rc']=rc
 if path:path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(record,indent=2)+'\n')
 print(json.dumps(record,indent=2));return 0 if rc==0 else 1
if __name__=='__main__':raise SystemExit(main())
