"""Opt-in ordered USLSIG3 parser projections in ordinary delta actions.
The LF topology is declaration order; source modifier and semantic FP facts
remain explicitly unknown. V2 actions and wire remain the default path.
"""
def install(E,P,b,frame,blob):
    import layoutfacts as LF
    from libraryexports import MEMBERRANK
    import gen2
    g=E.g
    def select(name,v3):
        old='L3.original.'+name
        assert old not in g.st
        g.st[old]=g.st.pop(name);g.labels.add(old)
        alias='L3.select.'+name
        P(alias).branch({3:v3},old,[('RLD','lx_wireversion')]);g.st[name]=g.st[alias]
    # Stage control lives in librarytypesv3-result.tsv / -byte.tsv (sections s0..s5);
    # fresh labels are declared in librarytypesv3-fresh.tsv. Python binds only dynamic
    # facts: layout-fact banks and limits (layoutfacts), MEMBERRANK, E.ARR, gen2.DIM, the
    # parent's banks (b), and the parent's frame save/restore sequences. Residue: select
    # (moves each V2 state aside behind a wire-version branch).
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules
    import libraryexports
    root=Path(__file__).parent
    bindings={'E.ARR':E.ARR,'gen2.DIM':gen2.DIM}
    for prefix,mod in (('LF.',LF),('libraryexports.',libraryexports)):
        for k,v in vars(mod).items():
            if type(v) is int:bindings[prefix+k]=v
    for k,v in b.items():
        if type(v) is int:bindings['b.'+k]=v
    def saved(op):
        q=_Scope('L3');frame(q,op);return q.acts
    sequences={'frame.STX':saved('STX'),'frame.LDX':saved('LDX')}
    fresh=[l.split('\t') for l in (root/'librarytypesv3-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
        rules(g,root,'librarytypesv3',bindings,sequences,None,name)
    select('LTY.aggregatemembers','L3.aggregate')
    section('s0')
    select('LTY.struct','L3.struct')
    section('s1')
    select('LTY.array','L3.array')
    section('s2')
    select('LTY.arraypayload','L3.arraypayload')
    section('s3')
    select('LTY.payload','L3.payload')
    section('s4')
    select('LTY.out','L3.out')
    section('s5')
