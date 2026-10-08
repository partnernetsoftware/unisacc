#!/usr/bin/env python3
"""Declare compiler CLI routes using existing, shared stage networks.
Only construction: the runtime reads the resulting package without Python.
"""
import argparse,os
from pathlib import Path
import tempfile,struct,hashlib,json,shutil
import subprocess,sys,fcntl,re
from pack import build

# Tests rebuild packages many times from identical inputs; the shipped build
# (buildcompiler.sh) passes --no-model-cache and constructs every model afresh.
MODEL_CACHE=True
_KEYBASE=None
# pack-models with --require-cached: every model must already sit verified in
# the cache (written by the pack-prep-K steps); constructing one here is an error.
REQUIRE_CACHED=False
# Prepare steps: buildcompiler.sh pack-prep-K builds part K of PREP_PARTS into
# $T/model-cache, so that no single bounded step constructs all models.  Groups
# are fixed (by name, measured to stay under ~45 s each); every model job must
# appear in exactly one group (checked in prepare()).
PREP_PARTS=[
    ['warnparse'],
    ['errorparse'],
    ['warnunits','warnlex','tokenlex','tokenpp','warnpp-shared','plainpp','nativeabi',
     'object-lower-arm64','object-lower-x86_64','object-enc-arm64','object-enc-x86_64'],
]

def _digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def _keybase():
    """The file closure shared with exec/pipeline/models.py, the Python
    version, and every environment name mentioned next to environ/getenv in
    the sources these generators can import (whole directories, so a new
    import is covered; per-suite checker settings elsewhere are not)."""
    global _KEYBASE
    if _KEYBASE is None:
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'pipeline'))
        from models import closure, ROOT
        names=set()
        sources=[p for d in ('exec/pp','exec/lex','exec/parse','exec/parse2','exec/nativeabi','unisa') for p in (ROOT/d).rglob('*.py')]
        sources+=sorted((ROOT/'exec').glob('*.py'))+sorted((ROOT/'exec').glob('model*.tsv'))+sorted((ROOT/'exec/build').glob('*.py'))+sorted((ROOT/'exec/facts').glob('*.py'))+[ROOT/'exec/c/tbl.py',ROOT/'exec/c/net.py']   # K2: model* installers are manifests now
        for p in sorted(sources):
            for line in p.read_text(errors='replace').splitlines():
                if 'environ' in line or 'getenv' in line:
                    names.update(re.findall(r'\b[A-Z][A-Z0-9_]{2,}\b',line))
        h=closure(hashlib.sha256(json.dumps(['compilerpack-model-v1',sys.version]).encode()))
        _KEYBASE=(h.hexdigest(),sorted(names))
    return _KEYBASE

def _valid(cache):
    try:
        manifest=json.loads((cache/'manifest.json').read_text())
        return set(manifest)=={'m.tbl','m.net'} and all(_digest(cache/k)==v for k,v in manifest.items())
    except (FileNotFoundError,ValueError):
        return False

def _construct(script,args,env,j,t,n):
    here=Path(__file__).resolve().parent
    for tool,argv in [(script,[j,*args]),(here/'tbl.py',[j,t]),(here/'net.py',[t,n])]:
        subprocess.run([sys.executable,str(tool),*map(str,argv)],check=True,timeout=60,env=env)

def built_model(td,name,script,args,env=None):
    """Construct NAME.tbl/.net in td, reusing a verified content-keyed cache."""
    j=Path(td)/(name+'.json');t=Path(td)/(name+'.tbl');n=Path(td)/(name+'.net')
    if not MODEL_CACHE:
        _construct(script,args,env,j,t,n); return n
    closure_hash,names=_keybase()
    effective=os.environ if env is None else env
    s=Path(script).resolve();root=Path(__file__).resolve().parents[2]
    # 0.0.36 P8: the script by its repo-relative spelling, so a frozen worktree with the same bytes hits
    sname=s.relative_to(root).as_posix() if s.is_relative_to(root) else str(s)
    key=hashlib.sha256(json.dumps([closure_hash,sname,list(map(str,args)),
                                   [(k,effective.get(k)) for k in names]]).encode()).hexdigest()
    base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
    base.mkdir(parents=True,exist_ok=True)
    cache=base/('pack-'+key)
    with (base/('pack-'+key+'.lock')).open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        if not _valid(cache):
            if REQUIRE_CACHED: raise ValueError('model %s not prepared (run the pack-prep steps first)'%name)
            work=Path(tempfile.mkdtemp(prefix='pack-'+key+'.build-',dir=base))
            try:
                _construct(script,args,env,work/'m.json',work/'m.tbl',work/'m.net')
                (work/'m.json').unlink()
                (work/'manifest.json').write_text(json.dumps({k:_digest(work/k) for k in ('m.tbl','m.net')},sort_keys=True))
                try: cache.stat()
                except FileNotFoundError: pass
                else: shutil.rmtree(cache)
                work.rename(cache)
            finally:
                try: work.stat()
                except FileNotFoundError: pass
                else: shutil.rmtree(work)
        shutil.copyfile(cache/'m.tbl',t); shutil.copyfile(cache/'m.net',n)
    return n

