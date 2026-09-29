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
    def common(entry,left,right,dest):
        P(entry).branch({2:entry+'.left'},entry+'.right',[('CMP',left,right)])
        P(entry+'.left').a(('COPYW',dest,left)).ret()
        P(entry+'.right').a(('COPYW',dest,right)).ret()
    common('VR.binary','left_rank','sv_rank','value_rank')
    common('VR.conditional','left_rank','value_rank','qt_common_rank')
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
    def hook(state,proc):
        old='VR.original.'+state;assert old not in g.st
        g.st[old]=g.st.pop(state);g.labels.add(old)
        alias='VR.hook.'+state;P(alias).call(proc).goto(old);g.st[state]=g.st[alias]
    # Conditional branch identities are computed before conversion temporaries.
    hook('QT.original','VR.conditional')
    hook('QT.merge','VR.qmerge')
    P('VR.qmerge').a(('COPYW','value_rank','qt_common_rank')).ret()
    hook('CL.typed','VR.fixed')
    P('VR.fixed').a(('LDI','vr_fixedrank',0)).branch({1:'VR.namedfixed'},'VR.fixedsig',[('CMPI','call_sig',0)])
    P('VR.namedfixed').a(('LDX','vr_sig','fid',b['FPS_FN'])).goto('VR.fixedindex')
    P('VR.fixedsig').a(('COPYW','vr_sig','call_sig')).goto('VR.fixedindex')
    P('VR.fixedindex').a(('ALUI','mul','vr_index','vr_sig',1024),('ALU','add','vr_index','vr_index','na'),('LDX','vr_fixedrank','vr_index',PARAMRANK)).ret()
    # CL.typed owns a conversion target distinct from its saved source value.
    old='VR.original.CL.a2';g.st[old]=g.st.pop('CL.a2');g.labels.add(old)
    P('VR.a2').branch({1:'VR.a2fixed'},old,[('CMPI','vr_hasfixed',1)])
    P('VR.a2fixed').a(('COPYW','value_rank','vr_fixedrank'),('LDI','vr_hasfixed',0)).goto(old)
    g.st['CL.a2']=g.st['VR.a2']
