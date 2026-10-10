#!/usr/bin/env python3
"""True public composite/array/callback union ABI; actual rejection is red."""
import argparse,hashlib,json,os,platform,shlex,shutil,struct,subprocess,tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def explicit_files(flags):
 result={}
 for flag in flags:
  if flag.startswith('-'):continue
  p=Path(flag)
  try:p.stat()
  except FileNotFoundError:continue
  result[flag]=sha(p)
 return result
def descriptor(k,w,a,uns=0,tag=0,payload=b''):
 return b''.join(U(n) for n in (0,0,0,k,w,uns,a))+bytes([tag])+U(len(payload))+payload
# Independent exact member/array offsets; no model carrier choice.
CASES={n:(i,16,8 if i<4 or i==8 else 4) for i,n in enumerate(('is8','si8','ss8','ii8','is4','si4','ss4','ii4','fpmix'))}
def composite(name):
 D=descriptor(3,8,8);I=descriptor(1,8,8,1);F=descriptor(3,4,4);J=descriptor(1,4,4,1)
 def obj(types):
  offset=0;fields=[];alignment=max(a for t,w,a in types)
  for t,w,a in types:
   offset=(offset+a-1)//a*a;fields.append(U(offset)+U(0)*2+U(w)+t);offset+=w
  width=(offset+alignment-1)//alignment*alignment
  return descriptor(5,width,alignment,tag=1,payload=U(len(fields))+b''.join(fields))
 def st(kinds):return obj([(t,8 if t in (D,I) else 4,8 if t in (D,I) else 4) for t in kinds])
 def array(t,n,w,a):return descriptor(5,n*w,a,tag=3,payload=U(n)+U(w)+t)
 pairs={'is8':[st([I,D]),st([I,F,F])],'si8':[st([D,I]),st([F,F,I])],'ss8':[st([D,D]),array(D,2,8,8)],'ii8':[st([I,D]),st([D,I])],'is4':[st([J,J,F,F]),st([J,J,F,F])],'si4':[st([F,F,J,J]),st([F,F,J,J])],'ss4':[st([F,F,F,F]),array(F,4,4,4)],'ii4':[st([F,F,F,F]),st([J,J,J,J])],'fpmix':[array(D,2,8,8),array(F,4,4,4)]}
 return descriptor(5,16,CASES[name][2],tag=2,payload=U(2)+b''.join(U(0)*3+U(16)+t for t in pairs[name]))
