"""Semantic FP identity accompanies existing value descriptors/actions.
Storage remains unchanged. Pointer descriptors carry underlying identity only
internally; call-site serialization emits rank zero for non-FP descriptors.
"""
VALUEBANK,NAMEDRETURN,LVSITERANK,LCSITERANK,COMMONBANK=(i<<40 for i in range(647,652))
RANKOF={'tb':'type_rank','fp_tb':'fp_rank','rb':'return_rank','vb':'value_rank','lb':'left_rank','rvb':'right_rank','svb':'sv_rank','cast_tb':'cast_rank','ivb':'iv_rank','qn_base':'qn_rank','bc_targetvb':'bc_target_rank'}
def slots(items):
    result=[]
    for item in items:
        result.append(item)
        companion=RANKOF.get(item)
        if companion and companion not in items:result.append(companion)
    return result

def install(E,P,b):
    from libraryexports import RETURNRANK,PARAMRANK,MEMBERRANK,TYPERANK
    import modelsignature,modelgraphequality,libraryexports,layoutfacts,libraryimports,librarycallables
    owned={VALUEBANK,NAMEDRETURN,LVSITERANK,LCSITERANK,COMMONBANK}
    for owner in (modelsignature,modelgraphequality,libraryexports,layoutfacts,libraryimports,librarycallables):
        assert not owned.intersection(v for k,v in vars(owner).items() if type(v) is int and v>=1<<40),owner.__name__
    g=E.g
    from pathlib import Path
    from finite_rules import install_template
    root=Path(__file__).parent
    holder=P('VR.fresh')  # unregistered: only names fresh labels
    def declare(section):
        install_template(g,root,'valueranks',{},holder.fresh,bindings=dict(FPS_FN=b['FPS_FN'],PARAMRANK=PARAMRANK),section=section)
    declare('common')
    # Apply companions to the already-expanded ordinary actions, preserving all
    # observations, output, and continuation labels. No executor primitive.
    poolrank={E.BASE:VALUEBANK,E.FRB:NAMEDRETURN,b['MBS']:MEMBERRANK,b['FPS_RB']:RETURNRANK}
    for state,(mode,row) in list(g.st.items()):
        if state.startswith('VR.'):continue
        for key,(n,q) in list(row.items()):
            source=list(g.seqs[q]);actions=[]
            if state=='START':
                for l in range(4):
                    for r in range(4):actions.extend([('LDI','vr_commonkey',l*4+r),('LDI','vr_commonvalue',max(l,r)),('STX','vr_commonkey',COMMONBANK,'vr_commonvalue')])
            for ix,a in enumerate(source):
                actions.append(a);op=a[0];extra=[]
                if op=='COPYW' and a[1] in RANKOF and a[2] in RANKOF:
                    extra=[('COPYW',RANKOF[a[1]],RANKOF[a[2]])]
                elif op=='LDX' and a[1] in RANKOF and a[3] in poolrank:
                    extra=[('LDX',RANKOF[a[1]],a[2],poolrank[a[3]])]
                elif op=='STX' and a[3] in RANKOF and a[2] in poolrank:
                    extra=[('STX',a[1],poolrank[a[2]],RANKOF[a[3]])]
                elif op=='LDI' and a[1]=='vb':
                    if a[2] in (b['DBL'],b['FLT']):
                        if state.startswith('OPX.'):extra=[('ALUI','mul','vr_commonkey','left_rank',4),('ALU','add','vr_commonkey','vr_commonkey','sv_rank'),('LDX','value_rank','vr_commonkey',COMMONBANK)]
                        elif state.startswith('QT.'):extra=[('COPYW','value_rank','qt_common_rank')]
                        elif state.startswith('U.') or 'ordinary_dispatch' in state:extra=[('COPYW','value_rank','df_rank')]
                    else:extra=[('LDI','value_rank',0)]
                # Existing type-rank companions are already present.
                if extra and source[ix+1:ix+1+len(extra)]!=extra:actions.extend(extra)
            row[key]=(n,g.seq(actions))
    # Hooks: each original moves to VR.original.*, an alias calls the rank
    # procedure and continues there (valueranks-template.tsv, in graph order).
    for section in ('h1','h2','h3','h4','h5','h6','h7'):declare(section)
    g.labels.update('VR.original.'+s for s in ('QT.original','QT.merge','CL.typed','CL.a2'))