def retain_models(directory, rows):
    """Keep the exact constructed pairs for the existing full-domain checker."""
    directory=Path(directory)
    directory.mkdir(parents=True,exist_ok=True)
    record=directory/'models.json'
    record.unlink(missing_ok=True)
    models={}
    for row in rows:
        network=Path(row.split('\t')[4]); table=network.with_suffix('.tbl')
        raw=network.read_bytes(); source=table.read_bytes()
        nh=hashlib.sha256(raw).hexdigest(); th=hashlib.sha256(source).hexdigest()
        # Several routes may share one network. Retain distinct source tables
        # too: equivalent networks do not prove their source tables identical.
        key=nh+'-'+th
        if key in models: continue
        shutil.copyfile(network,directory/(key+'.net'))
        shutil.copyfile(table,directory/(key+'.tbl'))
        models[key]={'network_sha256':nh,'table_sha256':th}
    if not models: raise ValueError('empty model audit')
    record.write_text(json.dumps({'pairs':models},sort_keys=True,indent=2)+'\n')


def model_jobs(targets, shared_e2=None, shared_nativeabi=None):
    """Every model compiler_package constructs, as (name, script, args), in order."""
    gen=Path(__file__).resolve().parent.parent/'build/gen.py'
    jobs=[]
    for target in sorted(set(targets) & {'lnx/x86_64','lnx/arm64'}):
        arch=target.split('/')[1]
        jobs.append(('object-lower-'+arch,gen,['lower','--full','--object']+(['--arm64'] if arch=='arm64' else [])))
        jobs.append(('object-enc-'+arch,gen,['enc/arm' if arch=='arm64' else 'enc','--object']))
    if shared_e2 is None: jobs.append(('plainpp',gen,['pp','--shared-predefines']))
    jobs+=[('tokenpp',gen,['pp','--shared-predefines','--no-autoinc']),
           ('tokenlex',gen,['lex']),
           ('warnlex',gen,['lex','--locations']),
           ('warnparse',gen,['parse2','--warnings','--errors']),
           ('warnunits',gen,['parse2/units','--locations']),
           ('errorparse',gen,['parse2','--errors']),
           ('warnpp-shared',gen,['pp','--locations','--shared-predefines'])]
    if shared_nativeabi is None: jobs.append(('nativeabi',gen,['nativeabi']))
    return jobs

def manifest_targets(manifests):
    return {next(x for x in Path(m).read_text().splitlines() if x and not x.startswith('#')).split('\t')[0] for m in manifests}

def prepare(manifests, part, parts, shared_e2=None, shared_nativeabi=None):
    """Construct part PART (1-based) of PARTS into the model cache; no package."""
    if parts!=len(PREP_PARTS) or not 1<=part<=parts: raise ValueError('--part must be K/%d'%len(PREP_PARTS))
    jobs={n:(sc,a) for n,sc,a in model_jobs(manifest_targets(manifests),shared_e2,shared_nativeabi)}
    named=[n for g in PREP_PARTS for n in g]
    if len(named)!=len(set(named)) or not set(jobs)<=set(named): raise ValueError('PREP_PARTS does not cover the model jobs once each')
    with tempfile.TemporaryDirectory(prefix='compiler-prepare-') as td:
        for n in PREP_PARTS[part-1]:
            if n in jobs: built_model(td,n,*jobs[n]); print('prepared',n,flush=True)

