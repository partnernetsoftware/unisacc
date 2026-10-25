#!/usr/bin/env python3
"""One seed evidence step. Existing tests/bound.py owns timeout/descendant cleanup.
Call once per route/backend; aggregate only matching source and successful records.
Linux-only RSS: direct constructor wait4, including pre-exec residency (never subtract).
"""
import argparse, hashlib, importlib.util, json, os, pathlib, re, stat, subprocess, sys, time

def digest(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''):h.update(b)
    return h.hexdigest()

def source_hash(root):
    # Reuse the shared reseal identity: source freezing is common, keys remain family-specific.
    spec=importlib.util.spec_from_file_location('model_reseal',root/'release/tools/model-reseal.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module.source_hash(root)

def worker(argv,record):
    start=time.monotonic();pid=os.fork()
    if pid==0:os.execvp(argv[0],argv)
    _,status,usage=os.wait4(pid,0)
    rc=os.waitstatus_to_exitcode(status)
    pathlib.Path(record).write_text(json.dumps({'rc':rc,'rss_kib':usage.ru_maxrss,'seconds':time.monotonic()-start,'scope':'direct constructor; includes pre-exec residency'},indent=2)+'\n')
    return rc if rc>=0 else 128-rc

def write_step(out,key,record,argv):
    # Same seven columns as reseal recipes. argv JSON is one TSV cell.
    cols=['step','argv','rc','monotonic','product_sha','source_hash','artifact_sha']
    row=[key,json.dumps(argv,separators=(',',':')),str(record.get('rc',2)),str(record.get('seconds',0)),record.get('product_sha256','-'),record.get('source_hash','-'),record.get('model_sha256','-')]
    (out/(key+'.tsv')).write_text('\t'.join(cols)+'\n'+'\t'.join(row)+'\n')

def summarize(argv):
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=pathlib.Path,required=True);ap.add_argument('--out',type=pathlib.Path,required=True)
    a=ap.parse_args(argv);root=a.root.resolve(strict=True);out=a.out.resolve(strict=True)
    # Only this producer's derived drafts: never expose stale constants after a failed re-summary.
    subprocess.run(['sh',str(pathlib.Path(__file__).resolve().parent/'cxbin'),str(pathlib.Path(__file__).resolve().parent/'safedel.cx'),'--purge','--',*(str(out/name) for name in ('seedmemory-reads.tsv','seedmemory-constants.json','seedmemory-constants.py.txt'))],check=True)
    (out/'summary.json').write_text(json.dumps({'status':'INCOMPLETE','reason':'aggregation in progress'})+'\n')
    spec=importlib.util.spec_from_file_location('seedmemory_consumer',root/'tests/seedmemory.py');consumer=importlib.util.module_from_spec(spec);spec.loader.exec_module(consumer)
    records=[json.loads(p.read_text()) for p in sorted(out.glob('*.result.json'))];source=digest(root/'seed/gen.c')
    shell=(root/'tests/seedgencheck.sh').read_text();gen_routes={name:args.split() for name,args in re.findall(r'([a-z0-9-]+)\) echo \"?([^\";]+)',shell)}
    frozen=source_hash(root);selected={};reads={f:{} for f in ('gen','parse2')};rss={};missing=[]
    for r in records:
        if r.get('source_sha256')!=source or r.get('source_hash')!=frozen:raise ValueError('source identity differs')
        key=(r.get('family'),r.get('name'),r.get('backend'),r.get('kind'))
        if key in selected:raise ValueError('duplicate observation '+str(key))
        if r.get('status')=='OBSERVED' and r.get('rc')==0:
            model_path=pathlib.Path(r['model_path']).resolve(strict=True);model_path.relative_to(out)
            if digest(model_path)!=r['model_sha256']:raise ValueError('artifact changed '+str(key))
            if r.get('backend')=='c' and digest(pathlib.Path(r['binary_path']))!=r['product_sha256']:raise ValueError('binary identity changed')
        selected[key]=r
    for family in ('gen','parse2'):
        for name in consumer.DEFAULTS[family]:
            for backend,kind in [('c','trace'),('python','trace'),('c','rss')]:
                key=(family,name,backend,kind);r=selected.get(key)
                expected=(gen_routes[name] if family=='gen' else ['parse2']+([] if name=='x' else ['--'+flag for flag in name.split(',')]))
                if r and r.get('route')!=expected:raise ValueError('route label/argv mismatch '+str(key))
                if not r or r.get('status')!='OBSERVED' or r.get('rc')!=0:
                    missing.append({'key':key,'status':r.get('status','MISSING') if r else 'MISSING','rc':r.get('rc') if r else None});continue
                if kind=='trace':
                    for row in r['read_bytes']:
                        path=row['path'];actual=digest(root/path)
                        if actual!=row['sha256']:raise ValueError('read bytes changed '+path)
                        reads[family][path]=actual
                else:
                    if r['rss']['rc']!=0:raise ValueError('nonzero direct RSS')
                    route=({'x':'e3','errors':'errorparse'}.get(name,name) if family=='parse2' else name)
                    value=r['rss']['rss_kib']//1024
                    rss[route]=max(value,rss.get(route,0))
            group=[selected.get((family,name,b,k)) for b,k in [('c','trace'),('python','trace'),('c','rss')]]
            if all(r and r.get('rc')==0 and r.get('status')=='OBSERVED' for r in group):
                if len({r['model_sha256'] for r in group})!=1:raise ValueError('C/P/direct output mismatch '+name)
    if missing:
        (out/'summary.json').write_text(json.dumps({'status':'INCOMPLETE','missing':missing},indent=2)+'\n');return 2
    # Call the consumption-side key function: no second formula or ordering.
    paths={f:sorted(v) for f,v in reads.items()};consumer.ROOT=root;consumer.read_set=lambda f:paths[f]
    keys={f:consumer.reference_key(f) for f in paths}
    (out/'seedmemory-reads.tsv').write_text('family\tpath\n'+''.join(f+'\t'+p+'\n' for f in ('gen','parse2') for p in paths[f]))
    binaries={r.get('product_sha256') for r in selected.values() if r.get('backend')=='c' and r.get('status')=='OBSERVED'}
    if len(binaries)!=1 or None in binaries:raise ValueError('C binary identity differs')
    constants={'C_SHA':source,'BINARY_SHA':next(iter(binaries)),'REFERENCE_KEYS':keys,'C_RSS':rss}
    (out/'seedmemory-constants.json').write_text(json.dumps(constants,indent=2)+'\n')
    (out/'seedmemory-constants.py.txt').write_text(''.join(k+'='+repr(v)+'\n' for k,v in constants.items()))
    (out/'summary.json').write_text(json.dumps({'status':'COMPLETE_SERIAL_EVIDENCE','constants':constants,'note':'MiB floor; consumer retains +1 MiB guard. Python cold/parallel admission not authorized.'},indent=2)+'\n');return 0

