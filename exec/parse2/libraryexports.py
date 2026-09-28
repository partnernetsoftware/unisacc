"""Library-only signature records from E3's existing typed descriptors.
No host parser or signature guess. Default tape is unchanged. Metadata is a
length-framed outer envelope; prune/lower see only its separated tape payload.
"""
from pathlib import Path
RECORDS, SEEN, VARIADIC, FPPRESENT, FPDEPTH, FPBASE, FPSHAPE = (i << 40 for i in (72,73,74,75,76,77,78))

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
    def hook(state,procedure):
        orig='LX.original.'+state;g.st[orig]=g.st.pop(state)
        g.labels.add(orig)
        P(state).branch({1:orig},'LX.hook.'+state,[('CMPI','lx_present',0)])
        P('LX.hook.'+state).a(('PUSH',orig)).goto(procedure)
    hook('TOP.st','LX.storage')
    append('FN',[('COPYW','lx_linkage','lx_storage'),('LDI','lx_storage',0)])
    # Store true syntactic ellipsis before FN.many selects stacked ABI for >6.
    append('FN.body',[('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN']),
                      ('STX','lx_sig',VARIADIC,'vfn')])
    # A named function-pointer parameter's legacy allocation skips SIG.store.
    # Capture its already parsed descriptor without changing parser type facts.
    capture=[('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN']),
             ('ALUI','mul','lx_index','lx_sig',16),('ALU','add','lx_index','lx_index','pk'),
             ('LDI','lx_one',1),('STX','lx_index',FPPRESENT,'lx_one'),
             ('STX','lx_index',FPDEPTH,'td'),('STX','lx_index',FPBASE,'tb'),
             ('STX','lx_index',FPSHAPE,'type_shape')]
    append('FN.pfpdecl1',capture)
    reset=[('INTERN','lx_id','fns','fne'),('LDX','lx_sig','lx_id',b['FPS_FN']),('LDI','lx_zero',0)]
    for i in range(8):reset += [('ALUI','mul','lx_index','lx_sig',16),('ALUI','add','lx_index','lx_index',i),('STX','lx_index',FPPRESENT,'lx_zero')]
    append('FN.fnplain',reset)
    hook('FN.def1','LX.definition')
    P('LX.definition').a(('INTERN','lx_id','fns','fne'),('LDX','lx_seen','lx_id',SEEN)).branch(
        {1:'LX.record'},'LX.fail',[('CMPI','lx_seen',0)])
    p=P('LX.record').a(('LDI','lx_one',1),('STX','lx_id',SEEN,'lx_one'),('LDI','lx_zero',0),
       ('OCUT','lx_tape','lx_zero'),('LDX','lx_sig','lx_id',b['FPS_FN']),
       ('LDX','lx_nparams','lx_sig',b['FPS_COUNT']),('LDX','lx_var','lx_sig',VARIADIC),
       ('LDI','lx_supported',1))
    p.branch({1:'LX.externalrecord'},'LX.unsupportedlink',[('CMPI','lx_linkage',0)])
    P('LX.unsupportedlink').a(('LDI','lx_supported',0)).goto('LX.externalrecord')
    P('LX.externalrecord').branch({1:'LX.paramlimit'},'LX.unsupportedvar',[('CMPI','lx_var',0)])
    P('LX.unsupportedvar').a(('LDI','lx_supported',0)).goto('LX.paramlimit')
    P('LX.paramlimit').branch({2:'LX.unsupportedcount'},'LX.recordfields',[('CMPI','lx_nparams',6)])
    P('LX.unsupportedcount').a(('LDI','lx_supported',0)).goto('LX.recordfields')
    # Supported status is appended after every descriptor has been classified.
    p=P('LX.recordfields').a(('ALU','sub','lx_v','fne','fns')).call('LX.u64').a(('SPAN2','fns','fne'),
       ('OUTW','lx_linkage'),('LDI','lx_one',1),('OUTW','lx_one'),('OUTW','lx_var'))
    p.a(('COPYW','lx_v','lx_nparams')).call('LX.u64')
    p.a(('LDX','lx_depth','lx_sig',b['FPS_RD']),('LDX','lx_base','lx_sig',b['FPS_RB']),
        ('LDX','lx_shape','lx_sig',b['FPS_RSH']),('LDI','lx_return',1)).call('LX.descriptor')
    P(p.cur).branch({2:'LX.truncate'},'LX.storecount',[('CMPI','lx_nparams',8)])
    P('LX.truncate').a(('LDI','lx_stored',8)).goto('LX.writecount')
    P('LX.storecount').a(('COPYW','lx_stored','lx_nparams')).goto('LX.writecount')
    P('LX.writecount').a(('COPYW','lx_v','lx_stored')).call('LX.u64').a(('LDI','lx_i',0),('LDI','lx_return',0)).goto('LX.params')
    P('LX.params').branch({0:'LX.param'},'LX.recordend',[('CMP','lx_i','lx_stored')])
    p=P('LX.param').a(('ALUI','mul','lx_index','lx_sig',16),('ALU','add','lx_index','lx_index','lx_i'),
        ('LDX','lx_type','lx_index',b['FPS_PARAM']),('ALUI','div','lx_depth','lx_type',4096),
        ('ALUI','rem','lx_base','lx_type',4096),('LDX','lx_shape','lx_index',b['FPS_PSH']),
        ('LDX','lx_one','lx_index',FPPRESENT)).branch({1:'LX.paramfp'},'LX.paramout',[('CMPI','lx_one',1)])
    P('LX.paramfp').a(('LDX','lx_depth','lx_index',FPDEPTH),('LDX','lx_base','lx_index',FPBASE),('LDX','lx_shape','lx_index',FPSHAPE)).goto('LX.paramout')
    P('LX.paramout').call('LX.descriptor').a(('ALUI','add','lx_i','lx_i',1)).goto('LX.params')
    P('LX.recordend').a(('OUTW','lx_supported'),('OCUT','lx_blob','lx_zero'),
        ('STX','lx_count',RECORDS,'lx_blob'),('ALUI','add','lx_count','lx_count',1),
        ('INPUSH','lx_tape'),('XLEN','lx_end'),('SPAN2','lx_zero','lx_end'),('INPOP',)).ret()
    # Exact primitive facts come from tyinfo. Composite categories use E3 ABI codes.
    P('LX.descriptor').a(('LDI','lx_class',6),('LDI','lx_width',0),('LDI','lx_unsigned',0)).branch(
        {1:'LX.scalar'},'LX.pointer',[('CMPI','lx_depth',0)])
    P('LX.pointer').branch({1:'LX.functionpointer'},'LX.pointerupper',[('CMPI','lx_base',b['FPB'])])
    P('LX.pointerupper').branch({1:'LX.functionpointer'},'LX.pointertyped',[('CMPI','lx_base',b['FPV'])])
    P('LX.pointertyped').branch({0:'LX.datapointer'},'LX.pointertypedupper',[('CMPI','lx_base',b['FPS_FIRST'])])
    P('LX.pointertypedupper').branch({0:'LX.functionpointer'},'LX.datapointer',[('CMPI','lx_base',b['SBB'])])
    P('LX.datapointer').a(('LDI','lx_class',2),('LDI','lx_width',8)).goto('LX.descriptorout')
    P('LX.functionpointer').a(('LDI','lx_class',4),('LDI','lx_width',8),('LDI','lx_supported',0)).goto('LX.descriptorout')
    P('LX.scalar').branch({code:'LX.integer.'+str(code) for _,code,_,_,_ in integers},'LX.other',[('RLD','lx_base')])
    for _,code,size,uns,_ in integers:
        P('LX.integer.'+str(code)).a(('LDI','lx_class',1),('LDI','lx_width',size),('LDI','lx_unsigned',uns)).goto('LX.descriptorout')
    P('LX.other').branch({0:'LX.void',b['BOOL']:'LX.bool',b['DBL']:'LX.double',b['FLT']:'LX.float',b['FPB']:'LX.functionpointer',b['FPV']:'LX.functionpointer'},'LX.composite',[('RLD','lx_base')])
    P('LX.void').a(('LDI','lx_class',0)).branch({1:'LX.descriptorout'},'LX.missingparameter',[('CMPI','lx_return',1)])
    P('LX.missingparameter').a(('LDI','lx_class',6)).goto('LX.unsupported')
    P('LX.bool').a(('LDI','lx_class',1),('LDI','lx_width',1),('LDI','lx_unsigned',1)).goto('LX.descriptorout')
    P('LX.double').a(('LDI','lx_class',3),('LDI','lx_width',8)).goto('LX.unsupported')
    P('LX.float').a(('LDI','lx_class',3),('LDI','lx_width',4)).goto('LX.unsupported')
    P('LX.composite').branch({0:'LX.enumcheck'},'LX.aggregate',[('CMPI','lx_base',b['SBB'])])
    P('LX.aggregate').a(('LDI','lx_class',5)).goto('LX.unsupported')
    P('LX.enumcheck').branch({0:'LX.unsupported'},'LX.enumupper',[('CMPI','lx_base',b['ENUM_FIRST'])])
    P('LX.enumupper').branch({0:'LX.enum'},'LX.functionpointer',[('CMPI','lx_base',b['FPS_FIRST'])])
    P('LX.enum').a(('LDI','lx_class',1),('LDI','lx_width',next(size for name,code,size,uns,narrow in integers if name=='i32'))).goto('LX.descriptorout')
    P('LX.unsupported').a(('LDI','lx_supported',0)).goto('LX.descriptorout')
    p=P('LX.descriptorout')
    for register in ('lx_depth','lx_base','lx_shape','lx_class','lx_width','lx_unsigned'):
        p.a(('COPYW','lx_v',register)).call('LX.u64')
    p.ret()
    # Intercept successful acceptance only, preserving its prior output actions.
    for state,(mode,row) in list(g.st.items()):
        for k,(n,q) in list(row.items()):
            seq=list(g.seqs[q])
            if seq and seq[-1][0]=='ACCEPT':row[k]=('LX.wrap',g.seq(seq[:-1]))
    P('LX.wrap').branch({1:'LX.accept'},'LX.envelope',[('CMPI','lx_present',0)])
    p=P('LX.envelope').a(('LDI','lx_zero',0),('OCUT','lx_tape','lx_zero')).o('USLSIG1\n').a(('COPYW','lx_v','lx_count')).call('LX.u64')
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
    module_start=module_install(E,P,start)
    mode,row=g.st['LX.startok']
    for k,(n,q) in list(row.items()):
        assert n==start
        row[k]=(module_start,q)
    return 'LX.start'
