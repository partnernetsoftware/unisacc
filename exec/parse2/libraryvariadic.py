"""One-pass, model-owned promoted variadic call graphs and source priority.
The host receives only final length-framed declarations; no source decisions.
"""
SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS=(i<<40 for i in range(400,408))
def install(E,P,b,integers):
    from libraryexports import RETURNRANK, PARAMRANK
    from valueranks import LVSITERANK
    from libraryimports import BYNAME,FORMAT,IDS,PLAN,DISPATCH,CANON,RETKIND,RETWIDTH,SUPPORTED,NAMES as IMPORTNAMES
    from unresolved import DEFINED
    from libraryexports import SIGEPOCH
    import modelsignature,modelcandidates,libraryimports,libraryexports
    owned={SITEFN,SITEBIND,COUNT,DEPTH,BASE,SHAPE,NAMES,REQUESTS}
    for module in (modelsignature,modelcandidates,libraryimports,libraryexports):
        assert not owned.intersection(v for v in vars(module).values() if type(v) is int and v>=1<<40), module.__name__
    g=E.g
    def hook(name,proc):
        old='LV.original.'+name;g.st[old]=g.st.pop(name);g.labels.add(old)
        alias='LV.hook.'+name
        P(alias).a(('PUSH',old)).goto(proc);g.st[name]=g.st[alias]
    # Stage control lives in libraryvariadic-result.tsv / -byte.tsv (sections s00..s13);
    # fresh labels are declared in libraryvariadic-fresh.tsv. Python binds only dynamic
    # facts: graph constants (this module's and its sources'), the parent's banks (b) and
    # the i32 code. Residue: parent-row edits (hook, pops/aliases of FPCALL, CL.ok, CL.a2,
    # CL.done, CL.vdirect, LI.wrapend, LI.typedemit, LI.wrapnext, LX.outer; the derived
    # LV.direct and LV.typedemit rows rewrite the parents' own action sequences) and the
    # narrow-integer promotion branch (keys come from the integers list).
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules
    import importlib
    root=Path(__file__).parent
    bindings={'i32':next(code for name,code,_,_,_ in integers if name=='i32')}
    for mn in ('libraryvariadic','libraryimports','libraryexports','valueranks','unresolved','modelsignature','modelcandidates'):
        for k,v in vars(importlib.import_module(mn)).items():
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
    hook('FPCALL','LV.indirect')
    section('s00')
    hook('CL.ok','LV.begin')
    section('s01')
    hook('CL.a2','LV.argument')
    section('s02')
    narrow={code:'LV.promoteint' for _,code,width,_,_ in integers if width<4};narrow[b['BOOL']]='LV.promoteint';narrow[b['FLT']]='LV.promotefloat'
    P('LV.tailscalar').branch(narrow,'LV.argstore',[('RLD','vb')])
    section('s03')
    hook('CL.done','LV.complete')
    section('s04')
    # Replace only the callee span of stacked direct calls. The following
    # return-type query still uses the real source name and signature.
    old='LV.original.direct';g.st[old]=g.st.pop('CL.vdirect');g.labels.add(old)
    section('s05')
    mode,row=g.st[old];assert mode=='r' and len(set(row.values()))==1
    _,seq=next(iter(row.values()));actions=list(g.seqs[seq]);at=actions.index(('SPAN2','cls','cle'))
    actions[at:at+1]=[('LDX','lv_name','lv_site',NAMES),('INPUSH','lv_name'),('XLEN','lv_len'),('SPAN2','lx_zero','lv_len'),('INPOP',)]
    g.st['LV.direct']=(mode,{k:(nx,g.seq(actions)) for k,(nx,q) in row.items()})
    # Source definitions are final now, after all units. Native source-priority
    # is never delegated to the host. A source winner gets a tail redirect.
    g.st['LV.original.wrapend']=g.st.pop('LI.wrapend');g.labels.add('LV.original.wrapend')
    section('s06')
    g.st['LI.wrapend']=g.st['LV.wrapend']
    section('s07')
    # The shared wrapper ordinarily reloads the function name. Site wrappers
    # instead keep their unique label, while retaining the same result rules.
    g.st['LV.original.typedemit']=g.st.pop('LI.typedemit');g.labels.add('LV.original.typedemit')
    section('s08')
    g.st['LI.typedemit']=g.st['LV.typedselect']
    mode,row=g.st['LV.original.typedemit'];old=('LDX','li_name','li_i',IMPORTNAMES)
    g.st['LV.typedemit']=(mode,{k:(nx,g.seq([('LDX','li_name','lv_iter',NAMES) if x==old else x for x in g.seqs[q]])) for k,(nx,q) in row.items()})
    section('s09')
    g.st['LV.original.wrapnext']=g.st.pop('LI.wrapnext');g.labels.add('LV.original.wrapnext')
    section('s10')
    g.st['LI.wrapnext']=g.st['LV.wrapnext']
    section('s11')
    # Requests exist only for selected native sites, never source winners.
    g.st['LV.original.outer']=g.st.pop('LX.outer');g.labels.add('LV.original.outer')
    section('s12')
    g.st['LX.outer']=g.st['LV.outerselect']
    section('s13')
