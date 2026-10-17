#!/usr/bin/env python3
"""Content-bound tools for the controlled pp consumer check; no result cache."""
import argparse, fcntl, hashlib, json, os, platform, shutil, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'exec/c'),str(ROOT/'tests')]
import provenance
from actorclosure import actor_closure
from checklib import cc_read_set_identity, verify_cached_file
TARGETS=('osx/arm64','win/x86_64','win/arm64')
FLAGS={'seed-gen':['-std=c99','-O2','-w'],'bound':['-std=c99','-O2','-Wall','-Wextra']}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def need(s):
    print('NEED_PREPARE seed pp consumer tools: '+str(s),file=sys.stderr);raise SystemExit(75)
def main():
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');p.add_argument('--require-cached',action='store_true');p.add_argument('--target',choices=TARGETS);a=p.parse_args()
    if a.prepare==a.require_cached or (a.prepare and a.target) or (a.require_cached and not a.target):p.error('choose --prepare or --require-cached --target TARGET')
    cc=shutil.which('cc')
    if not cc:need('cc missing')
    cc=Path(cc).resolve()
    # Identity contains the tool entity; no --version subprocess is needed on cache miss.
    seeds=['tests/seedppconsumerprepared.py','tests/seedppconsumercheck.sh','tests/bound','tests/bound.py','exec/c/provenance.py','exec/pipeline/prepare.sh','exec/pp/gen-delta.sh','seed/gen.c']
    actors,edges=actor_closure(ROOT,seeds)
    assert {'tests/actorclosure.py','tests/bound.c'}<=set(actors)
    basis=dict(schema='seed-ppconsumer-tools-v1',sources=provenance.source_digest(),actors=actors,actor_edges=edges,
               compiler=str(cc),compiler_sha256=sha(cc),flags=FLAGS,python=str(Path(sys.executable).resolve()),python_sha256=sha(Path(sys.executable).resolve()),
               seed_inputs={str(q.relative_to(ROOT)):sha(q) for q in sorted((ROOT/'seed').glob('*')) if q.suffix in ('.c','.h')},
               host=[platform.system(),platform.machine()],environment={k:os.environ.get(k) for k in ('PATH','PYTHONPATH','PYTHONHASHSEED','CPATH','C_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','SOURCE_DATE_EPOCH','LANG','LC_ALL','LD_LIBRARY_PATH','LD_PRELOAD','LD_AUDIT')})
    try:
        basis['cc_read_sets']={name:cc_read_set_identity(cc,[source],[*FLAGS[name],*(['-I'+str(ROOT/'seed')] if name=='seed-gen' else [])],cwd=ROOT,env=dict(os.environ))
                               for name,source in [('seed-gen','seed/gen.c'),('bound','tests/bound.c')]}
    except (OSError,ValueError,subprocess.TimeoutExpired) as e:need(e)
    key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest()
    base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'));cache=base/('tools-seed-ppconsumer-'+key)
    def valid():
        try:
            m=cache/'manifest.json';provenance.verify(m);d=json.loads(m.read_text())
            if d['basis']!=basis or set(d['outputs'])!={'seed-gen','bound'}:raise ValueError('role or required set mismatch')
            for n,h in d['outputs'].items():
                q=cache/n
                verify_cached_file(q,h,executable=True)
            return True
        except (SystemExit,FileNotFoundError,KeyError,ValueError,TypeError) as e:
            if not a.prepare:need('该 key 无有效准备（源或 actor 闭包变了，或产物缺坏）: '+key+'; '+str(e))
            return False
    if a.prepare:
        base.mkdir(parents=True,exist_ok=True)
        with Path(str(cache)+'.lock').open('a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX)
            if valid():print('seed pp consumer tools HIT '+key);return
            with tempfile.TemporaryDirectory(prefix='seed-ppconsumer-tools-',dir=base) as td:
                work=Path(td)
                for n,src in [('seed-gen','seed/gen.c'),('bound','tests/bound.c')]:
                    cmd=[sys.executable,str(ROOT/'tests/bound.py'),'55',str(cc),*FLAGS[n]]
                    if n=='seed-gen':cmd+=['-I'+str(ROOT/'seed')]
                    cmd += [str(ROOT/src),'-o',str(work/n)]
                    q=subprocess.run(cmd,cwd=ROOT)
                    if q.returncode:raise SystemExit(q.returncode)
                assert provenance.source_digest()==basis['sources'] and actor_closure(ROOT,seeds)==(actors,edges)
                assert sha(cc)==basis['compiler_sha256']
                assert {name:cc_read_set_identity(cc,[source],[*FLAGS[name],*(['-I'+str(ROOT/'seed')] if name=='seed-gen' else [])],cwd=ROOT,env=dict(os.environ)) for name,source in [('seed-gen','seed/gen.c'),('bound','tests/bound.c')]}==basis['cc_read_sets']
                m=work/'manifest.json';m.write_text(json.dumps(dict(basis=basis,outputs={n:sha(work/n) for n in FLAGS}),sort_keys=True)+'\n');provenance.write(m,basis['sources'])
                try:cache.stat()
                except FileNotFoundError:pass
                else:shutil.rmtree(cache)
                work.rename(cache)
            assert valid()
        print('seed pp consumer tools PREPARED '+key);return
    valid() # No subprocess/build before complete identity and artifact validation.
    env=dict(os.environ,SEEDPPC_PREPARED_TOOLS=str(cache))
    q=subprocess.run([sys.executable,str(ROOT/'tests/bound.py'),'20','sh',str(ROOT/'tests/seedppconsumercheck.sh'),'2',a.target],cwd=ROOT,env=env)
    raise SystemExit(q.returncode)
if __name__=='__main__':main()
