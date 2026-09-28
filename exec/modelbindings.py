"""Finite USBIND1 resource wire decoder, reusable by stage builders.
No source parsing or host-side ABI selection. Caller validates descriptors.
"""
from pathlib import Path
NAMES, IDS, ADDRESS, ARGC, DESC, SEEN = (i << 40 for i in range(190,196))
KIND, EXTENT, WRITABLE = (i << 40 for i in range(196,199))

def install(E):
    from finite_rules import install as rules
    P,g=E.P,E.g
    rules(g,Path(__file__).parent,'modelbindings',section='read')
    P('LBI.fail').a(E.rej('not covered: library import binding or signature')).goto('DEAD')
    P('LBI.read').a(('SBCLR',),*[('SBOUT',x) for x in b'\0library/bindings'],('SBFIND','lbi_blob'),('BLEN','lbi_len','lbi_blob')).branch({1:'LBI.fail'},'LBI.present',[('CMPI','lbi_len',0)])
    P('LBI.present').a(('INPUSH','lbi_blob'),('COPYW','lbi_limit','lbi_len')) .goto('LBI.magic0')
    for i,c in enumerate(b'USBIND1\n'):
        P('LBI.magic'+str(i)).call('LBI.byte').branch({c:'LBI.magic'+str(i+1) if i<7 else 'LBI.count'},'LBI.fail',[('RLD','lbi_byte')])
    P('LBI.count').call('LBI.u64').a(('COPYW','lbi_count','lbi_value'),('LDI','lbi_i',0)).branch({2:'LBI.fail'},'LBI.nonempty',[('LDI','lbi_max',1024),('C64U','lbi_count','lbi_max')])
    P('LBI.nonempty').branch({1:'LBI.fail'},'LBI.record',[('CMPI','lbi_count',0)])
    P('LBI.record').branch({1:'LBI.inputend'},'LBI.recordlength',[('CMP','lbi_i','lbi_count')])
    P('LBI.recordlength').a(('COPYW','lbi_limit','lbi_len')).call('LBI.u64').a(('MARK','lbi_pos'),('A64','add','lbi_end','lbi_pos','lbi_value')).branch({0:'LBI.fail'},'LBI.recordend',[('C64U','lbi_end','lbi_pos')])
    P('LBI.recordend').branch({2:'LBI.fail'},'LBI.namelen',[('C64U','lbi_end','lbi_len')])
    P('LBI.namelen').a(('COPYW','lbi_limit','lbi_end')).call('LBI.u64').a(('COPYW','lbi_n','lbi_value'),('LDI','lbi_j',0),('SBCLR',)).branch({1:'LBI.fail'},'LBI.namemax',[('CMPI','lbi_n',0)])
    P('LBI.namemax').branch({2:'LBI.fail'},'LBI.name',[('LDI','lbi_max',1024),('C64U','lbi_n','lbi_max')])
    P('LBI.name').branch({1:'LBI.named'},'LBI.namebyte',[('CMP','lbi_j','lbi_n')])
    P('LBI.namebyte').call('LBI.byte').branch({x:'LBI.nameok.'+str(x) for x in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'},'LBI.namedigit',[('RLD','lbi_byte')])
    P('LBI.namedigit').branch({1:'LBI.fail'},'LBI.digit',[('CMPI','lbi_j',0)])
    P('LBI.digit').branch({x:'LBI.nameok.'+str(x) for x in range(48,58)},'LBI.fail',[('RLD','lbi_byte')])
    for c in b'_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789':
        P('LBI.nameok.'+str(c)).a(('SBOUT',c),('ALUI','add','lbi_j','lbi_j',1)).goto('LBI.name')
    P('LBI.named').a(('SBSAVE','lbi_name'),('SBINTERN','lbi_id'),('LDX','lbi_seen','lbi_id',SEEN)).branch({1:'LBI.newname'},'LBI.fail',[('CMPI','lbi_seen',0)])
    P('LBI.newname').a(('LDI','lbi_one',1),('STX','lbi_id',SEEN,'lbi_one'),('STX','lbi_i',NAMES,'lbi_name'),('STX','lbi_i',IDS,'lbi_id')).goto('LBI.kind')
    P('LBI.kind').call('LBI.byte').branch({0:'LBI.kind0',1:'LBI.kind1'},'LBI.fail',[('RLD','lbi_byte')])
    for kind in (0,1):
        P('LBI.kind'+str(kind)).a(('LDI','lbi_kind',kind),('STX','lbi_i',KIND,'lbi_kind')).goto('LBI.origin')
    for state,nx in [('origin','abi'),('abi','variadic'),('variadic','address')]:
        P('LBI.'+state).call('LBI.byte').branch({0:'LBI.'+nx},'LBI.fail',[('RLD','lbi_byte')])
    P('LBI.address').call('LBI.u64').branch({2:'LBI.addressmax'},'LBI.fail',[('LDI','lbi_zero',0),('C64U','lbi_value','lbi_zero')])
    P('LBI.addressmax').branch({2:'LBI.fail'},'LBI.addrok',[('LDI','lbi_max',9223372036854775807),('C64U','lbi_value','lbi_max')])
    P('LBI.addrok').a(('STX','lbi_i',ADDRESS,'lbi_value')).call('LBI.u64').a(('COPYW','lbi_argc','lbi_value'),('STX','lbi_i',ARGC,'lbi_value')).branch({2:'LBI.fail'},'LBI.argkind',[('LDI','lbi_max',6),('C64U','lbi_argc','lbi_max')])
    P('LBI.argkind').branch({1:'LBI.dataargc'},'LBI.descbegin',[('CMPI','lbi_kind',1)])
    P('LBI.dataargc').branch({1:'LBI.descbegin'},'LBI.fail',[('CMPI','lbi_argc',0)])
    P('LBI.descbegin').a(('LDI','lbi_d',0),('LDI','lbi_field',0)).goto('LBI.desc')
    P('LBI.desc').call('LBI.u64').a(('ALUI','mul','lbi_index','lbi_i',42),('ALUI','mul','lbi_t','lbi_d',6),('ALU','add','lbi_index','lbi_index','lbi_t'),('ALU','add','lbi_index','lbi_index','lbi_field'),('STX','lbi_index',DESC,'lbi_value'),('ALUI','add','lbi_field','lbi_field',1)).branch({1:'LBI.checkdesc'},'LBI.desc',[('CMPI','lbi_field',6)])
    P('LBI.descnext').a(('LDI','lbi_field',0),('ALUI','add','lbi_d','lbi_d',1),('CMP','lbi_d','lbi_argc')).branch({2:'LBI.tailkind'},'LBI.desc',[('CMP','lbi_d','lbi_argc')])
    P('LBI.tailkind').branch({1:'LBI.extent'},'LBI.supported',[('CMPI','lbi_kind',1)])
    P('LBI.extent').call('LBI.u64').a(('STX','lbi_i',EXTENT,'lbi_value')).branch({0:'LBI.fail'},'LBI.extentadd',[('C64U','lbi_value','lbi_width')])
    P('LBI.extentadd').a(('LDX','lbi_address','lbi_i',ADDRESS),('A64','add','lbi_extentend','lbi_address','lbi_value')).branch({0:'LBI.fail',1:'LBI.fail'},'LBI.writable',[('C64U','lbi_extentend','lbi_address')])
    P('LBI.writable').call('LBI.byte').a(('STX','lbi_i',WRITABLE,'lbi_byte')).branch({1:'LBI.supported'},'LBI.fail',[('RLD','lbi_byte')])
    P('LBI.supported').call('LBI.byte').branch({1:'LBI.bound'},'LBI.fail',[('RLD','lbi_byte')])
    P('LBI.bound').a(('MARK','lbi_pos')).branch({1:'LBI.nextrecord'},'LBI.fail',[('CMP','lbi_pos','lbi_end')])
    P('LBI.nextrecord').a(('ALUI','add','lbi_i','lbi_i',1)).goto('LBI.record')
    P('LBI.inputend').a(('MARK','lbi_pos')).branch({1:'LBI.ready'},'LBI.fail',[('CMP','lbi_pos','lbi_len')])
    P('LBI.ready').a(('INPOP',),('LDI','lbi_validated',1)).goto('LBI.emit')
