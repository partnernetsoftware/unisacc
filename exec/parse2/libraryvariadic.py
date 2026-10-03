"""One-pass, model-owned promoted variadic call graphs and source priority.
The host receives only final length-framed declarations; no source decisions.
"""
SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS=(i<<40 for i in range(400,408))
def install(E,P,b,integers):
    from libraryexports import RETURNRANK, PARAMRANK
    from load import facts as _vrf; LVSITERANK={r['name']:r['value'] for r in _vrf('valueranks') if r['kind']=='bank'}['LVSITERANK']   # exec/facts/valueranks.tsv
    from libraryimports import BYNAME,FORMAT,IDS,PLAN,DISPATCH,CANON,RETKIND,RETWIDTH,SUPPORTED,NAMES as IMPORTNAMES
    from unresolved import DEFINED
    from libraryexports import SIGEPOCH
    import libraryimports,libraryexports,types,assemble
    modelsignature=types.SimpleNamespace(**assemble.load_facts('top-modelsignature-banks')['top-modelsignature-banks!'])  # exec/facts/top-modelsignature-banks.tsv
    modelcandidates=types.SimpleNamespace(**assemble.load_facts('top-modelcandidates-banks')['top-modelcandidates-banks!'])  # exec/facts/top-modelcandidates-banks.tsv
    owned={SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS}
    for module in (modelsignature,modelcandidates,libraryimports,libraryexports):
        assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40), module.__name__
    g=E.g
    # Stage control lives in libraryvariadic-result.tsv / -byte.tsv (sections s00..s13);
    # fresh labels are declared in libraryvariadic-fresh.tsv. Python binds only dynamic
    # facts: graph constants (this module's and its sources'), the parent's banks (b) and
    # the i32 code. Parent-row edits (hooks, renames to LV.original.*, aliases, the derived
    # LV.direct / LV.typedemit rows) and the narrow-integer promotion branch are
    # declared in libraryvariadic-template.tsv.
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules,install_template
    import importlib
    root=Path(__file__).parent
    bindings={'i32':next(code for name,code,_,_,_ in integers if name=='i32')}
    import types
    valueranks=types.SimpleNamespace(**{r['name']:r['value'] for r in _vrf('valueranks') if r['kind']=='bank'})   # exec/facts/valueranks.tsv banks
    for mn in ('libraryvariadic','libraryimports','libraryexports','valueranks','unresolved','modelsignature','modelcandidates'):
        for k,v in vars(dict(modelsignature=modelsignature,modelcandidates=modelcandidates,valueranks=valueranks).get(mn) or importlib.import_module(mn)).items():
            if type(v) is int and v>=1<<40:bindings[(mn+'.' if mn!='libraryvariadic' else '')+k]=v
    for k,v in b.items():
        if type(v) is int:bindings['b.'+k]=v
    import json
    sequences={}
    for line in (root/'libraryvariadic-result.tsv').read_text().splitlines():
        if line.startswith('#'):continue
        for action in json.loads(line.split('\t')[4]):
            if action[0]=='@' and action[1].startswith('O:'):sequences[action[1]]=[('OUT',c) for c in action[1][2:].encode()]
    fresh=[l.split('\t') for l in (root/'libraryvariadic-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
        rules(g,root,'libraryvariadic',bindings,sequences,None,name)
    def tmpl(name,**facts):
        install_template(g,root,'libraryvariadic',facts,lambda kind:E.P.fresh(_Scope('LV'),kind),section=name)
    def move(old,new):tmpl('move',m=[dict(old=old,new=new)])
    def hook(name,proc):move(name,'LV.original.'+name);tmpl('hookcall',h=[dict(name=name,proc=proc)])
    def alias(new,src):tmpl('copy',c=[dict(new=new,src=src)])
    k=[dict(NAMES=NAMES,IMPORTNAMES=IMPORTNAMES)]
    hook('FPCALL','LV.indirect')
    section('s00')
    hook('CL.ok','LV.begin')
    section('s01')
    hook('CL.a2','LV.argument')
    section('s02')
    narrow={code:'LV.promoteint' for _,code,width,_,_ in integers if width<4};narrow[b['BOOL']]='LV.promoteint';narrow[b['FLT']]='LV.promotefloat'
    tmpl('narrow',n=[dict(code=c,target=t) for c,t in narrow.items()])
    section('s03')
    hook('CL.done','LV.complete')
    section('s04')
    # Replace only the callee span of stacked direct calls. The following
    # return-type query still uses the real source name and signature.
    move('CL.vdirect','LV.original.direct')
    section('s05')
    tmpl('direct',k=k)
    # Source definitions are final now, after all units. Native source-priority
    # is never delegated to the host. A source winner gets a tail redirect.
    move('LI.wrapend','LV.original.wrapend')
    section('s06')
    alias('LI.wrapend','LV.wrapend')
    section('s07')
    # The shared wrapper ordinarily reloads the function name. Site wrappers
    # instead keep their unique label, while retaining the same result rules.
    move('LI.typedemit','LV.original.typedemit')
    section('s08')
    alias('LI.typedemit','LV.typedselect')
    tmpl('typed',k=k)
    section('s09')
    move('LI.wrapnext','LV.original.wrapnext')
    section('s10')
    alias('LI.wrapnext','LV.wrapnext')
    section('s11')
    # Requests exist only for selected native sites, never source winners.
    move('LX.outer','LV.original.outer')
    section('s12')
    alias('LX.outer','LV.outerselect')
    section('s13')
