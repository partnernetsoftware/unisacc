"""Library-only signature records from E3's existing typed descriptors.
No host parser or signature guess. Default tape is unchanged. Metadata is a
length-framed outer envelope; prune/lower see only its separated tape payload.
"""
from pathlib import Path
RECORDS, SEEN, VARIADIC, PARAMMARK, PARAMDEPTH, PARAMBASE, PARAMSHAPE, UNION = (i << 40 for i in range(72,80))
SIGEPOCH = 81 << 40
TYPERANK,MEMBERRANK,RETURNRANK,PARAMRANK,PREVEPOCH,PREVRETURN,RETURNPENDING=(i<<40 for i in range(640,647))
RANK_BANKS=(TYPERANK,MEMBERRANK,RETURNRANK,PARAMRANK,PREVEPOCH,PREVRETURN,RETURNPENDING)

def install(E,P,b,start,integers):
    from modelinput import u64
    from finite_rules import install as rules
    import modelsignature, modelgraphequality, libraryimports, librarycallables, layoutfacts
    for owner in (modelsignature,modelgraphequality,libraryimports,librarycallables,layoutfacts):
        assert not set(RANK_BANKS).intersection(v for k,v in vars(owner).items() if type(v) is int and v>=1<<40), owner.__name__
    g=E.g
    u64(E,'LX.resource',b'\0library/symbols','lx_flag','lx_present','LX.fail')
    u64(E,'LX.versionresource',b'\0library/signatureversion','lx_wireversion','lx_versionpresent','LX.fail')
    # Stage control: libraryexports-result.tsv sections head/record/signature. Python
    # binds only dynamic facts: graph constants (bank ids, b[...] descriptor banks),
    # the start continuation, and fresh labels allocated in libraryexports-fresh.tsv order.
    root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'libraryexports-fresh.tsv').read_text().splitlines()[1:]]
    sequences={'out USLSIG2':E.O('USLSIG2\n'),'out USLSIG3':E.O('USLSIG3\n'),
               'reject':E.rej('not covered: library signature resource or duplicate definition')}
    constants=dict({k:v for k,v in globals().items() if type(v) is int and v>=1<<40},
                   **{k:b[k] for k in ('FPS_FN','FPS_COUNT','FPS_VAR','FPS_RD','FPS_RB','FPS_RSH')})
    def section(name):
        p=P('LX.fresh.'+name);bindings=dict(constants,start=start)
        for part,key,prefix,kind in fresh:
            if part==name:assert prefix=='LX';bindings[key]=p.fresh(kind)
        rules(g,root,'libraryexports',bindings,sequences,section=name)
    section('head')
    def append(state,actions):
        mode,row=g.st[state]
        for key,(n,q) in list(row.items()):row[key]=(n,g.seq(list(g.seqs[q])+actions))
    # TOP.st observes the actual storage token before NEXT drops it.
    P('LX.storage').branch({1:'LX.internal'},'LX.external',[('CMPI','tk',E.TK['type=static'])])
    P('LX.internal').a(('LDI','lx_storage',1)).ret()
    P('LX.external').a(('LDI','lx_storage',0)).ret()
    def hook(state,procedure,always=False):
        orig='LX.original.'+state;g.st[orig]=g.st.pop(state)
        g.labels.add(orig)
        if always:P(state).goto('LX.hook.'+state)
        else:P(state).branch({1:orig},'LX.hook.'+state,[('CMPI','lx_present',0)])
        P('LX.hook.'+state).a(('PUSH',orig)).goto(procedure)
    hook('TOP.st','LX.storage')
    hook('TOP.static','LX.storage')   # file-scope `static` takes its own state since R13-0b #31 (STATICF); linkage must still be recorded
    append('FN',[('COPYW','lx_linkage','lx_storage'),('LDI','lx_storage',0)])
    # Each parameter list owns its epoch, including nested anonymous lists.
    P('LX.owner').branch({1:'LX.ownernamed'},'LX.ownertyped',[('CMPI','sig_fp',0)])
    P('LX.ownernamed').a(('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN'])).ret()
    P('LX.ownertyped').a(('COPYW','lx_sig','sig_fp')).ret()
    hook('FN.params','LX.begin',always=True)
    P('LX.begin').call('LX.owner').a(('ALUI','add','lx_captureepoch','lx_captureepoch',1),
        ('LDX','rank_previous','lx_sig',SIGEPOCH),('STX','lx_sig',PREVEPOCH,'rank_previous'),('STX','lx_sig',SIGEPOCH,'lx_captureepoch'),('LDI','lx_ellipsis',0),
        ('STX','lx_sig',VARIADIC,'lx_ellipsis')).ret()
    # A callback parameter's own mode does not change the enclosing ABI.
    mode,row=g.st['FN.pfpstacked']
    for key,(target,seq) in list(row.items()):
        assert list(g.seqs[seq])==[('LDI','vfn',1)]
        row[key]=(target,g.seq([]))
    hook('FN.dots','LX.ellipsis',always=True)
    P('LX.ellipsis').call('LX.owner').a(('LDI','lx_ellipsis',1),('STX','lx_sig',VARIADIC,'lx_ellipsis')).ret()
    # SIG.store is reached after scalar/array/function parameter adjustment.
    hook('SIG.store','LX.capture',always=True)
    hook('FN.pfpdecl1','LX.capture',always=True)
    P('LX.capture').branch({0:'LX.captureok'},'LX.fail',[('CMPI','pk',1024)])
    P('LX.captureok').call('LX.owner').call('LX.rankcapture').a(
        ('ALUI','mul','lx_index','lx_sig',1024),('ALU','add','lx_index','lx_index','pk'),
        ('LDX','lx_ownerepoch','lx_sig',SIGEPOCH),('STX','lx_index',PARAMMARK,'lx_ownerepoch'),
        ('STX','lx_index',PARAMDEPTH,'td'),('STX','lx_index',PARAMBASE,'tb'),
        ('STX','lx_index',PARAMSHAPE,'type_shape'),('STX','lx_index',PARAMRANK,'type_rank')).ret()
    # Prior prototype slots remain readable until the current slot is replaced.
    P('LX.rankcapture').a(('ALUI','mul','rank_index','lx_sig',1024),('ALU','add','rank_index','rank_index','pk'),('LDX','rank_previous','lx_sig',PREVEPOCH)).branch({1:'RET'},'LX.rankold',[('CMPI','rank_previous',0)])
    P('LX.rankold').a(('LDX','rank_mark','rank_index',PARAMMARK)).branch({1:'LX.rankread'},'RET',[('CMP','rank_mark','rank_previous')])
    P('LX.rankread').a(('LDX','rank_old','rank_index',PARAMRANK)).branch({1:'LX.rankbase'},'FS.rank.fail',[('CMP','rank_old','type_rank')])
    P('LX.rankbase').a(('LDX','fs_l','rank_index',PARAMBASE),('COPYW','fs_r','tb')).call('FS.rank.baseequal').branch({1:'RET'},'FS.rank.fail',[('CMPI','fs_result',1)])
    # Return-callback suffixes are parsed after FN.params/FN.fpclose.
    hook('FN.bodykind','LX.rankreturn',always=True)
    P('LX.rankreturn').call('LX.owner').a(('LDX','rank_pending','lx_sig',RETURNPENDING)).branch({1:'RET'},'LX.rankreturnread',[('CMPI','rank_pending',0)])
    P('LX.rankreturnread').a(('LDX','fs_l','lx_sig',PREVRETURN),('COPYW','fs_r','rb'),('LDI','rank_zero',0),('STX','lx_sig',RETURNPENDING,'rank_zero')).call('FS.rank.baseequal').branch({1:'RET'},'FS.rank.fail',[('CMPI','fs_result',1)])
    # Union identity must survive SB.end's restoration of the enclosing parser.
    hook('SB.go','LX.union')
    P('LX.union').a(('STX','sid',UNION,'sun_n')).ret()
    hook('FN.def1','LX.definition')
    section('record')
    from librarytypes import install as type_install
    type_install(E,P,b,integers,UNION)
    # Import prototypes serialize through the same descriptor path. Names are
    # already-owned blobs; parser source offsets no longer need remain active.
    section('signature')
    # Intercept successful acceptance only, preserving its prior output actions.
    for state,(mode,row) in list(g.st.items()):
        for k,(n,q) in list(row.items()):
            seq=list(g.seqs[q])
            if seq and seq[-1][0]=='ACCEPT':row[k]=('LX.wrap',g.seq(seq[:-1]))
    P('LX.wrap').branch({1:'LX.accept'},'LX.envelope',[('CMPI','lx_present',0)])
    p=P('LX.envelope').a(('LDI','lx_zero',0),('OCUT','lx_tape','lx_zero')).call('LX.magic').a(('COPYW','lx_v','lx_count')).call('LX.u64')
    p.a(('LDI','lx_i',0)).goto('LX.records')
    P('LX.records').branch({0:'LX.copyrecord'},'LX.outer',[('CMP','lx_i','lx_count')])
    P('LX.copyrecord').a(('LDX','lx_blob','lx_i',RECORDS),('INPUSH','lx_blob'),('XLEN','lx_end'),
        ('SPAN2','lx_zero','lx_end'),('INPOP',),('ALUI','add','lx_i','lx_i',1)).goto('LX.records')
    p=P('LX.outer').a(('OCUT','lx_metadata','lx_zero')).o('USLTAPE1\n').a(('BLEN','lx_v','lx_tape')).call('LX.u64')
    p.a(('BLEN','lx_v','lx_metadata')).call('LX.u64')
    for blob in ('lx_tape','lx_metadata'):p.a(('INPUSH',blob),('XLEN','lx_end'),('SPAN2','lx_zero','lx_end'),('INPOP',))
    p.goto('LX.accept')
    rules(g,Path(__file__).parent,'libraryexports',section='write')
    from librarymodule import install as module_install
    from libraryimports import install as imports_install
    imports_start=imports_install(E,P,b,start,integers)
    # Complete parameter capture is also needed by ordinary source calls.
    # The legacy first-eight cache default-promoted a ninth fixed float.
    # Extend only the missing ninth-and-later slots. First-eight source slots
    # retain PDB, whose declarator path includes array/function decay; the
    # export descriptor captured before FN.pfpdecl1 is not that conversion.
    # Typed imports keep their separate complete-signature query below.
    # Genuinely variadic tail and absent declarations retain the old path.
    g.st['LX.original.argumentquery']=g.st.pop('CL.namedquery');g.labels.add('LX.original.argumentquery')
    P('LX.argumentquery').branch({0:'LX.original.argumentquery'},'LX.argumentlong',[('CMPI','na',8)])
    P('LX.argumentlong').a(('LDX','lx_callsig','fid',b['FPS_FN'])).branch({1:'LX.original.argumentquery'},'LX.callcount',[('CMPI','lx_callsig',0)])
    g.st['CL.namedquery']=g.st['LX.argumentquery']
    P('LX.callcount').a(('LDX','lx_callcount','lx_callsig',b['FPS_COUNT'])).branch({0:'LX.callindex'},'LX.original.argumentquery',[('CMP','na','lx_callcount')])
    P('LX.callindex').a(('ALUI','mul','lx_callindex','lx_callsig',1024),('ALU','add','lx_callindex','lx_callindex','na'),('LDX','lx_callmark','lx_callindex',PARAMMARK),('LDX','lx_callepoch','lx_callsig',SIGEPOCH)).branch({1:'LX.calltype'},'LX.original.argumentquery',[('CMP','lx_callmark','lx_callepoch')])
    P('LX.calltype').a(('LDX','t','lx_callindex',PARAMDEPTH),('ALUI','mul','t','t',4096),('LDX','lx_callbase','lx_callindex',PARAMBASE),('ALU','add','t','t','lx_callbase'),('CMPI','t',0)).goto('CL.signature')
    from libraryvariadic import install as variadic_install
    variadic_install(E,P,b,integers)
    from librarycallables import install as callable_install
    callable_start=callable_install(E,P,b,imports_start)
    module_start=module_install(E,P,callable_start)
    mode,row=g.st['LX.startok']
    for k,(n,q) in list(row.items()):
        assert n==start
        row[k]=(module_start,q)
    return 'LX.version'
