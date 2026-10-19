#!/usr/bin/env python3
"""Prepared compiler/model inputs; protocol assertions always execute in consumers."""
import argparse,hashlib,json,os,pathlib,platform,shutil,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT),str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance
from actorclosure import actor_closure
from checklib import cc_read_set_identity,verify_cached_file
from modelnativecarrierimportcheck import dump_source
PARTS=('import','package')
OUTPUTS={'e3.json','e3.tbl','e3.net','nc.json','nc.tbl','nc.net','run','dump'}
SEEDS=['tests/carrierimportprepared.py','tests/modelnativecarrierimportcheck.py','tests/bound','tests/bound.py','tests/build_ref.sh','tests/export_ref.sh','exec/c/provenance.py']
ENV_KEYS=('CC','CFLAGS','PATH','CPATH','C_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SOURCE_DATE_EPOCH','PYTHONPATH','PYTHONHASHSEED','LANG','LC_ALL','LD_LIBRARY_PATH','LD_PRELOAD','LD_AUDIT')
from checklib import sha  # same body as checklib.sha (tests/checklib.py:26)
def need(reason):
 print('NEED_PREPARE carrier import: '+str(reason),file=sys.stderr);raise SystemExit(75)
def identity():
 if os.environ.get('CC') not in (None,'','cc'):need('unverified CC override; this recipe uses system cc')
 cc=shutil.which('cc')
 if not cc:need('system cc absent')
 cc=pathlib.Path(cc).resolve(strict=True)
 actors,edges=actor_closure(ROOT,SEEDS)
 env=dict(os.environ,UA_TYPESPELL='1')
 runtime=cc_read_set_identity(cc,['exec/c/run.c'],['-O2'],cwd=ROOT,env=env)
 # Generated translation-unit names are scratch paths, not semantic inputs.
 # Bind their exact bytes and every actual header; only substitute the one source
 # pathname, and require its hash to agree before substitution.
 with tempfile.TemporaryDirectory(prefix='carrier-import-readset-') as name:
  d=pathlib.Path(name);ref=d/'ref.c';flat=d/'flat.c';src=d/'dump.c'
  p=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'20','sh',str(ROOT/'tests/export_ref.sh'),str(flat)],cwd=ROOT,env=env,capture_output=True)
  if p.returncode:raise ValueError('export metadata failed rc='+str(p.returncode)+': '+p.stderr.decode(errors='replace')[-1000:])
  ref.write_bytes((ROOT/'tests/refshim.h').read_bytes()+flat.read_bytes()+(ROOT/'tests/reffoot.h').read_bytes())
  src.write_bytes(dump_source(ref.read_bytes()))
  scan=cc_read_set_identity(cc,[src],['-w','-std=c99','-O0'],cwd=ROOT,env=env)
  source=scan['files'].pop(str(src));assert source['sha256']==sha(src) and source['resolved']==str(src.resolve())
  assert not any(str(d) in n or str(d) in q['resolved'] for n,q in scan['files'].items())
  # Preserve actual command flags, environment, compiler entity and input bytes.
  command=scan['commands'][0];assert command[-1]==str(src);command[-1]='@generated/dump.c'
  scan['files']['@generated/dump.c']={'resolved':'@generated/dump.c','sha256':source['sha256']}
  generated=dict(export_sha256=sha(ref),dumper_sha256=sha(src),readset=scan)
 return dict(schema='carrier-import-prepared-v1',sources=provenance.source_digest(),actors=actors,actor_edges=edges,
  compiler=str(cc),compiler_sha256=sha(cc),python=str(pathlib.Path(sys.executable).resolve()),python_sha256=sha(pathlib.Path(sys.executable).resolve()),
  runtime_readset=runtime,generated=generated,environment={k:env.get(k) for k in ENV_KEYS},
  stage_environment={k:v for k,v in env.items() if k.startswith(('E1','E2','E3','E4','UNISA_','DYLD_'))},
  UA_TYPESPELL='1',host=[platform.system(),platform.machine()],outputs=sorted(OUTPUTS))
def main():
 if os.name!='posix':
  print('UNVERIFIED carrier import prepared inputs: requires POSIX host');raise SystemExit(77)
 import fcntl
 ap=argparse.ArgumentParser();ap.add_argument('--prepare',action='store_true');ap.add_argument('--require-cached',action='store_true');ap.add_argument('--part',choices=PARTS);a=ap.parse_args()
 if a.prepare==a.require_cached or (a.prepare and a.part) or (a.require_cached and not a.part):ap.error('choose --prepare or --require-cached --part NAME')
 try:basis=identity()
 except (OSError,ValueError,subprocess.TimeoutExpired) as e:need(e)
 key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest()
 base=pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'));cache=base/('carrier-import-'+key)
 def valid():
  try:
   m=cache/'manifest.json';provenance.verify(m);doc=json.loads(m.read_text())
   if doc['basis']!=basis or set(doc['outputs'])!=OUTPUTS:raise ValueError('role/output set mismatch')
   for n,h in doc['outputs'].items():verify_cached_file(cache/n,h,executable=n in ('run','dump'))
   return True
  except (SystemExit,OSError,KeyError,TypeError,ValueError) as e:
   if not a.prepare:need('cache missing, changed or damaged '+key+': '+str(e))
   return False
 if a.prepare:
  base.mkdir(parents=True,exist_ok=True)
  with pathlib.Path(str(cache)+'.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX)
   if valid():print('carrier import prepare HIT '+key);return
   with tempfile.TemporaryDirectory(prefix='carrier-import-prepare-',dir=base) as name:
    work=pathlib.Path(name);p=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'55',sys.executable,str(ROOT/'tests/modelnativecarrierimportcheck.py'),'--prepare',str(work)],cwd=ROOT)
    if p.returncode:raise SystemExit(p.returncode)
    assert identity()==basis
    m=work/'manifest.json';m.write_text(json.dumps(dict(basis=basis,outputs={n:sha(work/n) for n in sorted(OUTPUTS)}),sort_keys=True)+'\n');provenance.write(m,basis['sources'])
    try:cache.stat()
    except FileNotFoundError:pass
    else:shutil.rmtree(cache)
    work.rename(cache)
   assert valid()
  print('carrier import PREPARED '+key);return
 valid() # metadata scan is allowed; no compiler build/model construction on miss.
 p=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'20',sys.executable,str(ROOT/'tests/modelnativecarrierimportcheck.py'),'--cached',str(cache),'--part',a.part],cwd=ROOT)
 raise SystemExit(p.returncode)
if __name__=='__main__':main()
