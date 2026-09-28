"""Borrowed writable scalar data: declaration facts and address selection in δ.
Only resource-enabled externs get deferred addresses; real definitions win later.
"""
EXTERN, DEPTH, BASE, SHAPE = (i << 40 for i in range(193,197))
def global_address(E,P,entry,classic,done):
    E.g.labels.add(entry+'.emit')
    P(entry).branch({1:classic},entry+'.library',[('CMPI','li_len',0)])
    P(entry+'.library').a(('INTERN','ld_v','ips','ipe'),('LDX','ld_ext','ld_v',EXTERN)).branch({1:entry+'.import'},classic,[('CMPI','ld_ext',1)])
    P(entry+'.import').a(('LDX','ld_depth','ld_v',DEPTH),('LDX','ld_base','ld_v',BASE),('LDI','ld_class',0),('PUSH',entry+'.emit')).goto('LD.classify')
    p=P(entry+'.emit').o('  .libraryaddr r0, g_').a(('SPAN2','ips','ipe')).o(', ').a(('COPYW','n','ld_depth')).call('PRN').o(', ').a(('COPYW','n','ld_class')).call('PRN').o(', ').a(('COPYW','n','ld_width')).call('PRN').o(', ').a(('COPYW','n','ld_uns')).call('PRN').o('\n').goto(done)

