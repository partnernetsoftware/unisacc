#!/usr/bin/env python3
"""True public composite/array/callback union ABI; actual rejection is red."""
from checklib import copy_runtime
import argparse,hashlib,json,os,platform,shutil,struct,subprocess,tempfile
from pathlib import Path
import sys as _sys, pathlib as _pl; _sys.path.insert(0, next(str(_p / 'tests') for _p in _pl.Path(__file__).resolve().parents if (_p / 'tests/checklib.py').is_file()))
from checklib import sha
ROOT=Path(__file__).resolve().parents[1]
U=lambda n:struct.pack('<Q',n)
def descriptor(k,w,a,uns=0,tag=0,payload=b''):
 return b''.join(U(n) for n in (0,0,0,k,w,uns,a))+bytes([tag])+U(len(payload))+payload
# Independent exact member/array offsets; no model carrier choice.
CASES={'mixed16':(0,16,8),'fp16':(1,16,8),'array2':(2,8,4),'array4':(3,16,4),'array5':(4,20,4)}
def composite(name):
 D=descriptor(3,8,8);I=descriptor(1,8,8,1);L=descriptor(1,8,8);F=descriptor(3,4,4)
 def union(types,w,a):return descriptor(5,w,a,tag=2,payload=U(len(types))+b''.join(U(0)*3+U(w)+t for t in types))
 if name in ('mixed16','fp16'):
  u=union([D,I] if name=='mixed16' else [D,D],8,8);k=L if name=='mixed16' else D
  return descriptor(5,16,8,tag=1,payload=U(2)+U(0)*3+U(8)+u+U(8)+U(0)*2+U(8)+k)
 n={'array2':2,'array4':4,'array5':5}[name];u=union([F,F],4,4);arr=descriptor(5,4*n,4,tag=3,payload=U(n)+U(4)+u)
 return descriptor(5,4*n,4,tag=1,payload=U(1)+U(0)*3+U(4*n)+arr)
