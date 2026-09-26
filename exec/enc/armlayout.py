"""ARM text/data address resolution. Header values are declarations, not answers.
Image format constants are read at generation; layout/ADRP arithmetic runs as
integer transitions. This outputs text; retained data headers are not an image.
"""
from unisa.image import elf,macho
from unisa.tape import DATA_BASE
from armbranch import LABELS
SYM,PRESENT,HSEEN=77000000,78000000,79000000


def init(p):
    p.a(('LDI','target_os',1),('SBCLR',),[('SBOUT',c) for c in b'_start'],('SBINTERN','id_entry'))
    for key in ('target','data','src_os','data_len','bss','relocs','sym'):
        p.a(('SBCLR',),[('SBOUT',c) for c in ('@'+key).encode()],('SBINTERN','h_'+key))
    for key,value in (('lnx','lnx/arm64'),('osx','osx/arm64')):
        p.a(('SBCLR',),[('SBOUT',c) for c in value.encode()],('SBINTERN','target_'+key))


def install(E,word):
    P,g=E.P,E.g
    g.on('HDR.key',[32],'HDR.ws',[('MARK','he'),('ADV',)])
    g.on('HDR.key',[10,256],'FAIL',[]);g.els('HDR.key','HDR.key',[('ADV',)])
    g.on('HDR.ws',[32],'HDR.ws',[('ADV',)])
    g.on('HDR.ws',[10,256],'FAIL',[]);g.els('HDR.ws','HDR.value',[('MARK','hv')])
    g.on('HDR.value',[10,256],'HDR.end',[('MARK','hend')]);g.els('HDR.value','HDR.value',[('ADV',)])
    P('HDR.end').branch({1:'LINE'},'HDR.first',[('CMPI','pass',1)])
    P('HDR.first').branch({1:'HDR.dispatch'},'FAIL',[('CMPI','started',0)])
    p=P('HDR.dispatch').a(('INTERN','hk','hs','he'))
    p.branch({1:'HDR.sym'},'HDR.seen',[('CMP','hk','h_sym')])
    P('HDR.seen').a(('LDX','t','hk',HSEEN)).branch({1:'HDR.unique'},'FAIL',[('CMPI','t',0)])
    p=P('HDR.unique').a(('LDI','t',1),('STX','hk',HSEEN,'t'))
    for key in ('target','data','src_os','data_len','bss','relocs'):
        p.branch({1:'HDR.'+key},'HDR.next.'+key,[('CMP','hk','h_'+key)]);p=P('HDR.next.'+key)
    p.goto('FAIL')
    for key in ('data','src_os','data_len','bss','relocs'):
        p=P('HDR.'+key).a(('BLOBSAVE','header_'+key,'hv','hend'))
        if key in ('data','data_len','bss','relocs'):p.a(('LDI','has_'+key,1))
        p.goto('LINE')
    P('HDR.target').a(('INTERN','t','hv','hend')).branch({1:'HDR.lnx'},'HDR.osxcheck',[('CMP','t','target_lnx')])
    P('HDR.osxcheck').branch({1:'HDR.osx'},'FAIL',[('CMP','t','target_osx')])
    for os,n in (('lnx',1),('osx',2)):P('HDR.'+os).a(('LDI','target_os',n)).goto('LINE')
    P('HDR.sym').a(('JUMP','hv')).goto('HDR.name')
    g.on('HDR.name',[32],'HDR.numfirst',[('MARK','hne'),('ADV',),('LDI','ha',0)])
    g.on('HDR.name',[10,256],'FAIL',[]);g.els('HDR.name','HDR.name',[('ADV',)])
    g.on('HDR.numfirst',range(48,58),'HDR.num',[]);g.els('HDR.numfirst','FAIL',[])
    g.on('HDR.num',range(48,58),'HDR.bound',[('BYTE','t'),('ALUI','sub','t','t',48),('A64I','mul','ha','ha',10),('A64','add','ha','ha','t')])
    P('HDR.bound').a(('LDI','limit',2147483647)).branch({2:'FAIL'},'HDR.numadv',[('C64U','ha','limit')])
    P('HDR.numadv').a(('ADV',)).goto('HDR.num')
    g.on('HDR.num',[10,256],'HDR.storecheck',[]);g.els('HDR.num','FAIL',[])
    P('HDR.storecheck').a(('INTERN','sid','hv','hne'),('LDX','t','sid',PRESENT)).branch({1:'HDR.store'},'FAIL',[('CMPI','t',0)])
    P('HDR.store').a(('STX','sid',SYM,'ha'),('LDI','t',1),('STX','sid',PRESENT,'t')).goto('LINE')
    P('LAYOUT').a(('OLEN','length')).branch({1:'LAY.lnx',2:'LAY.osx'},'FAIL',[('RLD','target_os')])
    for os,m,base in (('lnx',elf,elf.VADDR),('osx',macho,macho.VMADDR)):
        h=m.HDRS('arm64');pg=m.PAGE
        P('LAY.'+os).a(('LDI','text_va',base+h),('A64I','add','data_va','length',h+pg-1),('A64I','and','data_va','data_va',-pg),('A64I','add','data_va','data_va',base),('A64I','sub','data_shift','data_va',DATA_BASE)).ret()
    P('AD.addr').a(('COPYW','ad_r','a0'),('COPYW','ad_v','a1')).call('AD.data').call('ADRP').goto('LINE')
    p=P('AD.mem').a(('LDI','ad_r',17),('COPYW','ad_v','a1')).call('AD.data').call('ADRP')
    word(p.a(('ALUI','or','w','a0',0xF9400000|(17<<5)))).goto('LINE')
    p=P('EMIT.29').a(('LDI','ad_r',17),('COPYW','ad_v','a0')).branch({1:'FAIL'},'SETMEM.emit',[('CMPI','a1',17)])
    p=P('SETMEM.emit').call('AD.data').call('ADRP')
    word(p.a(('ALUI','or','w','a1',0xF9000000|(17<<5)))).goto('LINE')
    P('EMIT.28').a(('COPYW','ad_r','a0'),('COPYW','ad_v','a1')).branch({1:'LEA.resolve'},'LEA.placeholder',[('CMPI','pass',1)])
    P('LEA.placeholder').a(('LDI','ad_v',0)).call('ADRP').goto('LINE')
    P('LEA.resolve').a(('LDX','t','ad_v',PRESENT)).branch({1:'LEA.data'},'LEA.code',[('CMPI','t',1)])
    P('LEA.data').a(('LDX','ad_v','ad_v',SYM)).call('AD.data').call('ADRP').goto('LINE')
    P('LEA.code').a(('LDX','ad_v','ad_v',LABELS)).branch({1:'FAIL'},'LEA.codeok',[('CMPI','ad_v',0)])
    P('LEA.codeok').a(('A64I','sub','ad_v','ad_v',1),('A64','add','ad_v','ad_v','text_va')).call('ADRP').goto('LINE')
    P('AD.data').a(('LDI','limit',2147483647)).branch({2:'FAIL'},'AD.shift',[('C64U','ad_v','limit')])
    P('AD.shift').a(('A64','add','ad_v','ad_v','data_shift')).ret()
    # Fixed two words, including sizing pass. Signed 21-bit page displacement.
    P('ADRP').branch({1:'ADR.calc'},'ADR.zero',[('CMPI','pass',1)])
    P('ADR.zero').a(('LDI','page',0),('LDI','lo',0)).goto('ADR.emit')
    P('ADR.calc').a(('OLEN','pc'),('A64','add','pc','pc','text_va'),('A64I','shr','pc','pc',12),('A64I','shr','page','ad_v',12),('A64','sub','page','page','pc'),('ALUI','and','lo','ad_v',4095),('LDI','limit',-1048576)).branch({0:'FAIL'},'ADR.high',[('C64','page','limit')])
    P('ADR.high').a(('LDI','limit',1048576)).branch({0:'ADR.emit'},'FAIL',[('C64','page','limit')])
    p=P('ADR.emit').a(('ALUI','and','w','page',3),('ALUI','shl','w','w',29),('A64I','sar','t','page',2),('ALUI','and','t','t',524287),('ALUI','shl','t','t',5),('ALU','or','w','w','t'),('ALUI','or','w','w',0x90000000),('ALU','or','w','w','ad_r'))
    word(p).a(('ALUI','shl','w','lo',10),('ALUI','shl','t','ad_r',5),('ALU','or','w','w','t'),('ALU','or','w','w','ad_r'),('ALUI','or','w','w',0x91000000));word(p).ret()
    P('EMIT.30').branch({1:'AS.stack'},'AS.first',[('CMPI','a2',1)])
    p=P('AS.stack')
    for v in (0xF94003E0,0x910023E1):word(p.a(('LDI','w',v)))
    p.goto('AS.first')
    for label,arg,r,nxt in (('AS.first','a0',0,'AS.second'),('AS.second','a1',1,'LINE')):
        p=P(label).a(('LDI','ad_r',17),('COPYW','ad_v',arg)).call('AD.data').call('ADRP')
        word(p.a(('LDI','w',0xF9000000|(17<<5)|r))).goto(nxt)
    P('EMIT.31').branch({1:'FAIL'},'ARGV.emit',[('CMPI','a1',17)])
    p=P('ARGV.emit').a(('LDI','ad_r',17),('COPYW','ad_v','a2')).call('AD.data').call('ADRP')
    word(p.a(('LDI','w',0xF9400000|(17<<5)|17)))
    word(p.a(('ALUI','shl','w','a1',16),('ALUI','or','w','w',0x8B000000|(3<<10)|(17<<5)|17)))
    word(p.a(('ALUI','or','w','a0',0xF9400000|(17<<5)))).goto('LINE')
