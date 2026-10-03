"""Library-only signature records from E3's existing typed descriptors.
No host parser or signature guess. Default tape is unchanged. Metadata is a
length-framed outer envelope; prune/lower see only its separated tape payload.
"""
from pathlib import Path
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('libraryexports'))

def install(E,P,b,start,integers):
    import assemble
    from finite_rules import install as rules, install_template
    import types
    librarycallables=types.SimpleNamespace(__name__='librarycallables',**{r['name']:_t(r['value']) for r in _facts('librarycallables')})  # exec/facts/librarycallables.tsv
    layoutfacts=types.SimpleNamespace(**assemble.load_facts('layoutfacts')['layoutfacts!'])  # exec/facts/layoutfacts.tsv
    libraryimports=types.SimpleNamespace(**assemble.load_facts('libraryimports')['libraryimports!'])  # exec/facts/libraryimports.tsv
    modelsignature=types.SimpleNamespace(**assemble.load_facts('top-modelsignature-banks')['top-modelsignature-banks!'])  # exec/facts/top-modelsignature-banks.tsv
    modelgraphequality=types.SimpleNamespace(**assemble.load_facts('top-modelgraphequality-banks')['top-modelgraphequality-banks!'])  # exec/facts/top-modelgraphequality-banks.tsv
    for owner in (modelsignature,modelgraphequality,libraryimports,librarycallables,layoutfacts):
        assert not set(RANK_BANKS).intersection(v for k,v in vars(owner).items() if type(v) is int and v>=1<<40), owner.__name__
    g=E.g
    assemble.run(Path(__file__).resolve().parent/'libraryresources-manifest.tsv',E,E.P,dict(lx=1,lc=0,lmd=0),{})   # libraryresources-manifest.tsv
    # Stage control: libraryexports-result.tsv sections head/record/signature/storage/owner/
    # begin/ellipsis/capture/rankreturn/union/envelope/argument/call. Rewrites of
    # existing states (hooks, FN linkage, FN.pfpstacked, CL.namedquery, LX.startok,
    # the ACCEPT interception) are declared in libraryexports-template.tsv. Python
    # binds only dynamic facts: graph constants (bank ids, b[...] descriptor banks),
    # the start continuation, and fresh labels allocated in libraryexports-fresh.tsv order.
    root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'libraryexports-fresh.tsv').read_text().splitlines()[1:]]
    sequences={'out USLSIG2':E.O('USLSIG2\n'),'out USLSIG3':E.O('USLSIG3\n'),
               'out USLTAPE1':E.O('USLTAPE1\n'),
               'reject':E.rej('not covered: library signature resource or duplicate definition')}
    constants=dict({k:v for k,v in globals().items() if type(v) is int and v>=1<<40},
                   **{k:b[k] for k in ('FPS_FN','FPS_COUNT','FPS_VAR','FPS_RD','FPS_RB','FPS_RSH')})
    def section(name):
        p=P('LX.fresh.'+name);bindings=dict(constants,start=start,tk_static=E.TK['type=static'])
        for part,key,prefix,kind in fresh:
            if part==name:assert prefix=='LX';bindings[key]=p.fresh(kind)
        rules(g,root,'libraryexports',bindings,sequences,section=name)
    section('head')
    class Holder: pass
    def template(name,facts={},cur='LX.x'):
        holder=Holder();holder.cur=cur
        install_template(g,root,'libraryexports',facts,lambda k:E.P.fresh(holder,k),section=name)
    # TOP.st observes the actual storage token before NEXT drops it.
    section('storage')
    def hook(state,procedure,always=False):
        facts={'H':[dict(s=state,p=procedure)]}
        template('move',facts);template('always' if always else 'hook',facts,state)
    hook('TOP.st','LX.storage')
    hook('TOP.static','LX.storage')   # file-scope `static` takes its own state since R13-0b #31 (STATICF); linkage must still be recorded
    template('linkage')
    # Each parameter list owns its epoch, including nested anonymous lists.
    section('owner')
    hook('FN.params','LX.begin',always=True)
    section('begin')
    # A callback parameter's own mode does not change the enclosing ABI.
    template('pfpstacked',{'key':range(257)})
    hook('FN.dots','LX.ellipsis',always=True)
    section('ellipsis')
    # SIG.store is reached after scalar/array/function parameter adjustment.
    hook('SIG.store','LX.capture',always=True)
    hook('FN.pfpdecl1','LX.capture',always=True)
    # Prior prototype slots remain readable until the current slot is replaced.
    section('capture')
    # Return-callback suffixes are parsed after FN.params/FN.fpclose.
    hook('FN.bodykind','LX.rankreturn',always=True)
    section('rankreturn')
    # Union identity must survive SB.end's restoration of the enclosing parser.
    hook('SB.go','LX.union')
    section('union')
    hook('FN.def1','LX.definition')
    section('record')
    import assemble, gen2
    assemble.run(root/'librarytypes-manifest.tsv',E,P,{},dict({'b_'+k:v for k,v in b.items() if type(v) is int},
        isize=next(size for name,code,size,uns,narrow in integers if name=='i32'),
        ints=[{'code':code,'size':size,'uns':int(uns)} for _,code,size,uns,_ in integers],E_ARR=E.ARR,gen2_DIM=gen2.DIM))
    # Import prototypes serialize through the same descriptor path. Names are
    # already-owned blobs; parser source offsets no longer need remain active.
    section('signature')
    # Intercept successful acceptance only, preserving its prior output actions.
    template('accept')
    section('envelope')
    rules(g,Path(__file__).parent,'libraryexports',section='write')
    from librarymodule import install as module_install
    assemble.run(root/'libraryimports-manifest.tsv',E,P,{},dict({'b_'+k:v for k,v in b.items() if type(v) is int},start=start,
        TK_ID=E.TK_ID,TK_SEMI=E.TK[';'],FPB_FPV=[b['FPB'],b['FPV']],BOOL=[b['BOOL']],
        ints2=[{'code':code,'width':width,'uns':uns} for _,code,width,uns,_ in integers]));imports_start='LI.start'
    # Complete parameter capture is also needed by ordinary source calls.
    # The legacy first-eight cache default-promoted a ninth fixed float.
    # Extend only the missing ninth-and-later slots. First-eight source slots
    # retain PDB, whose declarator path includes array/function decay; the
    # export descriptor captured before FN.pfpdecl1 is not that conversion.
    # Typed imports keep their separate complete-signature query below.
    # Genuinely variadic tail and absent declarations retain the old path.
    template('argumentmove')
    section('argument')
    template('argumentcopy')
    section('call')
    import assemble
    assemble.run(root/'libraryvariadic-manifest.tsv',E,P,{})
    import json
    _li=assemble.load_facts('libraryimports')['libraryimports!']  # exec/facts/libraryimports.tsv
    _vr={r['name']:r['value'] for r in _facts('valueranks') if r['kind']=='bank'}  # exec/facts/valueranks.tsv
    from unresolved import DEFINED
    lc_constants=dict({k:v for k,v in vars(librarycallables).items() if type(v) is int and v>=1<<40},
        RETURNRANK=RETURNRANK,PARAMRANK=PARAMRANK,LCSITERANK=_vr['LCSITERANK'],
        **{k:_li[k] for k in ('BYNAME','ADDRESS','FORMAT','SUPPORTED','TYPEDSIG','PLAN')},
        REQUESTS=assemble.load_facts('libraryvariadic')['REQUESTS'],DEFINED=DEFINED,VARIADIC=VARIADIC,
        E_VAR=E.VAR,E_DBL=E.DBL,E_INT=E.SZ['int'],TK_SEMI=E.TK[';'],
        **{'b_'+k:b[k] for k in ('FPS_FN','FPS_COUNT','FPS_RB','FPS_RD','FPS_RSH','FPS_VAR','SSZ','SBB')})
    lc_classes={'uns1':[E.UNS+1],'uns2':[E.UNS+2],'bool':[b['BOOL']],'float':[b['FLT']]}
    lc_seq={}
    for line in (root/'librarycallables-result.tsv').read_text().splitlines()[1:]:
        if line:
            for a in json.loads(line.split('\t')[4]):
                if a[0]=='@' and a[1].startswith('out:'):lc_seq[a[1]]=E.O(a[1][4:])
    callable_start=assemble.run(root/'librarycallables-manifest.tsv',E,P,{},   # librarycallables-manifest.tsv
        dict(constants=lc_constants,classes=lc_classes,start=imports_start,fpcont=E.results['fpcont'],text_seqs=lc_seq))['ret']
    module_start=module_install(E,P,callable_start)
    template('startok',{'module_start':[module_start],'key':range(257)})
    return 'LX.version'
