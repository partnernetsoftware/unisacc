#!/usr/bin/env python3
"""Prepared independent references; real chain helper compilation stays under test."""
import argparse,fcntl,hashlib,json,os,platform,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance
from actorclosure import actor_closure
from checklib import cc_read_set_identity,verify_cached_file
PARTS=('default','inherited')
RECIPES={'seed-gen':('seed/gen.c',['-std=c99','-O2','-w','-I'+str(ROOT/'seed')]),
         'bound':('tests/bound.c',['-std=c99','-O2','-Wall','-Wextra']),
         'run':('exec/c/run.c',['-O2','-std=c99','-w'])}
REFS=('e2-c-ref.json','e2-py.json','e3-c-ref.json')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def need(reason):
    print('NEED_PREPARE seed pp chain: '+str(reason),file=sys.stderr);raise SystemExit(75)
def main():
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');p.add_argument('--require-cached',action='store_true');p.add_argument('--part',choices=PARTS);a=p.parse_args()
    if a.prepare==a.require_cached or (a.prepare and a.part) or (a.require_cached and not a.part):p.error('choose --prepare or --require-cached --part NAME')
    cc=shutil.which('cc');ua=Path(os.environ.get('UA','/tmp/ua_ref')).resolve()
    if not cc:need('system cc missing')
    cc=Path(cc).resolve()
    if os.environ.get('CC') not in (None,'','cc'):need('unverified watchdog CC override; use the keyed system cc')
    try:
        # Never manufacture an attestation for an arbitrary old UA.
        ua_record=provenance.verify(ua);verify_cached_file(ua,ua_record['artifact_sha256'],executable=True)
        actors,edges=actor_closure(ROOT,['tests/seedppchainprepared.py','tests/seedppchaincheck.sh','tests/bound','tests/bound.py','tests/lib.sh','tests/knownfail.py','exec/c/provenance.py','exec/c/chain.sh','exec/pp/gen-delta.sh','exec/parse2gen/gen-delta.sh','tests/c/a_char.c'])
        reads={n:cc_read_set_identity(cc,[src],flags,cwd=ROOT,env=dict(os.environ)) for n,(src,flags) in RECIPES.items()}
    except (SystemExit,OSError,ValueError,KeyError,subprocess.TimeoutExpired) as e:need(e)
    assert {'tests/bound.c','tests/actorclosure.py','tests/checklib.py'}<=set(actors)
    basis=dict(schema='seed-ppchain-reference-v1',sources=provenance.source_digest(),actors=actors,actor_edges=edges,
               cc_read_sets=reads,recipes=RECIPES,compiler=str(cc),compiler_sha256=sha(cc),
               ua=str(ua),ua_sha256=sha(ua),ua_record=ua_record,
               python=str(Path(sys.executable).resolve()),python_sha256=sha(Path(sys.executable).resolve()),
               host=[platform.system(),platform.machine()],environment={k:os.environ.get(k) for k in ('CC','PATH','CPATH','C_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SOURCE_DATE_EPOCH','PYTHONPATH','PYTHONHASHSEED','LANG','LC_ALL','LD_LIBRARY_PATH','LD_PRELOAD','LD_AUDIT')})
    basis['stage_environment']={k:v for k,v in os.environ.items() if k.startswith(('E1','E2','E3','E4','UNISA_','DYLD_'))}
    basis=json.loads(json.dumps(basis,sort_keys=True));key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest()
    base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'));cache=base/('seed-ppchain-'+key);required=set(RECIPES)|set(REFS)
    def valid():
        try:
            m=cache/'manifest.json';provenance.verify(m);doc=json.loads(m.read_text())
            if doc['basis']!=basis or set(doc['outputs'])!=required:raise ValueError('role or required set mismatch')
            for n,h in doc['outputs'].items():verify_cached_file(cache/n,h,executable=n in RECIPES)
            return True
        except (SystemExit,OSError,ValueError,KeyError,TypeError) as e:
            if not a.prepare:need('该 key 无有效准备（源或 actor 闭包变了，或产物缺坏）: '+key+'; '+str(e))
            return False
    if a.prepare:
        base.mkdir(parents=True,exist_ok=True)
        with Path(str(cache)+'.lock').open('a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX)
            if valid():print('seed pp chain reference HIT '+key);return
            with tempfile.TemporaryDirectory(prefix='seed-ppchain-prepare-',dir=base) as td:
                work=Path(td)
                def step(label,cmd):
                    start=time.monotonic();log=work/(label+'.log')
                    with log.open('wb') as out:q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'55',*map(str,cmd)],cwd=ROOT,stdout=out,stderr=subprocess.STDOUT)
                    row=dict(label=label,argv=list(map(str,cmd)),rc=q.returncode,start_monotonic=start,seconds=time.monotonic()-start,log_sha256=sha(log))
                    with (work/'steps.jsonl').open('a') as out:out.write(json.dumps(row)+'\n')
                    print('SEED_PREP_STEP '+json.dumps(row),flush=True)
                    if q.returncode:sys.stdout.buffer.write(log.read_bytes());raise SystemExit(q.returncode)
                for n,(src,flags) in RECIPES.items():step('build-'+n,[cc,*flags,ROOT/src,'-o',work/n])
                step('reference-e2-c',[work/'seed-gen','pp',work/'e2-c-ref.json'])
                step('reference-e2-python',[sys.executable,'exec/build/gen.py','pp',work/'e2-py.json'])
                step('reference-e3-c',[work/'seed-gen','parse2',work/'e3-c-ref.json'])
                assert provenance.source_digest()==basis['sources'] and actor_closure(ROOT,list(actors))==(actors,edges)
                assert sha(ua)==basis['ua_sha256']
                assert {n:cc_read_set_identity(cc,[src],flags,cwd=ROOT,env=dict(os.environ)) for n,(src,flags) in RECIPES.items()}==reads
                m=work/'manifest.json';m.write_text(json.dumps(dict(basis=basis,outputs={n:sha(work/n) for n in sorted(required)}),sort_keys=True)+'\n');provenance.write(m,basis['sources'])
                try:cache.stat()
                except FileNotFoundError:pass
                else:shutil.rmtree(cache)
                work.rename(cache)
            assert valid()
        print('seed pp chain reference PREPARED '+key);return
    valid()
    env=dict(os.environ,SEEDPPCH_PREPARED=str(cache),SEEDPPCH_UA=str(ua))
    q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'20','sh',str(ROOT/'tests/seedppchaincheck.sh'),'1',a.part],cwd=ROOT,env=env)
    raise SystemExit(q.returncode)
if __name__=='__main__':main()
