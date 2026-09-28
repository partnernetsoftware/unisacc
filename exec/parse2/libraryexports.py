"""Library-only signature records from E3's existing typed descriptors.
No host parser or signature guess. Default tape is unchanged. Metadata is a
length-framed outer envelope; prune/lower see only its separated tape payload.
"""
from pathlib import Path
RECORDS, SEEN, VARIADIC, PARAMMARK, PARAMDEPTH, PARAMBASE, PARAMSHAPE, UNION = (i << 40 for i in range(72,80))
SIGEPOCH = 81 << 40

def install(E,P,b,start,integers):
    from modelinput import u64
    from finite_rules import install as rules
    g=E.g
    u64(E,'LX.resource',b'\0library/symbols','lx_flag','lx_present','LX.fail')
    P('LX.start').call('LX.resource').branch({1:'LX.startok'},'LX.flag',[('CMPI','lx_present',0)])
    P('LX.flag').branch({1:'LX.startok'},'LX.fail',[('CMPI','lx_flag',1)])
    P('LX.startok').a(('LDI','lx_count',0),('LDI','lx_storage',0)).goto(start)
    P('LX.fail').a(E.rej('not covered: library signature resource or duplicate definition')).goto('DEAD')
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
    append('FN',[('COPYW','lx_linkage','lx_storage'),('LDI','lx_storage',0)])
    # Store true syntactic ellipsis before FN.many selects stacked ABI for >6.
    append('FN.body',[('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN']),
                      ('STX','lx_sig',VARIADIC,'vfn')])
    # Capture all parameters before the legacy SIG.store eight-slot limit.
    # Per-definition epoch marks avoid clearing 1024 slots on each declaration.
    append('FN.fnplain',[('ALUI','add','lx_captureepoch','lx_captureepoch',1)])
    hook('SIG.store','LX.capture',always=True)
    hook('FN.pfpdecl1','LX.capture',always=True)
    P('LX.capture').branch({0:'LX.captureok'},'LX.fail',[('CMPI','pk',1024)])
    P('LX.captureok').a(('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN']),
        ('ALUI','mul','lx_index','lx_sig',1024),('ALU','add','lx_index','lx_index','pk'),
        ('STX','lx_sig',SIGEPOCH,'lx_captureepoch'),('STX','lx_index',PARAMMARK,'lx_captureepoch'),('STX','lx_index',PARAMDEPTH,'td'),
        ('STX','lx_index',PARAMBASE,'tb'),('STX','lx_index',PARAMSHAPE,'type_shape')).ret()
    # Union identity must survive SB.end's restoration of the enclosing parser.
    hook('SB.go','LX.union')
    P('LX.union').a(('STX','sid',UNION,'sun_n')).ret()
    hook('FN.def1','LX.definition')
    P('LX.definition').branch({0:'LX.definitionok'},'LX.fail',[('CMPI','lx_count',8192)])
    P('LX.definitionok').a(('INTERN','lx_id','fns','fne'),('LDX','lx_seen','lx_id',SEEN)).branch(
        {1:'LX.record'},'LX.fail',[('CMPI','lx_seen',0)])
    p=P('LX.record').a(('LDI','lx_one',1),('STX','lx_id',SEEN,'lx_one'),('LDI','lx_zero',0),
       ('OCUT','lx_tape','lx_zero'),('LDX','lx_sig','lx_id',b['FPS_FN']),
       ('LDX','lx_epoch','lx_sig',SIGEPOCH),('LDX','lx_nparams','lx_sig',b['FPS_COUNT']),('LDX','lx_var','lx_sig',VARIADIC),
       ('LDI','lx_supported',1),('LDI','lx_mode',0),('LDI','lx_recursion',0),('LDI','lx_nodes',0))
    p.branch({1:'LX.externalrecord'},'LX.unsupportedlink',[('CMPI','lx_linkage',0)])
    P('LX.unsupportedlink').a(('LDI','lx_supported',0)).goto('LX.externalrecord')
    P('LX.externalrecord').branch({1:'LX.paramlimit'},'LX.unsupportedvar',[('CMPI','lx_var',0)])
    P('LX.unsupportedvar').a(('LDI','lx_supported',0)).goto('LX.paramlimit')
    P('LX.paramlimit').branch({2:'LX.fail'},'LX.mode',[('CMPI','lx_nparams',1024)])
    P('LX.mode').branch({2:'LX.stackmode'},'LX.varmode',[('CMPI','lx_nparams',6)])
    P('LX.varmode').branch({1:'LX.stackmode'},'LX.recordfields',[('CMPI','lx_var',1)])
    P('LX.stackmode').a(('LDI','lx_mode',1)).goto('LX.recordfields')
    # Supported status is appended after every descriptor has been classified.
    p=P('LX.recordfields').a(('ALU','sub','lx_v','fne','fns')).call('LX.u64').a(('SPAN2','fns','fne'),
       ('OUTW','lx_linkage'),('LDI','lx_one',1),('OUTW','lx_one'),('OUTW','lx_var'),('OUTW','lx_mode'))
    p.a(('COPYW','lx_v','lx_nparams')).call('LX.u64')
    p.a(('LDX','lx_depth','lx_sig',b['FPS_RD']),('LDX','lx_base','lx_sig',b['FPS_RB']),
        ('LDX','lx_shape','lx_sig',b['FPS_RSH']),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_return',1)).call('LX.descriptor')
    p.a(('COPYW','lx_v','lx_nparams')).call('LX.u64').a(('LDI','lx_i',0),('LDI','lx_return',0)).goto('LX.params')
    P('LX.params').branch({0:'LX.param'},'LX.recordend',[('CMP','lx_i','lx_nparams')])
    P('LX.param').a(('ALUI','mul','lx_index','lx_sig',1024),('ALU','add','lx_index','lx_index','lx_i'),
        ('LDX','lx_mark','lx_index',PARAMMARK)).branch({1:'LX.paramout'},'LX.fail',[('CMP','lx_mark','lx_epoch')])
    P('LX.paramout').a(('LDX','lx_depth','lx_index',PARAMDEPTH),('LDX','lx_base','lx_index',PARAMBASE),
        ('LDX','lx_shape','lx_index',PARAMSHAPE),('LDI','lx_array',0),('LDI','lx_arraybytes',0)).call('LX.descriptor').a(('ALUI','add','lx_i','lx_i',1)).goto('LX.params')
    P('LX.recordend').a(('OUTW','lx_supported'),('OCUT','lx_blob','lx_zero'),
        ('STX','lx_count',RECORDS,'lx_blob'),('ALUI','add','lx_count','lx_count',1),
        ('INPUSH','lx_tape'),('XLEN','lx_end'),('SPAN2','lx_zero','lx_end'),('INPOP',)).ret()
    from librarytypes import install as type_install
    type_install(E,P,b,integers,UNION)
    # Import prototypes serialize through the same descriptor path. Names are
    # already-owned blobs; parser source offsets no longer need remain active.
    p=P('LX.signature').a(('OCUT','lx_sigold','lx_zero'),('LDX','lx_epoch','lx_sig',SIGEPOCH),
        ('LDX','lx_nparams','lx_sig',b['FPS_COUNT']),('LDX','lx_var','lx_sig',VARIADIC),
        ('LDI','lx_supported',1),('LDI','lx_mode',0),('LDI','lx_recursion',0),('LDI','lx_nodes',0)).o('USLSIG2\n').a(('LDI','lx_v',1)).call('LX.u64').goto('LX.signaturemode')
    P('LX.signaturemode').branch({2:'LX.signaturestack'},'LX.signaturevar',[('CMPI','lx_nparams',6)])
    # The preceding procedure builder continuation must explicitly reach mode.
    P('LX.signaturevar').branch({1:'LX.signaturestack'},'LX.signaturefields',[('CMPI','lx_var',1)])
    P('LX.signaturestack').a(('LDI','lx_mode',1)).goto('LX.signaturefields')
    p=P('LX.signaturefields').a(('BLEN','lx_v','lx_nameblob')).call('LX.u64').a(('INPUSH','lx_nameblob'),('XLEN','lx_end'),('SPAN2','lx_zero','lx_end'),('INPOP',),('LDI','lx_one',1),('OUTW','lx_zero'),('OUTW','lx_one'),('OUTW','lx_var'),('OUTW','lx_mode'),('COPYW','lx_v','lx_nparams')).call('LX.u64')
    p.a(('LDX','lx_depth','lx_sig',b['FPS_RD']),('LDX','lx_base','lx_sig',b['FPS_RB']),('LDX','lx_shape','lx_sig',b['FPS_RSH']),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_return',1)).call('LX.descriptor')
    p.a(('COPYW','lx_v','lx_nparams')).call('LX.u64').a(('LDI','lx_proto_i',0)).goto('LX.prototypeparams')
    P('LX.prototypeparams').branch({0:'LX.prototypeparam'},'LX.signatureend',[('CMP','lx_proto_i','lx_nparams')])
    p=P('LX.prototypeparam').a(('ALUI','mul','lx_index','lx_sig',1024),('ALU','add','lx_index','lx_index','lx_proto_i'),('LDX','lx_mark','lx_index',PARAMMARK)).branch({1:'LX.prototypeparamout'},'LX.fail',[('CMP','lx_mark','lx_epoch')])
    P('LX.prototypeparamout').a(('LDX','lx_depth','lx_index',PARAMDEPTH),('LDX','lx_base','lx_index',PARAMBASE),('LDX','lx_shape','lx_index',PARAMSHAPE),('LDI','lx_array',0),('LDI','lx_arraybytes',0),('LDI','lx_return',0)).call('LX.descriptor').a(('ALUI','add','lx_proto_i','lx_proto_i',1)).goto('LX.prototypeparams')
    P('LX.signatureend').a(('OUTW','lx_supported'),('OCUT','lx_sigblob','lx_zero'),('INPUSH','lx_sigold'),('XLEN','lx_end'),('SPAN2','lx_zero','lx_end'),('INPOP',)).ret()
    # Intercept successful acceptance only, preserving its prior output actions.
    for state,(mode,row) in list(g.st.items()):
        for k,(n,q) in list(row.items()):
            seq=list(g.seqs[q])
            if seq and seq[-1][0]=='ACCEPT':row[k]=('LX.wrap',g.seq(seq[:-1]))
    P('LX.wrap').branch({1:'LX.accept'},'LX.envelope',[('CMPI','lx_present',0)])
    p=P('LX.envelope').a(('LDI','lx_zero',0),('OCUT','lx_tape','lx_zero')).o('USLSIG2\n').a(('COPYW','lx_v','lx_count')).call('LX.u64')
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
    module_start=module_install(E,P,imports_start)
    mode,row=g.st['LX.startok']
    for k,(n,q) in list(row.items()):
        assert n==start
        row[k]=(module_start,q)
    return 'LX.start'
