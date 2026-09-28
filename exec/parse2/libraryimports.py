"""USBIND1 injected function declarations resolved by E3 delta.
Pure resource decoding, ABI matching and wrapper emission use existing actions.
No host source parser. Parent installs this after libraryexports, before finish.
Data, loaded/dynamic origins, callbacks and unsupported signatures reject.
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(180,186))

def install(E,P,b,start,integers):
    from finite_rules import install as rules
    from unresolved import DEFINED
    g=E.g
    rules(g,Path(__file__).parent,'libraryimports',section='read')
    P('LI.fail').a(E.rej('not covered: library import binding or signature')).goto('DEAD')
    P('LI.start').a(('SBCLR',),*[('SBOUT',x) for x in b'\0library/bindings'],('SBFIND','li_blob'),('BLEN','li_len','li_blob')).branch({1:start},'LI.present',[('CMPI','li_len',0)])
    P('LI.present').a(('INPUSH','li_blob'),('COPYW','li_limit','li_len')) .goto('LI.magic0')
    for i,c in enumerate(b'USBIND1\n'):
        P('LI.magic'+str(i)).call('LI.byte').branch({c:'LI.magic'+str(i+1) if i<7 else 'LI.count'},'LI.fail',[('RLD','li_byte')])
    P('LI.count').call('LI.u64').a(('COPYW','li_count','li_value'),('LDI','li_i',0)).branch({2:'LI.fail'},'LI.record',[('CMPI','li_count',1024)])
    P('LI.record').branch({1:'LI.inputend'},'LI.recordlength',[('CMP','li_i','li_count')])
    P('LI.recordlength').a(('COPYW','li_limit','li_len')).call('LI.u64').a(('MARK','li_pos'),('A64','add','li_end','li_pos','li_value')).branch({0:'LI.fail'},'LI.recordend',[('CMP','li_end','li_pos')])
    P('LI.recordend').branch({2:'LI.fail'},'LI.namelen',[('CMP','li_end','li_len')])
    P('LI.namelen').a(('COPYW','li_limit','li_end')).call('LI.u64').a(('COPYW','li_n','li_value'),('LDI','li_j',0),('SBCLR',)).branch({1:'LI.fail'},'LI.namemax',[('CMPI','li_n',0)])
    P('LI.namemax').branch({2:'LI.fail'},'LI.name',[('CMPI','li_n',1024)])
    P('LI.name').branch({1:'LI.named'},'LI.namebyte',[('CMP','li_j','li_n')])
    P('LI.namebyte').call('LI.byte').branch({x:'LI.nameok.'+str(x) for x in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'},'LI.namedigit',[('RLD','li_byte')])
    P('LI.namedigit').branch({1:'LI.fail'},'LI.digit',[('CMPI','li_j',0)])
    P('LI.digit').branch({x:'LI.nameok.'+str(x) for x in range(48,58)},'LI.fail',[('RLD','li_byte')])
    for c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
        P('LI.nameok.'+str(c)).a(('SBOUT',c),('ALUI','add','li_j','li_j',1)).goto('LI.name')
    P('LI.named').a(('SBSAVE','li_name'),('SBINTERN','li_id'),('LDX','li_seen','li_id',SEEN)).branch({1:'LI.newname'},'LI.fail',[('CMPI','li_seen',0)])
    P('LI.newname').a(('LDI','li_one',1),('STX','li_id',SEEN,'li_one'),('STX','li_i',NAMES,'li_name'),('STX','li_i',IDS,'li_id')).goto('LI.kind')
    for state,nx in [('kind','origin'),('origin','abi'),('abi','variadic'),('variadic','address')]:
        P('LI.'+state).call('LI.byte').branch({0:'LI.'+nx},'LI.fail',[('RLD','li_byte')])
    P('LI.address').call('LI.u64').branch({2:'LI.addrok'},'LI.fail',[('CMPI','li_value',0)])
    P('LI.addrok').a(('STX','li_i',ADDRESS,'li_value')).call('LI.u64').a(('COPYW','li_argc','li_value'),('STX','li_i',ARGC,'li_value')).branch({2:'LI.fail'},'LI.descbegin',[('CMPI','li_argc',6)])
    P('LI.descbegin').a(('LDI','li_d',0),('LDI','li_field',0)).goto('LI.desc')
    P('LI.desc').call('LI.u64').a(('ALUI','mul','li_index','li_i',42),('ALUI','mul','li_t','li_d',6),('ALU','add','li_index','li_index','li_t'),('ALU','add','li_index','li_index','li_field'),('STX','li_index',DESC,'li_value'),('ALUI','add','li_field','li_field',1)).branch({1:'LI.descnext'},'LI.desc',[('CMPI','li_field',6)])
    P('LI.descnext').a(('LDI','li_field',0),('ALUI','add','li_d','li_d',1),('CMP','li_d','li_argc')).branch({2:'LI.supported'},'LI.desc',[('CMP','li_d','li_argc')])
    P('LI.supported').call('LI.byte').branch({1:'LI.bound'},'LI.fail',[('RLD','li_byte')])
    P('LI.bound').a(('MARK','li_pos')).branch({1:'LI.nextrecord'},'LI.fail',[('CMP','li_pos','li_end')])
    P('LI.nextrecord').a(('ALUI','add','li_i','li_i',1)).goto('LI.record')
    P('LI.inputend').a(('MARK','li_pos')).branch({1:'LI.ready'},'LI.fail',[('CMP','li_pos','li_len')])
    P('LI.ready').a(('INPOP',),('LDI','li_emitted',0)).goto(start)
    # UD has already collected actual source labels here. Reconstruct the tape
    # with real wrapper definitions, then its ordinary unresolved call scan.
    original=g.st.pop('UD.calls0');g.st['LI.original.calls0']=original;g.labels.add('LI.original.calls0')
    P('UD.calls0').branch({1:'LI.original.calls0'},'LI.firstwrap',[('CMPI','li_len',0)])
    P('LI.firstwrap').branch({1:'LI.original.calls0'},'LI.wrapstart',[('CMPI','li_emitted',1)])
    P('LI.wrapstart').a(('INPOP',),('INPUSH','ud_tape'),('SPAN2','ud_zero','ud_len'),('INPOP',),('LDI','li_i',0)).goto('LI.wrap')
    P('LI.wrap').branch({1:'LI.wrapend'},'LI.lookup',[('CMP','li_i','li_count')])
    P('LI.lookup').a(('LDX','li_id','li_i',IDS),('LDX','li_def','li_id',DEFINED)).branch({1:'LI.wrapnext'},'LI.prototype',[('CMPI','li_def',1)])
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
    P('LI.match').a(('ALUI','mul','li_index','li_i',42),('ALUI','mul','li_t','li_d',6),('ALU','add','li_index','li_index','li_t')).goto('LI.match0')
    for field,register,nx in [(0,'li_depth','LI.match3'),(3,'li_class','LI.match4'),(4,'li_width','LI.match5'),(5,'li_uns','LI.matched')]:
        P('LI.match'+str(field)).a(('ALUI','add','li_t','li_index',field),('LDX','li_value','li_t',DESC)).branch({1:nx},'LI.fail',[('CMP','li_value',register)])
    P('LI.matched').branch({1:'LI.save_return'},'LI.paramnext',[('CMPI','li_d',0)])
    P('LI.save_return').a(('COPYW','li_retbase','li_base'),('COPYW','li_retwidth','li_width'),('COPYW','li_retuns','li_uns'),('COPYW','li_retclass','li_class')).goto('LI.paramnext')
    P('LI.paramnext').a(('ALUI','add','li_d','li_d',1)).branch({2:'LI.emit'},'LI.paramread',[('CMP','li_d','li_argc')])
    P('LI.paramread').a(('ALUI','mul','li_t','li_sig',16),('ALU','add','li_t','li_t','li_d'),('ALUI','sub','li_t','li_t',1),('LDX','li_value','li_t',b['FPS_PARAM']),('ALUI','div','li_depth','li_value',4096),('ALUI','rem','li_base','li_value',4096)).goto('LI.classify')
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
    return 'LI.start'