PRESSURES={'plain':0,'gp9':1,'fp9':2}
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--package',default=str(ROOT/'unisacc.com'));ap.add_argument('--arch',choices=['arm64','x86_64']);ap.add_argument('--cases',default='mixed16,fp16,array2,array4,array5');ap.add_argument('--callback',action='store_true');ap.add_argument('--pressures',default='plain,gp9,fp9');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--prepare',action='store_true');ap.add_argument('--require-cached',action='store_true');ap.add_argument('--cache-root',type=Path);ap.add_argument('--build-only',action='store_true');ap.add_argument('--evidence',type=Path);a=ap.parse_args()
 cases=a.cases.split(',');pressures=a.pressures.split(',');assert cases and pressures and all(x in CASES for x in cases) and all(x in PRESSURES for x in pressures)
 host='arm64' if platform.machine() in ('arm64','aarch64') else 'x86_64';arch=a.arch or host
 if (a.prepare or a.require_cached) and (platform.system()!='Linux' or arch!='x86_64'):
  print('UNVERIFIED: required=native-linux-x86_64 observed=%s/%s (prepare/require-cached tool)' % (platform.system(),arch)); raise SystemExit(77)
 if a.prepare and a.require_cached: raise SystemExit('choose --prepare or --require-cached')
 if platform.system() not in ('Darwin','Linux') or (arch!=host and platform.system()!='Darwin'):raise RuntimeError('requires matching native platform runner')
 target=('osx/' if platform.system()=='Darwin' else 'lnx/')+arch;flags=['-arch',arch] if platform.system()=='Darwin' else [];package=Path(a.package).resolve(strict=True)
 import sys as _s2; _s2.path.insert(0,str(ROOT/'tests')); from actorclosure import actor_closure as _actor_closure
 _ffi_cflags=[];_ffi_ldflags=['-lffi']
 _rt=Path(tempfile.mkdtemp(prefix='composite-key-')); _inputs=copy_runtime(ROOT,_rt); shutil.rmtree(_rt, ignore_errors=True)
 _actors,_=_actor_closure(ROOT,['tests/libraryunioncompositecheck.py'])
 _s2.path.insert(0,str(ROOT/'exec/c')); from provenance import source_digest as _sd
 _ccname=os.environ.get('CC','cc'); _cc=Path(shutil.which(_ccname) or _ccname).resolve(strict=True)
 _ccver=subprocess.run([str(_cc),'--version'],env={k:v for k,v in os.environ.items() if not k.startswith('LD_') and k!='GLIBC_TUNABLES'},capture_output=True,text=True,check=True,timeout=5).stdout
 _tool={'cc_env':_ccname,'cc_path':str(_cc),'cc_sha256':sha(_cc),'cc_version':_ccver,'cflags_env':os.environ.get('CFLAGS',''),'environment':{k:os.environ.get(k) for k in ('PATH','CPATH','C_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SOURCE_DATE_EPOCH')}}
 _keydoc={'schema':'composite-lib-v1','package':sha(package),'arch':arch,'sanitize':a.sanitize,'ffi':[_ffi_cflags,_ffi_ldflags],'provenance':_sd(),'actors':dict(sorted(_actors.items())),'runtime_inputs':dict(sorted(_inputs.items())),'tool':_tool}
 COMPOSITE_KEY=hashlib.sha256(json.dumps(_keydoc,sort_keys=True).encode()).hexdigest()
 CACHE_DIR=(a.cache_root or Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))/'composite')/('composite-'+COMPOSITE_KEY)
 def _verify_cache():
  import re as _re
  try: m=json.loads((CACHE_DIR/'manifest.json').read_text())
  except (FileNotFoundError,ValueError): print('NEED_PREPARE composite runtime: manifest absent or unreadable for key '+COMPOSITE_KEY,file=_sys.stderr); raise SystemExit(75)
  if not isinstance(m,dict) or set(m)!={'libcomposite.so'}: print('NEED_PREPARE composite runtime: manifest names %s, expected libcomposite.so' % (sorted(m) if isinstance(m,dict) else type(m).__name__),file=_sys.stderr); raise SystemExit(75)
  h=m['libcomposite.so']; f=CACHE_DIR/'libcomposite.so'
  if not isinstance(h,str) or not _re.fullmatch(r'[0-9a-f]{64}',h): print('NEED_PREPARE composite runtime: bad digest format',file=_sys.stderr); raise SystemExit(75)
  if not f.is_file() or f.is_symlink() or f.stat().st_size==0: print('NEED_PREPARE composite runtime: artifact absent, not regular, or empty',file=_sys.stderr); raise SystemExit(75)
  if sha(f)!=h: print('NEED_PREPARE composite runtime: artifact corrupt',file=_sys.stderr); raise SystemExit(75)
  return f
 if a.prepare:
  try: _verify_cache()
  except SystemExit: pass
  else: print(json.dumps({'status':'HIT','key':COMPOSITE_KEY})); raise SystemExit(0)
 record={'schema':1,'target':target,'scope':'build-only; no public calls' if a.build_only else 'actual public nested composite union matrix; any rejection is failure','package_path':str(package),'package_sha256':sha(package),'sanitize':a.sanitize,'cases':cases,'pressures':pressures,'commands':[],'variants':[],'status':'failed'}
 def finish(rc):
  record['actual_runner_rc']=rc
  if a.evidence:a.evidence.parent.mkdir(parents=True,exist_ok=True);a.evidence.write_text(json.dumps(record,indent=2)+'\n')
  print(json.dumps({'target':target,'status':record['status'],'actual_runner_rc':rc,'variants':[{'case':v['case'],'pressure':v['pressure'],'rc':v['rc'],'calls_100':[e['opt'] for e in v['events'] if e['stage']=='calls_100']} for v in record['variants']]}));return rc
 with tempfile.TemporaryDirectory(prefix='r10-union-composite-') as tmp:
  t=Path(tmp);rt=t/'exec/c';rt.mkdir(parents=True);(t/'src').mkdir();inputs={}
  inputs.update(copy_runtime(ROOT,t))
  shutil.copy2(ROOT/'tests/libraryabi/union_composite.c',t/'probe.c');inputs['tests/libraryabi/union_composite.c']=sha(t/'probe.c');record['source_closure']=inputs
  frozen=t/'compiler.pkg';shutil.copy2(package,frozen);assert sha(frozen)==record['package_sha256']
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitize else []
  def run(stage,command):
   cmd=[str(ROOT/'tests/bound'),'20',*map(str,command)];r=subprocess.run(cmd,capture_output=True,text=True,timeout=25);item={'stage':stage,'command':cmd,'rc':r.returncode,'stdout':r.stdout,'stderr':r.stderr};record['commands'].append(item);return item
  try:
   lib=t/'library.so'
   if a.require_cached:
    shutil.copy2(_verify_cache(),lib); r={'rc':0}
   else: r=run('runtime_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-shared','-fPIC','-fvisibility=hidden',rt/'libunisacc.c',rt/('librarycall_'+arch+'.S'),'-lffi','-o',lib])
   if r['rc']:return finish(1)
   record['runtime_sha256']=sha(lib);record['callback']=a.callback
   if a.prepare:
    CACHE_DIR.mkdir(parents=True,exist_ok=True); staged=CACHE_DIR/'.staged-libcomposite.so'; shutil.copy2(lib,staged); os.replace(staged,CACHE_DIR/'libcomposite.so')
    (CACHE_DIR/'manifest.json').write_text(json.dumps({'libcomposite.so':sha(CACHE_DIR/'libcomposite.so')},sort_keys=True)+'\n')
    record['status']='prepared'; print(json.dumps({'status':'PREPARED','key':COMPOSITE_KEY,'cache':str(CACHE_DIR)})); return finish(0)
   for name in cases:
    case,w,al=CASES[name];union=composite(name)
    for pressure in pressures:
     prefix=PRESSURES[pressure];params=([descriptor(1 if prefix==1 else 3,8,8,1 if prefix==1 else 0)]*9 if prefix else [])
     if a.callback:
      child=bytes([1,0])+U(1)+bytes([0,0])+U(1)+union+U(1)+union+bytes([0])
      callback=b''.join(U(n) for n in (1,0,0,4,8,0,8))+bytes([4])+U(len(child))+child;params.append(callback)
     params.append(union)
     nm=b'host_step';sig=b'USLSIG2\n'+U(1)+U(len(nm))+nm+bytes([0,1,0,int(len(params)>6)])+U(len(params))+union+U(len(params))+b''.join(params)+bytes([0]);sf=t/'host_step.sig';sf.write_bytes(sig);exe=t/'probe'
     variant={'case':name,'pressure':pressure,'union_extent':w,'union_alignment':al,'signature_hex':sig.hex(),'signature_sha256':sha(sf),'callback':a.callback,'events':[],'rc':1};record['variants'].append(variant)
     r=run('probe_build',[os.environ.get('CC','cc'),*flags,*san,'-std=c11','-O2','-Wall','-Wextra','-I',rt,'-DCOMPOSITE_CASE='+str(case),'-DCOMPOSITE_PRESSURE='+str(prefix),'-DCOMPOSITE_CALLBACK='+str(int(a.callback)),t/'probe.c',lib,'-o',exe])
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
     assert [e['callback_calls'] for e in complete]==([100,200,300] if a.callback else [0,0,0])
   record['status']='built_only' if a.build_only else 'passed';return finish(0)
  except Exception as exc:record['exception']=repr(exc);return finish(1)
if __name__=='__main__':raise SystemExit(main())