def install(E,P,b,integers):
    from libraryimports import KIND, EXTENT, WRITABLE, IDS, DESC
    g=E.g
    g.labels.add('LD.match')
    def prefix(state,acts):
        mode,row=g.st[state]
        for key,(n,q) in list(row.items()):row[key]=(n,g.seq(acts+list(g.seqs[q])))
    def redirect(state,procedure):
        orig='LD.original.'+state;g.st[orig]=g.st.pop(state);g.labels.add(orig)
        P(state).branch({1:orig},'LD.hook.'+state,[('CMPI','li_len',0)])
        P('LD.hook.'+state).a(('PUSH',orig)).goto(procedure)
    # FN is the shared entry to function and global declarations.
    prefix('TOP.st',[('COPYW','ld_storage','tk')])
    prefix('FN',[('COPYW','ld_decl','ld_storage'),('LDI','ld_storage',0)])
    redirect('GV.storage','LD.storagecheck')
    P('LD.storagecheck').branch({1:'LD.storageextern'},'RET',[('CMPI','ld_decl',E.TK['type=extern'])])
    P('LD.storageextern').branch({1:'RET'},'LD.skipstorage',[('CMPI','tk',E.TK['='])])
    # An extern without initializer records type but creates no .bss.
    P('LD.skipstorage').a(('POP',)).goto('GV.record')
    redirect('GV.record','LD.record')
    P('LD.record').a(('INTERN','ld_v','fns','fne'),('LDI','ld_ext',0)).branch({1:'LD.recordextern'},'LD.save',[('CMPI','ld_decl',E.TK['type=extern'])])
    P('LD.recordextern').branch({1:'LD.save'},'LD.noinit',[('CMPI','tk',E.TK['='])])
    P('LD.noinit').a(('LDX','ld_old','ld_v',EXTERN),('LDX','ld_loc','ld_v',E.LOC)).branch({1:'LD.existing'},'LD.newextern',[('CMPI','ld_loc',E.GMARK)])
    P('LD.newextern').a(('LDI','ld_ext',1),('LDI','ld_epoch',-1),('STX','ld_v',b['GUNIT'],'ld_epoch')).goto('LD.save')
    P('LD.existing').branch({1:'LD.newextern'},'LD.save',[('CMPI','ld_old',1)])
    P('LD.save').a(('STX','ld_v',EXTERN,'ld_ext'),('STX','ld_v',DEPTH,'td'),('STX','ld_v',BASE,'tb'),('STX','ld_v',SHAPE,'gar')).ret()
    # The complete registry decoder checks a data record's fixed structure.
    P('LD.tail').branch({1:'LD.extent'},'LI.fail',[('CMPI','li_argc',0)])
    P('LD.extent').call('LI.u64').branch({2:'LD.extentok'},'LI.fail',[('CMPI','li_value',0)])
    P('LD.extentok').a(('STX','li_i',EXTENT,'li_value'),('LDX','ld_address','li_i',__import__('libraryimports').ADDRESS),('A64','add','ld_end','ld_address','li_value')).branch({0:'LI.fail',1:'LI.fail'},'LD.extentnowrap',[('C64U','ld_end','ld_address')])
    P('LD.extentnowrap').call('LI.byte').branch({1:'LD.writable'},'LI.fail',[('RLD','li_byte')])
    P('LD.writable').a(('STX','li_i',WRITABLE,'li_byte'),('ALUI','mul','ld_index','li_i',42),('LDX','ld_depth','ld_index',DESC),('ALUI','add','ld_t','ld_index',3),('LDX','ld_class','ld_t',DESC),('ALUI','add','ld_t','ld_index',4),('LDX','ld_width','ld_t',DESC),('ALUI','add','ld_t','ld_index',5),('LDX','ld_uns','ld_t',DESC)).goto('LD.wirebound0')
    P('LD.wireinteger').branch({1:'LD.wirewidth'},'LI.fail',[('CMPI','ld_depth',0)])
    P('LD.wirewidth').branch({w:'LD.wireunsigned' for w in (1,2,4,8)},'LI.fail',[('RLD','ld_width')])
    P('LD.wireunsigned').branch({0:'LD.wireextent',1:'LD.wireextent'},'LI.fail',[('RLD','ld_uns')])
    P('LD.wirepointer').branch({2:'LD.wirepointerwidth'},'LI.fail',[('CMPI','ld_depth',0)])
    P('LD.wirepointerwidth').branch({1:'LD.wirepointeruns'},'LI.fail',[('CMPI','ld_width',8)])
    P('LD.wirepointeruns').branch({1:'LD.wireextent'},'LI.fail',[('CMPI','ld_uns',0)])
    P('LD.wireextent').a(('LDX','ld_extent','li_i',EXTENT)).branch({0:'LI.fail'},'LI.supported',[('C64U','ld_extent','ld_width')])
    P('LD.validate').a(('LDX','ld_v','li_i',IDS),('LDX','ld_ext','ld_v',EXTERN)).branch({1:'LD.validateextern'},'LI.wrapnext',[('CMPI','ld_ext',1)])
    P('LD.validateextern').a(('LDX','ld_shape','ld_v',SHAPE)).branch({1:'LD.types'},'LI.fail',[('CMPI','ld_shape',0)])
    P('LD.types').a(('LDX','ld_depth','ld_v',DEPTH),('LDX','ld_base','ld_v',BASE),('PUSH','LD.match')).goto('LD.classify')
    P('LD.classify').a(('LDI','ld_uns',0),('LDI','ld_width',8)).branch({1:'LD.scalar'},'LD.pointer',[('CMPI','ld_depth',0)])
    P('LD.pointer').branch({b['FPB']:'LI.fail',b['FPV']:'LI.fail'},'LD.pointerrange',[('RLD','ld_base')])
    P('LD.pointerrange').branch({0:'LD.pointerok'},'LD.pointerupper',[('CMPI','ld_base',b['FPS_FIRST'])])
    P('LD.pointerupper').branch({0:'LI.fail'},'LD.pointerok',[('CMPI','ld_base',b['SBB'])])
    P('LD.pointerok').a(('LDI','ld_class',2)).ret()
    P('LD.scalar').branch({code:'LD.int.'+str(code) for _,code,_,_,_ in integers},'LD.bool',[('RLD','ld_base')])
    for _,code,width,uns,_ in integers:P('LD.int.'+str(code)).a(('LDI','ld_class',1),('LDI','ld_width',width),('LDI','ld_uns',uns)).ret()
    P('LD.bool').branch({b['BOOL']:'LD.boolok'},'LI.fail',[('RLD','ld_base')])
    P('LD.boolok').a(('LDI','ld_class',1),('LDI','ld_width',1),('LDI','ld_uns',1)).ret()
    P('LD.match').a(('ALUI','mul','ld_index','li_i',42)).goto('LD.match0')
    for f,r,nx in [(0,'ld_depth','LD.match3'),(3,'ld_class','LD.match4'),(4,'ld_width','LD.match5'),(5,'ld_uns','LD.extentmatch')]:
        P('LD.match'+str(f)).a(('ALUI','add','ld_t','ld_index',f),('LDX','ld_value','ld_t',DESC)).branch({1:nx},'LI.fail',[('C64U','ld_value',r)])
    P('LD.extentmatch').a(('LDX','ld_extent','li_i',EXTENT)).branch({0:'LI.fail'},'LI.wrapnext',[('C64U','ld_extent','ld_width')])

    P('LD.wirebound0').branch({2:'LI.fail'},'LD.wirebound1',[('LDI','ld_max',2147483647),('C64U','ld_depth','ld_max')])

    P('LD.wirebound1').branch({2:'LI.fail'},'LD.wirebound2',[('LDI','ld_max',2),('C64U','ld_class','ld_max')])

    P('LD.wirebound2').branch({2:'LI.fail'},'LD.wirebound3',[('LDI','ld_max',8),('C64U','ld_width','ld_max')])

    P('LD.wirebound3').branch({2:'LI.fail'},'LD.wireselect',[('LDI','ld_max',1),('C64U','ld_uns','ld_max')])
    P('LD.wireselect').branch({1:'LD.wireinteger',2:'LD.wirepointer'},'LI.fail',[('RLD','ld_class')])