def compiler_package(manifests, o1, includes, kernels=None, audit_dir=None, compressed=True, shared_e2=None, shared_nativeabi=None):
    mounts=[('006864722f',includes)]
    if kernels is not None:
        kernels=Path(kernels)
        if not kernels.is_dir() or {p.name for p in kernels.iterdir()}!={'arm64','x86_64'}:
            raise ValueError('kernel directory must contain arm64 and x86_64 only')
        for isa,arch in enumerate(('arm64','x86_64'),1):
            raw=(kernels/arch).read_bytes()
            if len(raw)<40 or raw[:8]!=b'UNIKERN1': raise ValueError('invalid kernel header')
            kind,entry,slot,length=struct.unpack_from('<4Q',raw,8)
            if (kind!=isa or length!=len(raw)-40 or entry>=length or slot+8>length
                    or slot%8 or any(raw[40+slot:48+slot])):
                raise ValueError('invalid kernel ISA or extent')
        mounts.append(('006b65726e656c2f',kernels))
    specs=[]
    for text in Path(__file__).with_name('compiler-routes.tsv').read_text().splitlines():
        if text and not text.startswith('#'): specs.append(text.split('\t'))
    rows=[];targets=set();target_specs={}
    for path in map(Path,manifests):
        stages=[x.split('\t') for x in path.read_text().splitlines() if x and not x.startswith('#')]
        if not stages or any(len(s)!=5 for s in stages): raise ValueError('invalid source route')
        target=stages[0][0]
        if target in targets or any(s[0]!=target for s in stages): raise ValueError('duplicate/mixed target')
        targets.add(target)
        target_specs[target]=list(specs)
        if target in ('lnx/x86_64','lnx/arm64'):
            target_specs[target]+=[['object/'+level,'elf',opt] for level,opt in (('O0','none'),('O1','O1'),('O2','O2'))]
        names=[s[1] for s in stages]
        if names!=['e2','e1','e3','e4','prune','lower','elf']: raise ValueError('unexpected image stages')
        for _,name,inp,out,model in stages[:2]:
            rows.append('\t'.join([target+'/unit',name,inp,out,str((path.parent/model).resolve())]))
        _,name,inp,out,model=stages[-1]
        rows.append('\t'.join([target+'/memory',name,inp,'memory-v1',str((path.parent/model).resolve())]))
        for suffix,last,opt in target_specs[target]:
            for _,name,inp,out,model in stages:
                if name=='e4' and opt=='none': continue
                model=Path(o1).resolve() if name=='e4' and opt=='O1' else (path.parent/model).resolve()
                if suffix.startswith('object/') and name=='elf': out='object-v1'
                rows.append('\t'.join([target+'/'+suffix,name,inp,out,str(model)]))
                if name==last: break
    with tempfile.TemporaryDirectory(prefix='compiler-package-') as td:
        # Offline construction of the shared unit-framing network. Runtime
        # does not invoke these Python tools or parse declarations in C.
        here=Path(__file__).resolve().parent
        jobs={n:(sc,a) for n,sc,a in model_jobs(targets,shared_e2,shared_nativeabi)}
        def job(name): return built_model(td,name,*jobs[name])
        object_models={}
        for target in sorted(targets & {'lnx/x86_64','lnx/arm64'}):
            arch=target.split('/')[1]
            object_models[target]={
                'lower':job('object-lower-'+arch),'elf':job('object-enc-'+arch)}
        for i,row in enumerate(rows):
            cols=row.split('\t')
            for target,models in object_models.items():
                if cols[0].startswith(target+'/object/') and cols[1] in models:
                    cols[4]=str(models[cols[1]]);break
            rows[i]='\t'.join(cols)
        # Declare all target macro lists as resources; choosing and parsing
        # the list remains in the shared E2 delta. The driver supplies target.
        # The lists are facts (exec/facts/pp-gen.tsv predefres, from exec/pp/predefines.tsv).
        sys.path.insert(0,str(here.parent))
        import assemble
        predefines=Path(td)/'predefines';predefines.mkdir()
        for target,value in sorted(assemble.load_facts('pp-gen')['predefres'].items()):
            path=predefines/target;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(value.encode('ascii'))
        mounts.append(('00707265646566696e65732f',predefines))
        plain_pp=Path(shared_e2).resolve() if shared_e2 is not None else job('plainpp')
        for i,row in enumerate(rows):
            cols=row.split('\t')
            if cols[1]=='e2': cols[4]=str(plain_pp);rows[i]='\t'.join(cols)
        # Public token dump uses the reference's fixed Linux/x86 predefines
        # and the plain E1 output, independently of image-target selection.
        for name,inp,out in [('tokenpp','src.c','pp.text'),('tokenlex','pp.text','tokens.plain')]:
            # Public token dump has no implicit header selection.
            n=job(name)
            rows.append('\t'.join(['tokens',name,inp,out,str(n)]))
        # Located warning routes share one lexer/parser; preprocessing keeps
        # target predefines. This construction never runs in the driver.
        warning_models={}
        warning_models['e1']=job('warnlex')
        warning_models['e3']=job('warnparse')
        located_units=job('warnunits')
        quiet_parse=job('errorparse')
        ordinary=list(rows)
        warning_models['e2']=job('warnpp-shared')
        for target in sorted(targets):
            # Normal compilation also carries locations, without enabling
            # warnings. The same units model preserves file boundaries.
            quiet_routes={target+'/unit'}|{target+'/'+s for s,_,_ in target_specs[target] if s!='pp'}
            for i,row in enumerate(rows):
                cols=row.split('\t')
                if cols[0] not in quiet_routes: continue
                if cols[1]=='e2': cols[3:5]=['pp.locations',str(warning_models['e2'])]
                elif cols[1]=='e1': cols[2:5]=['pp.locations','tokens.locations',str(warning_models['e1'])]
                elif cols[1]=='e3': cols[2]='tokens.locations';cols[4]=str(quiet_parse)
                rows[i]='\t'.join(cols)
            rows.append('\t'.join([target+'/warn/unit','e2','src.c','pp.locations',str(warning_models['e2'])]))
            rows.append('\t'.join([target+'/warn/unit','e1','pp.locations','tokens.locations',str(warning_models['e1'])]))
            for suffix,last,opt in target_specs[target]:
                if suffix=='pp': continue
                for row in ordinary:
                    cols=row.split('\t')
                    if cols[0]!=target+'/'+suffix: continue
                    cols[0]=target+'/warn/'+suffix
                    if cols[1] in warning_models: cols[4]=str(warning_models[cols[1]])
                    if cols[1]=='e2': cols[3]='pp.locations'
                    elif cols[1]=='e1': cols[2:4]=['pp.locations','tokens.locations']
                    elif cols[1]=='e3': cols[2]='tokens.locations'
                    rows.append('\t'.join(cols))
        base=list(rows)
        for target in sorted(targets):
            for suffix,last,opt in target_specs[target]:
                if suffix=='pp': continue
                route=target+'/multi/'+suffix
                rows.append('\t'.join([route,'units','units.locations','tokens.locations',str(located_units)]))
                for row in base:
                    cols=row.split('\t')
                    if cols[0]==target+'/'+suffix and cols[1] not in ('e2','e1'):
                        cols[0]=route;rows.append('\t'.join(cols))
        for target in sorted(targets):
            for suffix,last,opt in target_specs[target]:
                if suffix=='pp': continue
                route=target+'/warn/multi/'+suffix
                rows.append('\t'.join([route,'units','units.locations','tokens.locations',str(located_units)]))
                for row in base:
                    cols=row.split('\t')
                    if cols[0]==target+'/warn/'+suffix and cols[1] not in ('e2','e1'):
                        cols[0]=route;rows.append('\t'.join(cols))
        nativeabi=Path(shared_nativeabi).resolve() if shared_nativeabi is not None else job('nativeabi')
        for target in sorted(targets):
            rows.append('\t'.join([target+'/nativeabi','nativeabi','USLSIG2','USLNCAR1',str(nativeabi)]))
        manifest=Path(td)/'routes.tsv';manifest.write_text('\n'.join(rows)+'\n')
        payload=build([manifest],mounts,compressed=compressed,cache=MODEL_CACHE)
        if audit_dir is not None: retain_models(audit_dir, rows)
        return payload

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('-o',required=True,type=Path)
    ap.add_argument('--o1',required=True,type=Path)
    ap.add_argument('--include',required=True,type=Path)
    ap.add_argument('--kernels',type=Path,help='explicit directory containing both ISA kernel blobs')
    ap.add_argument('--audit-dir',type=Path,help='retain exact table/network pairs for offline --check-net')
    ap.add_argument('--shared-e2',type=Path,help='fresh shared plain E2 network from --shared-predefines')
    ap.add_argument('--shared-nativeabi',type=Path,help='fresh shared model-certified ABI carrier network')
    ap.add_argument('--compressed',dest='compressed',action='store_true',default=True,help='P3 binary+DEFLATE models (default)')
    ap.add_argument('--legacy-package',dest='compressed',action='store_false',help='legacy uncompressed P1/P2')
    ap.add_argument('--no-model-cache',action='store_true',help='construct every model afresh (the shipped build)')
    ap.add_argument('--prepare-only',action='store_true',help='construct models of --part into the cache; write no package')
    ap.add_argument('--part',help='K/N: which prepare group (with --prepare-only)')
    ap.add_argument('--require-cached',action='store_true',help='fail if any model is not already in the cache')
    ap.add_argument('manifests',nargs='+',type=Path)
    a=ap.parse_args()
    MODEL_CACHE=not a.no_model_cache
    REQUIRE_CACHED=a.require_cached
    if a.prepare_only:
        try:
            k,n=map(int,(a.part or '').split('/'))
        except ValueError: ap.exit(2,'compilerpack: --prepare-only needs --part K/N\n')
        try: prepare(a.manifests,k,n,a.shared_e2,a.shared_nativeabi)
        except (OSError,ValueError) as e: ap.exit(1,f'compilerpack: {e}\n')
        raise SystemExit(0)
    try:
        payload=compiler_package(a.manifests,a.o1,a.include,a.kernels,a.audit_dir,a.compressed,a.shared_e2,a.shared_nativeabi);a.o.write_bytes(payload)
    except (OSError,ValueError) as e: ap.exit(1,f'compilerpack: {e}\n')
    print(f'compiler package: {len(payload)} B')