def main():
    if len(sys.argv)>1 and sys.argv[1]=='--rss-worker':return worker(sys.argv[3:],sys.argv[2])
    if len(sys.argv)>1 and sys.argv[1]=='--summarize':return summarize(sys.argv[2:])
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root',required=True,type=pathlib.Path);ap.add_argument('--out',required=True,type=pathlib.Path)
    ap.add_argument('--name',required=True);ap.add_argument('--family',choices=['gen','parse2'],required=True)
    ap.add_argument('--kind',choices=['trace','rss'],required=True);ap.add_argument('--backend',choices=['c','python'],required=True)
    ap.add_argument('--binary',type=pathlib.Path);ap.add_argument('route',nargs=argparse.REMAINDER)
    a=ap.parse_args();root=a.root.resolve(strict=True);out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
    if out.is_relative_to(root):ap.error('--out must be outside the repository')
    assert re.fullmatch('[a-zA-Z0-9_,.-]+',a.name),'unsafe evidence name'
    route=a.route[1:] if a.route[:1]==['--'] else a.route;assert route,'stage/flags required'
    key=a.name+'.'+a.backend+'.'+a.kind;rec=out/(key+'.result.json');model=out/(key+'.model.json');trace=out/(key+'.strace')
    # Cold Python code reads: do not reuse the export's pyc files.
    cmd=([str(a.binary.resolve(strict=True))] if a.backend=='c' and a.binary else [sys.executable,'-I','-X','pycache_prefix='+str(out/(key+'.pycache')),'exec/build/gen.py'])
    if a.backend=='c' and not a.binary:ap.error('--binary required for C')
    cmd += [route[0],str(model)]+route[1:]
    frozen=source_hash(root);source_sha=digest(root/'seed/gen.c');product_sha=digest(a.binary.resolve(strict=True)) if a.backend=='c' else '-';start=time.monotonic();observation={}
    if a.kind=='trace':cmd=['strace','-f','-qq','-s','4096','-e','trace=openat','-o',str(trace)]+cmd
    else:
        if not hasattr(os,'wait4'):ap.error('RSS requires Linux wait4')
        cmd=[sys.executable,str(pathlib.Path(__file__).resolve()),'--rss-worker',str(out/(key+'.rss.json'))]+cmd
    try:
        with (out/(key+'.stdout')).open('wb') as f,(out/(key+'.stderr')).open('wb') as e:
            p=subprocess.run([sys.executable,str(root/'tests/bound.py'),'45']+cmd,cwd=root,stdout=f,stderr=e)
        observation={'status':'OBSERVED' if p.returncode==0 else 'NOT_ACCEPTED','rc':p.returncode,'seconds':time.monotonic()-start,'source_sha256':source_sha,'source_hash':frozen,'family':a.family,'name':a.name,'backend':a.backend,'kind':a.kind,'route':route,'root':str(root),'argv':cmd,'product_sha256':product_sha,'model_path':str(model),'binary_path':str(a.binary.resolve(strict=True)) if a.backend=='c' else None}
        if p.returncode==0:
            assert digest(root/'seed/gen.c')==source_sha and source_hash(root)==frozen,'source changed during observation'
            if a.backend=='c':assert digest(a.binary.resolve(strict=True))==product_sha,'binary changed during observation'
            observation['model_sha256']=digest(model)
            if a.kind=='rss':observation['rss']=json.loads((out/(key+'.rss.json')).read_text())
            else:
                reads=set()
                for line in trace.read_text().splitlines():
                    if not re.search(r'= [0-9]+$',line):continue
                    m=re.search(r'openat\(([^,]+), ("(?:[^"\\]|\\.)*")',line)
                    if not m:continue
                    raw=json.loads(m.group(2));q=pathlib.Path(raw)
                    if not q.is_absolute():
                        assert m.group(1)=='AT_FDCWD','relative dirfd open needs explicit resolver'
                        q=root/q
                    try:q=q.resolve(strict=True);rel=q.relative_to(root);mode=q.stat().st_mode
                    except (FileNotFoundError,ValueError):continue
                    if stat.S_ISREG(mode) and '__pycache__' not in rel.parts and rel.suffix!='.pyc':reads.add(str(rel))
                observation['read_bytes']=[{'path':s,'sha256':digest(root/s)} for s in sorted(reads)]
        rec.write_text(json.dumps(observation,indent=2)+'\n');write_step(out,key,observation,cmd);return p.returncode
    except (FileNotFoundError,ValueError,AssertionError) as e:
        observation={'status':'EVIDENCE_ERROR','rc':2,'reason':str(e),'source_sha256':source_sha,'source_hash':frozen,'product_sha256':product_sha,'family':a.family,'name':a.name,'backend':a.backend,'kind':a.kind,'route':route,'seconds':time.monotonic()-start};rec.write_text(json.dumps(observation,indent=2)+'\n');write_step(out,key,observation,cmd);return 2
if __name__=='__main__':raise SystemExit(main())