PRESSURES={'plain':(0,0,0),'gp5fp7':(1,5,7),'gp6fp8':(2,6,8),'gp7fp7':(3,7,7)}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',default=str(ROOT/'unisacc.com'));ap.add_argument('--arch',choices=['arm64','x86_64']);ap.add_argument('--cases',default='is8,si8,ss8,ii8,is4,si4,ss4,ii4,fpmix');ap.add_argument('--callback',action='store_true');ap.add_argument('--pressures',default='plain,gp5fp7,gp6fp8,gp7fp7');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--build-only',action='store_true');ap.add_argument('--evidence',type=Path);ap.add_argument('--ffi-provider',type=Path);ap.add_argument('--ffi-cflags',default=os.environ.get('UNISACC_FFI_CFLAGS',''));ap.add_argument('--ffi-ldflags',default=os.environ.get('UNISACC_FFI_LDFLAGS','-lffi'));a=ap.parse_args()
 cases=a.cases.split(',');pressures=a.pressures.split(',');assert cases and pressures and all(x in CASES for x in cases) and all(x in PRESSURES for x in pressures)
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if platform.system() not in ('Darwin','Linux') or (arch!=host and platform.system()!='Darwin'):raise RuntimeError('requires matching native platform runner')
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch;flags=['-arch',arch] if platform.system()=='Darwin' else [];package=Path(a.package).resolve(strict=True)
 ffi_cflags=shlex.split(a.ffi_cflags);ffi_ldflags=shlex.split(a.ffi_ldflags)
 provider=a.ffi_provider or os.environ.get('UNISACC_FFI_X86_PROVIDER' if arch=='x86_64' else 'UNISACC_FFI_PROVIDER')
 if provider:
  if ffi_cflags or ffi_ldflags!=['-lffi']:raise ValueError('provider cannot mix explicit FFI flags')
  import sys
  sys.path.insert(0,str(ROOT/'exec/c'))
  from buildlibrary import provider_flags
  ffi_cflags,ffi_ldflags,provider_manifest=provider_flags(provider,target)
 record={'ffi_provider_manifest':provider_manifest if provider else None,'ffi_compile_flags':ffi_cflags,'ffi_link_flags':ffi_ldflags,'ffi_explicit_files':explicit_files(ffi_ldflags),'schema':1,'target':target,'scope':'build-only; no public calls' if a.build_only else 'actual public natural union16 matrix; any rejection is failure','package_path':str(package),'package_sha256':sha(package),'sanitize':a.sanitize,'cases':cases,'pressures':pressures,'commands':[],'variants':[],'status':'failed'}
 def finish(rc):
  record['actual_runner_rc']=rc
  if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(record,indent=2)+'\n')
  print(json.dumps({'target':target,'status':record['status'],'actual_runner_rc':rc,'variants':[{'case':v['case'],'pressure':v['pressure'],'rc':v['rc'],'calls_100':[e['opt'] for e in v['events'] if e['stage']=='calls_100']} for v in record['variants']]}));return rc
 with tempfile.TemporaryDirectory(prefix='r10-union16-') as tmp:
  t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir();inputs={}
  for f in sorted((ROOT/'exec/c').iterdir()):
   if f.is_file() and f.suffix in ('.c','.h','.S'):shutil.copy2(f,rt/f.name);inputs[str(f.relative_to(ROOT))]=sha(rt/f.name)
  shutil.copy2(ROOT/'src/host_dl.h',t/'src/host_dl.h');inputs['src/host_dl.h']=sha(t/'src/host_dl.h')
  shutil.copy2(ROOT/'tests/libraryabi/union16.c',t/'probe.c');inputs['tests/libraryunion16check.py']=sha(Path(__file__));inputs['tests/libraryabi/union16.c']=sha(t/'probe.c');record['source_closure']=inputs
  frozen=t/'compiler.pkg';shutil.copy2(package,frozen);assert sha(frozen)==record['package_sha256']
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else []
  def run(stage,command):
   cmd=[str(ROOT/'tests/bound'),'20',*map(str,command)]
   try: r=subprocess.run(cmd,capture_output=True,text=True,timeout=25)
   except subprocess.TimeoutExpired: r=subprocess.CompletedProcess(cmd,142,'','timeout')   # 0.0.38: a timed-out stage is a timeout
   item={'stage':stage,'command':cmd,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr};record['commands'].append(item);return item
  try:
   lib=t/'library.so';r=run('runtime_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',*ffi_cflags,rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),*ffi_ldflags,'-o',lib])
   if r['rc']:return finish(142 if r['rc']==142 else 1)   # 0.0.38 (董秘 10-10): a time cut exits 142, not an ordinary failure
   record['runtime_sha256']=sha(lib);record['callback']=a.callback
   for name in cases:
    case,w,al=CASES[name];union=composite(name)
    for pressure in pressures:
     prefix,gp,fp=PRESSURES[pressure];params=[descriptor(1,8,8,1)]*gp+[descriptor(3,8,8)]*fp
     if a.callback:
      child=bytes([1,0])+U(1)+bytes([0,0])+U(1)+union+U(1)+union+bytes([0])
      callback=b''.join(U(n) for n in (1,0,0,4,8,0,8))+bytes([4])+U(len(child))+child;params.append(callback)
     params.extend([union,descriptor(1,8,8,1),descriptor(3,8,8)])
     nm=b'host_step';sig=b'USLSIG2\n'+U(1)+U(len(nm))+nm+bytes([0,1,0,int(len(params)>6)])+U(len(params))+union+U(len(params))+b''.join(params)+bytes([0]);sf=t/'host_step.sig';sf.write_bytes(sig);exe=t/'probe'
     variant={'case':name,'pressure':pressure,'union_extent':w,'union_alignment':al,'gp_prefix':gp,'fp_prefix':fp,'tail_types':['u64','double'],'signature_hex':sig.hex(),'signature_sha256':sha(sf),'callback':a.callback,'events':[],'rc':1};record['variants'].append(variant)
     r=run('probe_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c99','-O2','-Wall','-Wextra','-I',rt,'-DUNION16_CASE='+str(case),'-DUNION16_PRESSURE='+str(prefix),'-DUNION16_CALLBACK='+str(int(a.callback)),t/'probe.c',lib,'-o',exe])
     if r['rc']:return finish(142 if r['rc']==142 else 1)
     if a.build_only:variant['rc']=0;continue
     r=run('public_probe',[*(['arch','-x86_64'] if host!=arch else []),exe,frozen,target,sf]);variant['rc']=r['rc']
     for line in r['stdout'].splitlines():
      try:event=json.loads(line)
      except json.JSONDecodeError:continue
      if isinstance(event,dict) and 'stage' in event:variant['events'].append(event)
     complete=[e for e in variant['events'] if e['stage']=='calls_100' and e['rc']==0]
     if r['rc']==142:return finish(142)
     if r['rc'] or [e['opt'] for e in complete]!=[0,1,2]:return finish(1)
     assert all(e['extent']==w and e['alignment']==al for e in variant['events'])
     assert [e['native_calls'] for e in complete]==[100,200,300]
     assert [e['callback_calls'] for e in complete]==([100,200,300] if a.callback else [0,0,0])
   record['status']='built_only' if a.build_only else 'passed';return finish(0)
  except Exception as exc:record['exception']=repr(exc);return finish(1)
if __name__=='__main__':raise SystemExit(main())
