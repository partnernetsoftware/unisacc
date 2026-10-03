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
    # Stage control: libraryexports-result.tsv sections head/record/signature/storage/owner/
    # begin/ellipsis/capture/rankreturn/union/envelope/argument/call. Residue kept in
    # Python (rewrites of existing, fact-dependent states): hook(), append('FN'), the
    # FN.pfpstacked and CL.namedquery/LX.startok rewrites, and the ACCEPT interception. Python
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
    def append(state,actions):
        mode,row=g.st[state]
        for key,(n,q) in list(row.items()):row[key]=(n,g.seq(list(g.seqs[q])+actions))
    # TOP.st observes the actual storage token before NEXT drops it.
    section('storage')
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
    section('owner')
    hook('FN.params','LX.begin',always=True)
    section('begin')
    # A callback parameter's own mode does not change the enclosing ABI.
    mode,row=g.st['FN.pfpstacked']
    for key,(target,seq) in list(row.items()):
        assert list(g.seqs[seq])==[('LDI','vfn',1)]
        row[key]=(target,g.seq([]))
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
    section('envelope')
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
    section('argument')
    g.st['CL.namedquery']=g.st['LX.argumentquery']
    section('call')
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
