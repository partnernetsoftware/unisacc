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
        sources=[p for d in ('exec/pp','exec/lex','exec/parse','exec/parse2','unisa') for p in (ROOT/d).rglob('*.py')]
        sources+=[ROOT/'exec/finite_rules.py',ROOT/'exec/c/tbl.py',ROOT/'exec/c/net.py']
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
    key=hashlib.sha256(json.dumps([closure_hash,str(Path(script).resolve()),list(map(str,args)),
                                   [(k,effective.get(k)) for k in names]]).encode()).hexdigest()
    base=Path(os.environ.get('UNISACC_MODEL_CACHE',tempfile.gettempdir()+'/unisacc-model-cache'))
    base.mkdir(parents=True,exist_ok=True)
    cache=base/('pack-'+key)
    with (base/('pack-'+key+'.lock')).open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        if not _valid(cache):
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


def compiler_package(manifests, o1, includes, kernels=None, audit_dir=None, compressed=True):
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
    rows=[];targets=set()
    for path in map(Path,manifests):
        stages=[x.split('\t') for x in path.read_text().splitlines() if x and not x.startswith('#')]
        if not stages or any(len(s)!=5 for s in stages): raise ValueError('invalid source route')
        target=stages[0][0]
        if target in targets or any(s[0]!=target for s in stages): raise ValueError('duplicate/mixed target')
        targets.add(target)
        names=[s[1] for s in stages]
        if names!=['e2','e1','e3','e4','prune','lower','elf']: raise ValueError('unexpected image stages')
        for _,name,inp,out,model in stages[:2]:
            rows.append('\t'.join([target+'/unit',name,inp,out,str((path.parent/model).resolve())]))
        _,name,inp,out,model=stages[-1]
        rows.append('\t'.join([target+'/memory',name,inp,'memory-v1',str((path.parent/model).resolve())]))
        for suffix,last,opt in specs:
            for _,name,inp,out,model in stages:
                if name=='e4' and opt=='none': continue
                model=Path(o1).resolve() if name=='e4' and opt=='O1' else (path.parent/model).resolve()
                rows.append('\t'.join([target+'/'+suffix,name,inp,out,str(model)]))
                if name==last: break
    with tempfile.TemporaryDirectory(prefix='compiler-package-') as td:
        # Offline construction of the shared unit-framing network. Runtime
        # does not invoke these Python tools or parse declarations in C.
        here=Path(__file__).resolve().parent
        # Public token dump uses the reference's fixed Linux/x86 predefines
        # and the plain E1 output, independently of image-target selection.
        for name,script,args,inp,out in [
                ('tokenpp',here.parent/'pp/gen.py',['lnx/x86_64'],'src.c','pp.text'),
                ('tokenlex',here.parent/'lex/gen.py',[],'pp.text','tokens.plain')]:
            # Public token dump has no implicit header selection.
            n=built_model(td,name,script,args,dict(os.environ,E2_AUTOINC='0') if name=='tokenpp' else None)
            rows.append('\t'.join(['tokens',name,inp,out,str(n)]))
        # Located warning routes share one lexer/parser; preprocessing keeps
        # target predefines. This construction never runs in the driver.
        warning_models={}
        def warning_model(name,script,args):
            return built_model(td,name,script,args)
        warning_models['e1']=warning_model('warnlex',here.parent/'lex/gen.py',['--locations'])
        warning_models['e3']=warning_model('warnparse',here.parent/'parse2/gen2.py',['--warnings','--errors'])
        located_units=warning_model('warnunits',here.parent/'parse2/units.py',['--locations'])
        quiet_parse=warning_model('errorparse',here.parent/'parse2/gen2.py',['--errors'])
        ordinary=list(rows)
        for target in sorted(targets):
            warning_models['e2']=warning_model('warnpp-'+target.replace('/','-'),here.parent/'pp/gen.py',[target,'--locations'])
            # Normal compilation also carries locations, without enabling
            # warnings. The same units model preserves file boundaries.
            quiet_routes={target+'/unit'}|{target+'/'+s for s,_,_ in specs if s!='pp'}
            for i,row in enumerate(rows):
                cols=row.split('\t')
                if cols[0] not in quiet_routes: continue
                if cols[1]=='e2': cols[3:5]=['pp.locations',str(warning_models['e2'])]
                elif cols[1]=='e1': cols[2:5]=['pp.locations','tokens.locations',str(warning_models['e1'])]
                elif cols[1]=='e3': cols[2]='tokens.locations';cols[4]=str(quiet_parse)
                rows[i]='\t'.join(cols)
            rows.append('\t'.join([target+'/warn/unit','e2','src.c','pp.locations',str(warning_models['e2'])]))
            rows.append('\t'.join([target+'/warn/unit','e1','pp.locations','tokens.locations',str(warning_models['e1'])]))
            for suffix,last,opt in specs:
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
            for suffix,last,opt in specs:
                if suffix=='pp': continue
                route=target+'/multi/'+suffix
                rows.append('\t'.join([route,'units','units.locations','tokens.locations',str(located_units)]))
                for row in base:
                    cols=row.split('\t')
                    if cols[0]==target+'/'+suffix and cols[1] not in ('e2','e1'):
                        cols[0]=route;rows.append('\t'.join(cols))
        for target in sorted(targets):
            for suffix,last,opt in specs:
                if suffix=='pp': continue
                route=target+'/warn/multi/'+suffix
                rows.append('\t'.join([route,'units','units.locations','tokens.locations',str(located_units)]))
                for row in base:
                    cols=row.split('\t')
                    if cols[0]==target+'/warn/'+suffix and cols[1] not in ('e2','e1'):
                        cols[0]=route;rows.append('\t'.join(cols))
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
    ap.add_argument('--compressed',dest='compressed',action='store_true',default=True,help='P3 binary+DEFLATE models (default)')
    ap.add_argument('--legacy-package',dest='compressed',action='store_false',help='legacy uncompressed P1/P2')
    ap.add_argument('--no-model-cache',action='store_true',help='construct every model afresh (the shipped build)')
    ap.add_argument('manifests',nargs='+',type=Path)
    a=ap.parse_args()
    MODEL_CACHE=not a.no_model_cache
    try:
        payload=compiler_package(a.manifests,a.o1,a.include,a.kernels,a.audit_dir,a.compressed);a.o.write_bytes(payload)
    except (OSError,ValueError) as e: ap.exit(1,f'compilerpack: {e}\n')
    print(f'compiler package: {len(payload)} B')
