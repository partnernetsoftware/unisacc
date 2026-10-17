#!/usr/bin/env python3
"""Prepared constructors/graphs; original matrix comparisons remain consumers."""
import argparse, fcntl, hashlib, json, os, platform, shutil, sys, tempfile, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'exec/c'), str(ROOT/'tests')]
import provenance
from actorclosure import actor_closure
from checklib import verify_cached_file
from seedconstructmatrix import SHARED, run

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def need(reason):
    print('NEED_PREPARE seed matrix: '+str(reason), file=sys.stderr)
    raise SystemExit(75)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--prepare',action='store_true')
    p.add_argument('--require-cached',action='store_true')
    p.add_argument('--unit',choices=[s[0] for s in SHARED])
    a=p.parse_args()
    if a.prepare == a.require_cached or (a.prepare and a.unit) or (a.require_cached and not a.unit):
        p.error('choose --prepare or --require-cached --unit NAME')
    product=ROOT/'unisacc.com'
    try:
        product_record=provenance.verify(product)
    except SystemExit as e: need(e)
    seeds=['tests/seedmatrixprepared.py','tests/seedconstructmatrix.py','tests/bound','tests/bound.py','exec/c/provenance.py','seed/net.c','seed/tbl.c']
    actors,edges=actor_closure(ROOT,seeds)
    assert {'tests/actorclosure.py','tests/bound.c'} <= set(actors)
    basis=dict(schema='seed-matrix-shared-v1',sources=provenance.source_digest(),actors=actors,actor_edges=edges,
               product_sha256=sha(product),product_record=product_record,
               python=str(Path(sys.executable).resolve()),python_sha256=sha(Path(sys.executable).resolve()),
               host=[platform.system(),platform.machine()],specs=SHARED,flags=['-fno-trim-libc'],
               environment={k:os.environ.get(k) for k in ('PATH','PYTHONPATH','PYTHONHASHSEED','LANG','LC_ALL','LD_LIBRARY_PATH','LD_PRELOAD','LD_AUDIT','UNISA_ABLATE','SOURCE_DATE_EPOCH')})
    # JSON normalization keeps tuples/lists identical across manifest readback.
    basis=json.loads(json.dumps(basis,sort_keys=True))
    key=hashlib.sha256(json.dumps(basis,sort_keys=True).encode()).hexdigest()
    base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
    cache=base/('seed-matrix-'+key)
    required={'seed-net','seed-tbl',*(n+'.json' for n,_,_ in SHARED)}
    def valid():
        try:
            m=cache/'manifest.json';provenance.verify(m)
            doc=json.loads(m.read_text())
            if doc['basis'] != basis or set(doc['outputs']) != required: raise ValueError('role/output set mismatch')
            for n,h in doc['outputs'].items():
                f=cache/n
                verify_cached_file(f,h,executable=n in ('seed-net','seed-tbl'))
            return True
        except (SystemExit,FileNotFoundError,ValueError,KeyError,TypeError) as e:
            if not a.prepare: need('该 key 无有效准备（源或 actor 闭包变了，或产物缺坏）: '+key+'; '+str(e))
            return False
    if a.prepare:
        base.mkdir(parents=True,exist_ok=True)
        with Path(str(cache)+'.lock').open('a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX)
            if valid(): print('seed matrix prepare HIT '+key);return
            with tempfile.TemporaryDirectory(prefix='seed-matrix-prepare-',dir=base) as td:
                work=Path(td);deadline=time.monotonic()+55
                for name in ('net','tbl'):
                    run(deadline,'sh',str(product),'-fno-trim-libc','seed/'+name+'.c','-o',str(work/('seed-'+name)))
                for name,generator,flags in SHARED:
                    run(deadline,sys.executable,generator,str(work/(name+'.json')),*flags)
                assert provenance.source_digest()==basis['sources']
                assert actor_closure(ROOT,seeds)==(actors,edges)
                assert sha(product)==basis['product_sha256']
                m=work/'manifest.json'
                m.write_text(json.dumps(dict(basis=basis,outputs={n:sha(work/n) for n in sorted(required)}),sort_keys=True)+'\n')
                provenance.write(m,basis['sources'])
                try: cache.stat()
                except FileNotFoundError: pass
                else: shutil.rmtree(cache)
                work.rename(cache)
            assert valid()
            print('seed matrix PREPARED '+key)
        return
    valid() # All validation precedes any subprocess; no construction on miss/corruption.
    name=a.unit
    with tempfile.TemporaryDirectory(prefix='seed-matrix-consume-') as td:
        d=Path(td);deadline=time.monotonic()+20
        graph=cache/(name+'.json');table=d/(name+'.tbl');ctable=d/(name+'.c.tbl')
        py=d/(name+'.py.net');c=d/(name+'.c.net')
        run(deadline,sys.executable,'exec/c/tbl.py',str(graph),str(table))
        run(deadline,str(cache/'seed-tbl'),str(graph),str(ctable))
        if table.read_bytes()!=ctable.read_bytes():
            raise AssertionError('shared/'+name+': seed/tbl.c table differs from exec/c/tbl.py')
        print('SEED_MATRIX_UNIT '+json.dumps([name,'table-exact']),flush=True)
        run(deadline,sys.executable,'exec/c/net.py',str(table),str(py))
        run(deadline,str(cache/'seed-net'),str(table),str(c))
        if py.read_bytes()!=c.read_bytes(): raise AssertionError('shared/'+name+': first differing network byte')
        print('SEED_MATRIX_UNIT '+json.dumps([name,'network-exact']),flush=True)
        print('seed matrix shared/'+name+': byte-identical tables and networks; '+str(c.stat().st_size))
if __name__=='__main__': main()
