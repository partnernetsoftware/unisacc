"""USBIND1 injected function declarations resolved by E3 delta.
Pure resource decoding, ABI matching and wrapper emission use existing actions.
No host source parser. Parent installs this after libraryexports, before finish.
Candidate origins are selected by modelcandidates. Structural wire checks run
before source parsing; support and ABI checks apply only to referenced externals.
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(180,186))
KIND, EXTENT, WRITABLE = (i << 40 for i in range(190,193))
VARIADIC, SUPPORTED, USED = (i << 40 for i in range(310,313))
STRIDE = 6150
FORMAT, TYPEDSIG, DISPATCH, PLAN, CANON, RETKIND, RETWIDTH = (i << 40 for i in range(322,329))
BYNAME = 329 << 40

def install(E,P,b,start,integers):
    from finite_rules import install as rules
    from unresolved import DEFINED
    g=E.g
    from modelgraphequality import install as install_graph_equality
    install_graph_equality(E)
    rules(g,Path(__file__).parent,'libraryimports',section='read')
    P('LI.fail').a(E.rej('not covered: library import binding or signature')).goto('DEAD')
    P('LI.start').a(('SBCLR',),*[('SBOUT',x) for x in b'\0library/bindings'],('SBFIND','li_blob'),('BLEN','li_len','li_blob')).branch({1:start},'LI.present',[('CMPI','li_len',0)])
    from modelcandidates import install as install_candidates
    install_candidates(E)
    P('LI.present').a(('COPYW','mc_blob','li_blob'),('COPYW','mc_len','li_len')).call('MC.normalize').a(('COPYW','li_blob','mc_blob'),('COPYW','li_len','mc_len'),('INPUSH','li_blob'),('COPYW','li_limit','li_len')).goto('LI.magic0')
    for i,c in enumerate(b'USBIND'):
        P('LI.magic'+str(i)).call('LI.byte').branch({c:'LI.magic'+str(i+1)},'LI.fail',[('RLD','li_byte')])
    P('LI.magic6').call('LI.byte').branch({49:'LI.version1',51:'LI.version3'},'LI.fail',[('RLD','li_byte')])
    P('LI.version1').a(('LDI','li_version',1)).goto('LI.magic7')
    P('LI.version3').a(('LDI','li_version',3)).goto('LI.magic7')
    P('LI.magic7').call('LI.byte').branch({10:'LI.count'},'LI.fail',[('RLD','li_byte')])
    P('LI.count').call('LI.u64').a(('COPYW','li_count','li_value'),('LDI','li_i',0)).branch({2:'LI.fail'},'LI.record',[('LDI','li_cmpmax',1024),('C64U','li_count','li_cmpmax')])
    P('LI.record').branch({1:'LI.inputend'},'LI.recordlength',[('CMP','li_i','li_count')])
    P('LI.recordlength').a(('COPYW','li_limit','li_len')).call('LI.u64').a(('MARK','li_pos'),('A64','add','li_end','li_pos','li_value')).branch({0:'LI.fail'},'LI.recordend',[('C64U','li_end','li_pos')])
    P('LI.recordend').branch({2:'LI.fail'},'LI.namelen',[('C64U','li_end','li_len')])
    P('LI.namelen').a(('COPYW','li_limit','li_end')).call('LI.u64').a(('COPYW','li_n','li_value'),('LDI','li_j',0),('SBCLR',)).branch({1:'LI.fail'},'LI.namemax',[('LDI','li_cmpmax',0),('C64U','li_n','li_cmpmax')])
    P('LI.namemax').branch({2:'LI.fail'},'LI.name',[('LDI','li_cmpmax',1024),('C64U','li_n','li_cmpmax')])
    P('LI.name').branch({1:'LI.named'},'LI.namebyte',[('CMP','li_j','li_n')])
    P('LI.namebyte').call('LI.byte').branch({x:'LI.nameok.'+str(x) for x in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'},'LI.namedigit',[('RLD','li_byte')])
    P('LI.namedigit').branch({1:'LI.fail'},'LI.digit',[('CMPI','li_j',0)])
    P('LI.digit').branch({x:'LI.nameok.'+str(x) for x in range(48,58)},'LI.fail',[('RLD','li_byte')])
    for c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
        P('LI.nameok.'+str(c)).a(('SBOUT',c),('ALUI','add','li_j','li_j',1)).goto('LI.name')
    P('LI.named').a(('SBSAVE','li_name'),('SBINTERN','li_id'),('LDX','li_seen','li_id',SEEN)).branch({1:'LI.newname'},'LI.fail',[('CMPI','li_seen',0)])
    P('LI.newname').a(('LDI','li_one',1),('STX','li_id',SEEN,'li_one'),('STX','li_i',NAMES,'li_name'),('STX','li_i',IDS,'li_id'),('ALUI','add','li_byname','li_i',1),('STX','li_id',BYNAME,'li_byname')).goto('LI.kind')
    P('LI.kind').call('LI.byte').branch({0:'LI.kindput',1:'LI.kindput'},'LI.fail',[('RLD','li_byte')])
    P('LI.kindput').a(('STX','li_i',KIND,'li_byte')).goto('LI.origin')
    P('LI.origin').call('LI.byte').branch({0:'LI.originversion'},'LI.fail',[('RLD','li_byte')])
    P('LI.originversion').branch({1:'LI.ordinal'},'LI.abi',[('CMPI','li_version',3)])
    P('LI.ordinal').call('LI.u64').branch({1:'LI.abi'},'LI.fail',[('CMPI','li_value',0)])
    P('LI.abi').call('LI.byte').branch({0:'LI.variadic'},'LI.fail',[('RLD','li_byte')])
    P('LI.variadic').call('LI.byte').a(('STX','li_i',VARIADIC,'li_byte')).goto('LI.address')
    P('LI.address').call('LI.u64').branch({2:'LI.addrok'},'LI.fail',[('LDI','li_cmpmax',0),('C64U','li_value','li_cmpmax')])
    P('LI.addrok').a(('STX','li_i',ADDRESS,'li_value')).call('LI.u64').a(('COPYW','li_argc','li_value'),('STX','li_i',ARGC,'li_value')).branch({2:'LI.fail'},'LI.formatversion',[('LDI','li_cmpmax',1024),('C64U','li_argc','li_cmpmax')])
    P('LI.formatversion').a(('LDI','li_format',0),('STX','li_i',FORMAT,'li_format')).branch({1:'LI.formatread'},'LI.descbegin',[('CMPI','li_version',3)])
    P('LI.formatread').call('LI.byte').a(('COPYW','li_format','li_byte'),('STX','li_i',FORMAT,'li_byte')).branch({0:'LI.descbegin',1:'LI.typeddispatch',2:'LI.typeddispatch'},'LI.fail',[('RLD','li_byte')])
    P('LI.typeddispatch').call('LI.u64').a(('STX','li_i',DISPATCH,'li_value')).call('LI.u64').a(('STX','li_i',PLAN,'li_value')).call('LI.u64').a(('COPYW','li_typedlen','li_value'),('LDI','li_typedat',0),('LDI','li_zero',0),('OCUT','li_typedold','li_zero')).goto('LI.typedbytes')
    P('LI.typedbytes').branch({0:'LI.typedbyte'},'LI.typedend',[('CMP','li_typedat','li_typedlen')])
    P('LI.typedbyte').call('LI.byte').a(('OUTW','li_byte'),('ALUI','add','li_typedat','li_typedat',1)).goto('LI.typedbytes')
    P('LI.typedend').a(('OCUT','li_typedsig','li_zero'),('STX','li_i',TYPEDSIG,'li_typedsig'),('INPUSH','li_typedold'),('XLEN','li_typedat'),('SPAN2','li_zero','li_typedat'),('INPOP',),('COPYW','ms_blob','li_typedsig'),('COPYW','ms_len','li_typedlen')).call('MS.canonical').a(('STX','li_i',CANON,'ms_canon'),('STX','li_i',RETKIND,'ms_resultkind'),('STX','li_i',RETWIDTH,'ms_resultwidth')).branch({1:'LI.typedcount'},'LI.fail',[('CMP','ms_nameid','li_id')])
    P('LI.typedcount').branch({1:'LI.supported'},'LI.fail',[('CMP','ms_count','li_argc')])
    P('LI.descbegin').a(('LDI','li_d',0),('LDI','li_field',0)).goto('LI.desc')
    P('LI.desc').call('LI.u64').a(('ALUI','mul','li_index','li_i',STRIDE),('ALUI','mul','li_t','li_d',6),('ALU','add','li_index','li_index','li_t'),('ALU','add','li_index','li_index','li_field'),('STX','li_index',DESC,'li_value'),('ALUI','add','li_field','li_field',1)).branch({1:'LI.descnext'},'LI.desc',[('CMPI','li_field',6)])
    P('LI.descnext').a(('LDI','li_field',0),('ALUI','add','li_d','li_d',1),('CMP','li_d','li_argc')).branch({2:'LI.kindtail'},'LI.desc',[('CMP','li_d','li_argc')])
    P('LI.kindtail').a(('LDX','li_kind','li_i',KIND)).branch({1:'LD.tail'},'LI.supported',[('CMPI','li_kind',1)])
    P('LI.supported').call('LI.byte').a(('STX','li_i',SUPPORTED,'li_byte')).goto('LI.bound')
    P('LI.bound').a(('MARK','li_pos')).branch({1:'LI.nextrecord'},'LI.fail',[('C64U','li_pos','li_end')])
    P('LI.nextrecord').a(('ALUI','add','li_i','li_i',1)).goto('LI.record')
    P('LI.inputend').a(('MARK','li_pos')).branch({1:'LI.ready'},'LI.fail',[('C64U','li_pos','li_len')])
    P('LI.ready').a(('INPOP',),('LDI','li_emitted',0)).goto(start)
    # A parsed typed native prototype is a real callable declaration. Do not
    # mark missing prototypes callable, or source definitions as host wrappers.
    g.st['LI.original.prototype']=g.st.pop('FN.pr1');g.labels.add('LI.original.prototype')
    # A forward prototype needs the same stacked calling mode as its later
    # definition; this is not specific to an injected native binding.
    from libraryexports import VARIADIC as PROTOTYPEVAR
    P('FN.pr1').a(('INTERN','li_protoid','fns','fne'),('LDX','li_modesig','li_protoid',b['FPS_FN']),('LDX','li_modecount','li_modesig',b['FPS_COUNT']),('LDX','li_modevar','li_modesig',PROTOTYPEVAR),('LDI','li_modestack',0)).branch({2:'LI.forwardstack'},'LI.forwardvar',[('CMPI','li_modecount',6)])
    P('LI.forwardvar').branch({1:'LI.forwardstack'},'LI.forwardput',[('CMPI','li_modevar',1)])
    P('LI.forwardstack').a(('LDI','li_modestack',1)).goto('LI.forwardput')
    P('LI.forwardput').a(('STX','li_protoid',E.VAR,'li_modestack'),('LDX','li_protoindex','li_protoid',BYNAME)).branch({1:'LI.original.prototype'},'LI.prototypebinding',[('CMPI','li_protoindex',0)])
    P('LI.prototypebinding').a(('ALUI','sub','li_protoindex','li_protoindex',1),('LDX','li_protoformat','li_protoindex',FORMAT)).branch({1:'LI.prototypesignature',2:'LI.prototypesignature'},'LI.original.prototype',[('RLD','li_protoformat')])
    P('LI.prototypesignature').a(('LDX','li_protosig','li_protoid',b['FPS_FN']),('LDI','li_protoone',1),('STX','li_protoid',E.FND,'li_protoone'),('LDX','li_protoret','li_protosig',b['FPS_RD']),('STX','li_protoid',E.FRD,'li_protoret'),('LDX','li_protoret','li_protosig',b['FPS_RB']),('STX','li_protoid',E.FRB,'li_protoret'),('LDX','li_protocount','li_protosig',b['FPS_COUNT']),('LDI','li_protomode',0)).branch({2:'LI.prototypestack'},'LI.prototypeput',[('CMPI','li_protocount',6)])
    P('LI.prototypestack').a(('LDI','li_protomode',1)).goto('LI.prototypeput')
    P('LI.prototypeput').a(('LDX','li_protovar','li_protosig',PROTOTYPEVAR)).branch({1:'LI.prototypevar'},'LI.prototypefinal',[('CMPI','li_protovar',1)])
    P('LI.prototypevar').a(('LDI','li_protomode',1)).goto('LI.prototypefinal')
    P('LI.prototypefinal').a(('STX','li_protoid',E.VAR,'li_protomode')).goto('LI.original.prototype')
    # Fixed typed imports query the complete prototype pool for conversion.
    # The legacy PDB first-eight query default-promotes an absent ninth float
    # to double, changing its raw slot. Reuse the existing conversion machine
    # with the real declaration instead of creating a second classifier.
    from libraryexports import PARAMMARK, PARAMDEPTH, PARAMBASE, SIGEPOCH, SEEN as SOURCEDEFS
    g.st['LI.original.argumentquery']=g.st.pop('CL.namedquery');g.labels.add('LI.original.argumentquery')
    P('CL.namedquery').a(('LDX','li_argsource','fid',SOURCEDEFS)).branch({1:'LI.original.argumentquery'},'LI.argumentbinding',[('CMPI','li_argsource',1)])
    P('LI.argumentbinding').a(('LDX','li_argbinding','fid',BYNAME)).branch({1:'LI.original.argumentquery'},'LI.argumentformat',[('CMPI','li_argbinding',0)])
    P('LI.argumentformat').a(('ALUI','sub','li_argbinding','li_argbinding',1),('LDX','li_argformat','li_argbinding',FORMAT)).branch({1:'LI.argumenttyped',2:'LI.argumentvariadic'},'LI.original.argumentquery',[('RLD','li_argformat')])
    P('LI.argumentvariadic').a(('LDX','li_argsig','fid',b['FPS_FN']),('LDX','li_argcount','li_argsig',b['FPS_COUNT'])).branch({0:'LI.argumentpool'},'CL.default',[('CMP','na','li_argcount')])
    P('LI.argumenttyped').a(('LDX','li_argsig','fid',b['FPS_FN']),('LDX','li_argcount','li_argsig',b['FPS_COUNT'])).branch({0:'LI.argumentpool'},'LI.fail',[('CMP','na','li_argcount')])
    P('LI.argumentpool').a(('ALUI','mul','li_argindex','li_argsig',1024),('ALU','add','li_argindex','li_argindex','na'),('LDX','li_argmark','li_argindex',PARAMMARK),('LDX','li_argepoch','li_argsig',SIGEPOCH)).branch({1:'LI.argumenttype'},'LI.fail',[('CMP','li_argmark','li_argepoch')])
    P('LI.argumenttype').a(('LDX','t','li_argindex',PARAMDEPTH),('ALUI','mul','t','t',4096),('LDX','li_argbase','li_argindex',PARAMBASE),('ALU','add','t','t','li_argbase'),('CMPI','t',0)).goto('CL.signature')
    # Every fixed typed native call supplies exactly the declared slots. A
    # deficit cannot be repaired by a wrapper reading past the caller's frame.
    from libraryexports import SEEN as SOURCEDEFS, VARIADIC as SOURCEVAR
    g.st['LI.original.callcomplete']=g.st.pop('CL.done');g.labels.add('LI.original.callcomplete')
    P('CL.done').a(('LDX','li_completebinding','fid',BYNAME)).branch({1:'LI.original.callcomplete'},'LI.completeformat',[('CMPI','li_completebinding',0)])
    P('LI.completeformat').a(('ALUI','sub','li_completebinding','li_completebinding',1),('LDX','li_completeformat','li_completebinding',FORMAT)).branch({1:'LI.completefixed'},'LI.original.callcomplete',[('CMPI','li_completeformat',1)])
    P('LI.completefixed').a(('LDX','li_completesource','fid',SOURCEDEFS)).branch({1:'LI.original.callcomplete'},'LI.completesignature',[('CMPI','li_completesource',1)])
    P('LI.completesignature').a(('LDX','li_completesig','fid',b['FPS_FN']),('LDX','li_completevar','li_completesig',SOURCEVAR)).branch({1:'LI.original.callcomplete'},'LI.completecount',[('CMPI','li_completevar',1)])
    P('LI.completecount').a(('LDX','li_completecount','li_completesig',b['FPS_COUNT'])).branch({1:'LI.original.callcomplete'},'LI.fail',[('CMP','na','li_completecount')])
    # Aggregate native call expressions return their stable result-buffer address.
    # The old aggregate return branch accepts only a local lvalue; preserve it
    # for every other input and use normal EXPR/COPYSTRUCT for this declared call.
    g.st['LI.original.structreturn']=g.st.pop('S.rs');g.labels.add('LI.original.structreturn')
    P('S.rs').branch({1:'LI.structreturnname'},'LI.original.structreturn',[('CMPI','tk',E.TK_ID)])
    P('LI.structreturnname').a(('INTERN','li_returnid','ps','pe'),('LDX','li_returnindex','li_returnid',BYNAME)).branch({1:'LI.original.structreturn'},'LI.structreturnformat',[('CMPI','li_returnindex',0)])
    P('LI.structreturnformat').a(('ALUI','sub','li_returnindex','li_returnindex',1),('LDX','li_returnformat','li_returnindex',FORMAT)).branch({1:'LI.structreturnexpr',2:'LI.structreturnexpr'},'LI.original.structreturn',[('RLD','li_returnformat')])
    P('LI.structreturnexpr').call('EXPR').branch({1:'LI.structreturntype'},'LI.fail',[('CMPI','vt',0)])
    P('LI.structreturntype').branch({1:'LI.structreturnsemicolon'},'LI.fail',[('CMP','vb','rb')])
    P('LI.structreturnsemicolon').branch({1:'LI.structreturncopy'},'LI.fail',[('CMPI','tk',E.TK[';'])])
    P('LI.structreturncopy').o('  mov r1, r0\n  .lea r0, __rv_').a(('SPAN2','fns','fne')).o('\n').call('WCOPY').goto('S.rj')
    # UD has already collected actual source labels here. Reconstruct the tape
    # with real wrapper definitions, then its ordinary unresolved call scan.
    original=g.st.pop('UD.calls0');g.st['LI.original.calls0']=original;g.labels.add('LI.original.calls0')
    P('UD.calls0').branch({1:'LI.original.calls0'},'LI.firstwrap',[('CMPI','li_len',0)])
    P('LI.firstwrap').branch({1:'LI.original.calls0'},'LI.wrapstart',[('CMPI','li_emitted',1)])
    P('LI.wrapstart').a(('INPOP',),('INPUSH','ud_tape'),('SPAN2','ud_zero','ud_len'),('INPOP',),('LDI','li_i',0)).call('LI.usedscan').goto('LI.wrap')
    P('LI.wrap').branch({1:'LI.wrapend'},'LI.lookup',[('CMP','li_i','li_count')])
    P('LI.lookup').a(('LDX','li_kind','li_i',KIND)).branch({1:'LD.validate'},'LI.functionlookup',[('CMPI','li_kind',1)])
    P('LI.functionlookup').a(('LDX','li_id','li_i',IDS),('LDX','li_def','li_id',DEFINED)).branch({1:'LI.wrapnext'},'LI.usedcheck',[('CMPI','li_def',1)])
    P('LI.usedcheck').a(('LDX','li_used','li_id',USED)).branch({1:'LI.importsupport'},'LI.wrapnext',[('CMPI','li_used',1)])
    P('LI.importsupport').a(('LDX','li_supported','li_i',SUPPORTED)).branch({1:'LI.importvar'},'LI.fail',[('CMPI','li_supported',1)])
    P('LI.importvar').a(('LDX','li_format','li_i',FORMAT)).branch({2:'LI.wrapnext'},'LI.importfixed',[('RLD','li_format')])
    P('LI.importfixed').a(('LDX','li_var','li_i',VARIADIC)).branch({1:'LI.importargc'},'LI.fail',[('CMPI','li_var',0)])
    P('LI.importargc').a(('LDX','li_argc','li_i',ARGC),('LDX','li_format','li_i',FORMAT)).branch({1:'LI.typedprototype'},'LI.legacyargc',[('CMPI','li_format',1)])
    P('LI.legacyargc').branch({2:'LI.fail'},'LI.prototype',[('LDI','li_cmpmax',6),('C64U','li_argc','li_cmpmax')])
    P('LI.typedprototype').a(('LDX','lx_sig','li_id',b['FPS_FN']),('LDX','lx_nameblob','li_i',NAMES)).branch({1:'LI.fail'},'LI.typedsource',[('CMPI','lx_sig',0)])
    P('LI.typedsource').call('LX.signature').a(('COPYW','ms_blob','lx_sigblob'),('BLEN','ms_len','lx_sigblob')).call('MS.canonical').a(('COPYW','li_sourcemode','ms_mode'),('COPYW','mg_left_blob','lx_sigblob'),('BLEN','mg_left_len','lx_sigblob'),('LDX','mg_right_blob','li_i',TYPEDSIG),('BLEN','mg_right_len','mg_right_blob')).call('MG.compatible').branch({1:'LI.typedemit'},'LI.fail',[('CMPI','mg_equal',1)])
    P('LI.prototype').a(('LDX','li_sig','li_id',b['FPS_FN']),('LDX','li_argc','li_i',ARGC)).branch({1:'LI.fail'},'LI.variadiccheck',[('CMPI','li_sig',0)])
    P('LI.variadiccheck').a(('LDX','li_var','li_id',E.VAR)).branch({1:'LI.countmatch'},'LI.fail',[('CMPI','li_var',0)])
    P('LI.countmatch').a(('LDX','li_n','li_sig',b['FPS_COUNT'])).branch({1:'LI.return'},'LI.fail',[('CMP','li_n','li_argc')])
    P('LI.return').a(('LDX','li_depth','li_sig',b['FPS_RD']),('LDX','li_base','li_sig',b['FPS_RB']),('LDI','li_d',0)).goto('LI.classify')
    P('LI.classify').a(('LDI','li_class',0),('LDI','li_width',0),('LDI','li_uns',0)).branch({1:'LI.scalar'},'LI.pointer',[('CMPI','li_depth',0)])
    P('LI.pointer').branch({b['FPB']:'LI.fail',b['FPV']:'LI.fail'},'LI.pointertyped',[('RLD','li_base')])
    P('LI.pointertyped').branch({0:'LI.pointerok'},'LI.pointerupper',[('CMPI','li_base',b['FPS_FIRST'])])
    P('LI.pointerupper').branch({0:'LI.fail'},'LI.pointerok',[('CMPI','li_base',b['SBB'])])
    P('LI.pointerok').a(('LDI','li_class',2),('LDI','li_width',8)).goto('LI.match')
    P('LI.scalar').branch({code:'LI.integer.'+str(code) for _,code,_,_,_ in integers},'LI.other',[('RLD','li_base')])
    for _,code,width,uns,_ in integers:
        P('LI.integer.'+str(code)).a(('LDI','li_class',1),('LDI','li_width',width),('LDI','li_uns',uns)).goto('LI.match')
    P('LI.other').branch({0:'LI.void',b['BOOL']:'LI.bool'},'LI.fail',[('RLD','li_base')])
    P('LI.void').branch({1:'LI.match'},'LI.fail',[('CMPI','li_d',0)])
    P('LI.bool').a(('LDI','li_class',1),('LDI','li_width',1),('LDI','li_uns',1)).goto('LI.match')
    P('LI.match').a(('ALUI','mul','li_index','li_i',STRIDE),('ALUI','mul','li_t','li_d',6),('ALU','add','li_index','li_index','li_t')).goto('LI.match0')
    for field,register,nx in [(0,'li_depth','LI.match3'),(3,'li_class','LI.match4'),(4,'li_width','LI.match5'),(5,'li_uns','LI.matched')]:
        P('LI.match'+str(field)).a(('ALUI','add','li_t','li_index',field),('LDX','li_value','li_t',DESC)).branch({1:nx},'LI.fail',[('C64U','li_value',register)])
    P('LI.matched').branch({1:'LI.save_return'},'LI.paramnext',[('CMPI','li_d',0)])
    P('LI.save_return').a(('COPYW','li_retbase','li_base'),('COPYW','li_retwidth','li_width'),('COPYW','li_retuns','li_uns'),('COPYW','li_retclass','li_class')).goto('LI.paramnext')
    P('LI.paramnext').a(('ALUI','add','li_d','li_d',1)).branch({2:'LI.emit'},'LI.paramread',[('CMP','li_d','li_argc')])
    P('LI.paramread').a(('ALUI','mul','li_t','li_sig',16),('ALU','add','li_t','li_t','li_d'),('ALUI','sub','li_t','li_t',1),('LDX','li_value','li_t',b['FPS_PARAM']),('ALUI','div','li_depth','li_value',4096),('ALUI','rem','li_base','li_value',4096)).goto('LI.classify')
    # Typed wrappers marshal raw script slots into the declared dispatcher.
    # Source and selected native TypeGraphs were compared in delta above.
    P('LI.typedemit').a(('LDX','li_retclass','li_i',RETKIND),('LDX','li_retwidth','li_i',RETWIDTH),('LDX','li_name','li_i',NAMES),('ALUI','mul','li_frame','li_argc',8),('ALUI','add','li_frame','li_frame',56),('INPUSH','li_name'),('XLEN','li_n'),('SPAN2','ud_zero','li_n'),('INPOP',)).o(':\n  .frame 8\n  store64 [r7+0], r6\n  mov r6, r7\n  .frame ').a(('COPYW','li_print','li_frame')).call('LI.print64').o('\n').branch({1:'LI.typedstackstart'},'LI.typedregister0',[('CMPI','li_sourcemode',1)])
    for j in range(6):
        P('LI.typedregister'+str(j)).branch({2:'LI.typedregisterput'+str(j)},'LI.typedcontrol',[('CMPI','li_argc',j)])
        P('LI.typedregisterput'+str(j)).o('  store64 [r7+%d], r%d\n'%(48+8*j,j)).goto('LI.typedregister'+str(j+1) if j<5 else 'LI.typedcontrol')
    P('LI.typedstackstart').a(('LDI','li_argindex',0)).goto('LI.typedstack')
    P('LI.typedstack').branch({0:'LI.typedstackput'},'LI.typedcontrol',[('CMP','li_argindex','li_argc')])
    P('LI.typedstackput').o('  load64 r2, [r6+').a(('ALUI','mul','li_print','li_argindex',8),('ALUI','add','li_print','li_print',16)).call('LI.print64').o(']\n  store64 [r7+').a(('ALUI','mul','li_print','li_argindex',8),('ALUI','add','li_print','li_print',48)).call('LI.print64').o('], r2\n').a(('ALUI','add','li_argindex','li_argindex',1)).goto('LI.typedstack')
    P('LI.typedcontrol').o('  imm r1, ').a(('LDX','li_print','li_i',PLAN)).call('LI.print64').o('\n  store64 [r7+0], r1\n  imm r1, 48\n  add64 r1, r7, r1\n  store64 [r7+8], r1\n').branch({1:'LI.typedaggregate'},'LI.typedscalarbuffer',[('CMPI','li_retclass',5)])
    P('LI.typedaggregate').o('  .lea r1, __rv_').a(('INPUSH','li_name'),('XLEN','li_n'),('SPAN2','ud_zero','li_n'),('INPOP',)).o('\n').goto('LI.typedresultpointer')
    P('LI.typedscalarbuffer').o('  imm r1, ').a(('ALUI','sub','li_print','li_frame',8)).call('LI.print64').o('\n  add64 r1, r7, r1\n').goto('LI.typedresultpointer')
    P('LI.typedresultpointer').o('  store64 [r7+16], r1\n  imm r1, ').a(('COPYW','li_print','li_argc')).call('LI.print64').o('\n  store64 [r7+24], r1\n  imm r1, ').call('LV.siteword').o('\n  store64 [r7+32], r1\n  imm r1, 0\n  store64 [r7+40], r1\n  imm r1, ').a(('LDX','li_print','li_i',DISPATCH)).call('LI.print64').o('\n  mov r0, r7\n  .librarycall r1, r0\n').branch({0:'LI.typedvoid',5:'LI.typedaggregatereturn'},'LI.typedscalarreturn',[('RLD','li_retclass')])
    P('LI.typedvoid').o('  imm r0, 0\n').goto('LI.typedtail')
    P('LI.typedaggregatereturn').o('  load64 r0, [r7+16]\n').goto('LI.typedtail')
    P('LI.typedscalarreturn').o('  load64 r1, [r7+16]\n  load64 r0, [r1+0]\n').goto('LI.typedtail')
    P('LI.typedtail').o('  .frame -').a(('COPYW','li_print','li_frame')).call('LI.print64').o('\n  load64 r6, [r7+0]\n  .frame -8\n  ret\n').branch({1:'LI.typedbss'},'LI.wrapnext',[('CMPI','li_retclass',5)])
    P('LI.typedbss').o('.bss __rv_').a(('INPUSH','li_name'),('XLEN','li_n'),('SPAN2','ud_zero','li_n'),('INPOP',)).o(' ').a(('COPYW','li_print','li_retwidth')).call('LI.print64').o('\n').goto('LI.wrapnext')
    p=P('LI.emit').a(('LDX','li_name','li_i',NAMES),('INPUSH','li_name'),('XLEN','li_n'),('SPAN2','ud_zero','li_n'),('INPOP',)).o(':\n  .frame 48\n')
    for j in range(6):p.o('  store64 [r7+%d], r%d\n'%(8*j,j))
    p.o('  imm r1, ').a(('LDX','li_print','li_i',ADDRESS)).call('LI.print64').o('\n  mov r0, r7\n  .librarycall r1, r0\n').goto('LI.retclass')
    # Addresses retain all 64 bits; the ordinary PRN helper is a 32-bit printer.
    P('LI.print64').a(('LDI','li_pk',0)).goto('LI.printdigit')
    P('LI.printdigit').a(('A64I','urem','li_pd','li_print',10),('A64I','udiv','li_print','li_print',10),('STX','li_pk',186 << 40,'li_pd'),('ALUI','add','li_pk','li_pk',1)).branch({1:'LI.printout'},'LI.printdigit',[('CMPI','li_print',0)])
    P('LI.printout').a(('ALUI','sub','li_pk','li_pk',1),('LDX','li_pd','li_pk',186 << 40),('ALUI','add','li_pd','li_pd',48),('OUTW','li_pd')).branch({1:'RET'},'LI.printout',[('CMPI','li_pk',0)])
    P('LI.retclass').branch({1:'LI.retinteger'},'LI.tail',[('CMPI','li_retclass',1)])
    P('LI.retinteger').branch({b['BOOL']:'LI.retbool'},'LI.retwidth',[('RLD','li_retbase')])
    P('LI.retbool').o('  imm r1, 0\n  ne r0, r0, r1\n').goto('LI.tail')
    P('LI.retwidth').branch({w:'LI.retwidth.'+str(w) for w in (1,2,4)},'LI.tail',[('RLD','li_retwidth')])
    for w in (1,2,4):
        P('LI.retwidth.'+str(w)).o('  imm r1, %d\n  and64 r0, r0, r1\n'%((1<<(8*w))-1)).branch({1:'LI.retsign.'+str(w)},'LI.tail',[('CMPI','li_retuns',0)])
        P('LI.retsign.'+str(w)).o('  imm r1, %d\n  xor64 r0, r0, r1\n  sub64 r0, r0, r1\n'%(1<<(8*w-1))).goto('LI.tail')
    P('LI.tail').o('  .frame -48\n  ret\n').goto('LI.wrapnext')
    P('LI.wrapnext').a(('ALUI','add','li_i','li_i',1)).goto('LI.wrap')
    P('LI.wrapend').a(('OCUT','ud_tape','ud_zero'),('BLEN','ud_len','ud_tape'),('INPUSH','ud_tape'),('LDI','li_emitted',1)).goto('UD.labels')
    P('LI.scanstart').a(('INPUSH','ud_tape'),('LDI','lis_flag',0),('LDI','lis_last',0),('LDI','lis_first',0)).goto('LI.scanbyte')
    letters=b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
    def bb(state,cases,other):
        for key,nx in cases.items():g.on(state,[key],nx,[],'b')
        g.on(state,[x for x in range(257) if x not in cases],other,[],'b')
    bb('LI.scanbyte',{256:'LI.scanend',10:'LI.scanline',**{c:'LI.scanword' for c in letters}},'LI.scanadv')
    P('LI.scanadv').a(('ADV',)).goto('LI.scanbyte')
    P('LI.scanword').a(('MARK','lis_start')).goto('LI.scanwordbyte')
    bb('LI.scanwordbyte',{c:'LI.scanwordadv' for c in letters+b'0123456789'},'LI.scanwordend')
    P('LI.scanwordadv').a(('ADV',)).goto('LI.scanwordbyte')
    P('LI.scanwordend').a(('MARK','lis_end'),('INTERN','lis_last','lis_start','lis_end')).branch({1:'LI.scanopcode'},'LI.scanbyte',[('CMPI','lis_first',0)])
    P('LI.scanopcode').a(('LDI','lis_first',1)).branch({1:'LI.scancall'},'LI.scanlea',[('CMP','lis_last','lis_call')])
    P('LI.scanlea').branch({1:'LI.scancall'},'LI.scanbyte',[('CMP','lis_last','lis_lea')])
    P('LI.scancall').a(('LDI','lis_flag',1)).goto('LI.scanbyte')
    P('LI.scanline').branch({1:'LI.scanmark'},'LI.scanreset',[('CMPI','lis_flag',1)])
    P('LI.scanmark').a(('STX','lis_last',USED,'lis_flag')).goto('LI.scanreset')
    P('LI.scanreset').a(('LDI','lis_flag',0),('LDI','lis_last',0),('LDI','lis_first',0),('ADV',)).goto('LI.scanbyte')
    P('LI.scanend').a(('INPOP',)).ret()
    # Intern the two opcode words once before the scan; caller has no live SB.
    pp=P('LI.usedscan')
    for text,reg in [('call','lis_call'),('lea','lis_lea')]:pp.a(('SBCLR',),*[('SBOUT',c) for c in text.encode()],('SBINTERN',reg))
    pp.goto('LI.scanstart')
    from librarydata import install as data_install
    data_install(E,P,b,integers)
    return 'LI.start'
