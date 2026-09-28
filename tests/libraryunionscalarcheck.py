#!/usr/bin/env python3
"""True public fixed union scalar matrix; unsupported actual cases are red."""
import argparse,hashlib,json,os,platform,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def descriptor(k,w,a,uns=0,tag=0,payload=b''):
 return b''.join(U(n) for n in (0,0,0,k,w,uns,a))+bytes([tag])+U(len(payload))+payload
# Independent exact member layout, no C carrier choice/classifier.
CASES={'int8':(0,8,8,[(1,8,1),(1,8,0)]),'float4':(1,4,4,[(3,4,0),(3,4,0)]),'double8':(2,8,8,[(3,8,0),(3,8,0)]),
 'mixed2':(3,8,8,[(3,8,0),(1,8,1)]),'mixed3':(4,8,8,[(3,8,0),(1,8,1),(1,8,0)]),'mixed-reordered':(5,8,8,[(1,8,1),(1,8,0),(3,8,0)]),
 'int1':(6,1,1,[(1,1,1),(1,1,0)]),'int2':(7,2,2,[(1,2,1),(1,2,0)]),'int4':(8,4,4,[(1,4,1),(1,4,0)])}
PRESSURES={'plain':0,'gp9':1,'fp9':2}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',default=str(ROOT/'unisacc.com'));ap.add_argument('--arch',choices=['arm64','x86_64']);ap.add_argument('--cases',default='int8,float4,double8,mixed2,mixed3,mixed-reordered');ap.add_argument('--pressures',default='plain,gp9,fp9');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--build-only',action='store_true');ap.add_argument('--evidence',type=Path);a=ap.parse_args()
 cases=a.cases.split(',');pressures=a.pressures.split(',');assert cases and pressures and all(x in CASES for x in cases) and all(x in PRESSURES for x in pressures)
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if platform.system() not in ('Darwin','Linux') or (arch!=host and platform.system()!='Darwin'):raise RuntimeError('requires matching native platform runner')
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch;flags=['-arch',arch] if platform.system()=='Darwin' else [];package=Path(a.package).resolve(strict=True)
 record={'schema':1,'target':target,'scope':'build-only; no public calls' if a.build_only else 'actual public fixed scalar union matrix; any rejection is failure','package_path':str(package),'package_sha256':sha(package),'sanitize':a.sanitize,'cases':cases,'pressures':pressures,'commands':[],'variants':[],'status':'failed'}
 def finish(rc):
  record['actual_runner_rc']=rc
  if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(record,indent=2)+'\n')
  print(json.dumps({'target':target,'status':record['status'],'actual_runner_rc':rc,'variants':[{'case':v['case'],'pressure':v['pressure'],'rc':v['rc'],'calls_100':[e['opt'] for e in v['events'] if e['stage']=='calls_100']} for v in record['variants']]}));return rc
 with tempfile.TemporaryDirectory(prefix='r10-union-scalar-') as tmp:
  t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir();inputs={}
  for f in sorted((ROOT/'exec/c').iterdir()):
   if f.is_file() and f.suffix in ('.c','.h','.S'):shutil.copy2(f,rt/f.name);inputs[str(f.relative_to(ROOT))]=sha(rt/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',t/'src/host_dl.h');inputs['src/host_dl.h']=sha(t/'src/host_dl.h')
  shutil.copy2(ROOT/'tests/libraryabi/union_scalar.c',t/'probe.c');inputs['tests/libraryabi/union_scalar.c']=sha(t/'probe.c');record['source_closure']=inputs
  frozen=t/'compiler.pkg';shutil.copy2(package,frozen);assert sha(frozen)==record['package_sha256']
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else []
  def run(stage,command):
   cmd=[str(ROOT/'tests/bound'),'20',*map(str,command)];r=subprocess.run(cmd,capture_output=True,text=True,timeout=25);item={'stage':stage,'command':cmd,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr};record['commands'].append(item);return item
  try:
   lib=t/'library.so';r=run('runtime_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',lib])
   if r['rc']:return finish(1)
   record['runtime_sha256']=sha(lib)
   for name in cases:
    case,w,al,members=CASES[name];payload=U(len(members))+b''.join(U(0)*3+U(mw)+descriptor(k,mw,mw,uns) for k,mw,uns in members);union=descriptor(5,w,al,tag=2,payload=payload)
    for pressure in pressures:
     prefix=PRESSURES[pressure];params=([descriptor(1 if prefix==1 else 3,8,8,1 if prefix==1 else 0)]*9 if prefix else [])+[union]
     nm=b'host_step';sig=b'USLSIG2\n'+U(1)+U(len(nm))+nm+bytes([0,1,0,int(len(params)>6)])+U(len(params))+union+U(len(params))+b''.join(params)+bytes([0]);sf=t/'host_step.sig';sf.write_bytes(sig);exe=t/'probe'
     variant={'case':name,'pressure':pressure,'union_extent':w,'union_alignment':al,'signature_hex':sig.hex(),'signature_sha256':sha(sf),'native_member_layout':members,'events':[],'rc':1};record['variants'].append(variant)
     r=run('probe_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-Wall','-Wextra','-I',rt,'-DUNION_CASE='+str(case),'-DUNION_PRESSURE='+str(prefix),t/'probe.c',lib,'-o',exe])
     if r['rc']:return finish(1)
     if a.build_only:variant['rc']=0;continue
     r=run('public_probe',[*(['arch','-x86_64'] if host!=arch else []),exe,frozen,target,sf]);variant['rc']=r['rc']
     for line in r['stdout'].splitlines():
      try:event=json.loads(line)
      except json.JSONDecodeError:continue
      if isinstance(event,dict) and 'stage' in event:variant['events'].append(event)
     complete=[e for e in variant['events'] if e['stage']=='calls_100' and e['rc']==0]
     if r['rc'] or [e['opt'] for e in complete]!=[0,1,2]:return finish(1)
     assert all(e['extent']==w and e['alignment']==al for e in variant['events'])
     assert [e['native_calls'] for e in complete]==[100,200,300]
   record['status']='built_only' if a.build_only else 'passed';return finish(0)
  except Exception as exc:record['exception']=repr(exc);return finish(1)
if __name__=='__main__':raise SystemExit(main())
