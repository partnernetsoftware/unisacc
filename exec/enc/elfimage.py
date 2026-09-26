"""ELF image-writing delta, after code generation. No image-specific executor op.

The binary format's fields are a template here; runtime layout, data decoding,
relocation, zero-tail trimming and output are ordinary delta actions. Only
Linux x86_64 is selected by this first image route.
"""
from unisa.image import elf
DATA = 93 * 10**6


def install(E, byte, OFF, LABD):
    P,g=E.P,E.g
    P('ELF').branch({1:'ELF.begin'},'DEAD.image',[('CMPI','target_os',1)])
    P('ELF.begin').a(('LDI','zero',0),('OCUT','text_blob','zero'),('LDI','dlen',0),('INPUSH','header_data')).goto('ED.first')
    g.on('ED.first',[45],'ED.empty',[('ADV',)])
    g.els('ED.first','ED.hi',[])
    g.on('ED.empty',[256],'ED.end',[])
    g.els('ED.empty','DEAD.image',[])
    g.on('ED.hi',[256],'ED.end',[])
    for c,v in [(c,c-48) for c in range(48,58)]+[(c,c-87) for c in range(97,103)]:
        g.on('ED.hi',[c],'ED.lo',[('LDI','db',v*16),('ADV',)])
        g.on('ED.lo',[c],'ED.put',[('ALUI','add','db','db',v),('ADV',)])
    g.els('ED.hi','DEAD.image',[]);g.els('ED.lo','DEAD.image',[])
    P('ED.put').a(('STX','dlen',DATA,'db'),('ALUI','add','dlen','dlen',1)).goto('ED.hi')
    # @data may omit its zero tail. data_len is its logical extent. The Linux route
    # requires bss=0 (the extra stack field belongs to Windows).
    P('ED.end').a(('INPOP',),('LDI','extent_max',2147483647),('COPYW','vlen','dlen')).branch({1:'EM.len'},'EM.bss',[('CMPI','has_data_len',1)])
    P('EM.len').a(('INPUSH','header_data_len')).call('EM.num').branch({0:'DEAD.image'},'EM.length',[('C64','mn','dlen')])
    P('EM.length').a(('COPYW','vlen','mn')).goto('EM.bss')
    P('EM.bss').a(('COPYW','memlen','vlen')).branch({1:'EM.extra'},'EM.ready',[('CMPI','has_bss',1)])
    P('EM.extra').a(('INPUSH','header_bss')).call('EM.num').branch({1:'EM.ready'},'DEAD.image',[('CMPI','mn',0)]) # Linux route has no additional Windows stack BSS
    P('EM.num').a(('LDI','mn',0),('LDI','nd',0)).goto('EM.digit')
    g.on('EM.digit',range(48,58),'EM.bound',[('BYTE','bt'),('ALUI','sub','bt','bt',48),('A64I','mul','mn','mn',10),('A64','add','mn','mn','bt'),('ALUI','add','nd','nd',1),('ADV',)])
    P('EM.bound').branch({2:'DEAD.image'},'EM.digit',[('C64U','mn','extent_max')])
    g.on('EM.digit',[256],'EM.end',[]);g.els('EM.digit','DEAD.image',[])
    P('EM.end').a(('INPOP',)).branch({1:'DEAD.image'},'RET',[('CMPI','nd',0)])
    P('EM.ready').branch({1:'ER.init'},'ER.trim',[('CMPI','has_relocs',1)])
    P('ER.init').a(('INPUSH','header_relocs')).goto('ER.first')
    g.on('ER.first',[45],'ER.empty',[('ADV',)])
    g.els('ER.first','ER.start',[])
    g.on('ER.empty',[256],'ER.end',[]);g.els('ER.empty','DEAD.image',[])
    g.els('ER.start','ER.digit',[('LDI','ra',0),('LDI','nd',0)])
    g.on('ER.digit',range(48,58),'ER.digit',[('BYTE','bt'),('ALUI','sub','bt','bt',48),('A64I','mul','ra','ra',10),('A64','add','ra','ra','bt'),('ALUI','add','nd','nd',1),('ADV',)])
    g.on('ER.digit',[44,256],'ER.bound',[]);g.els('ER.digit','DEAD.image',[])
    P('ER.bound').branch({1:'DEAD.image'},'ER.b2',[('CMPI','nd',0)])
    P('ER.b2').a(('A64I','add','rend','ra',8)).branch({2:'DEAD.image'},'ER.read',[('C64U','rend','vlen')])
    P('ER.read').branch({2:'ER.extend'},'ER.value',[('C64','rend','dlen')])
    P('ER.extend').a(('COPYW','dlen','rend')).goto('ER.value')
    p=P('ER.value');p.a(('LDI','rv',0))
    for j in range(8):
        p.a(('ALUI','add','di','ra',j),('LDX','db','di',DATA),('A64I','shl','db','db',8*j),('A64','or','rv','rv','db'))
    p.a(('A64','add','rv','rv','data_shift'))
    for j in range(8):
        p.a(('ALUI','add','di','ra',j),('A64I','and','db','rv',255),('STX','di',DATA,'db'),('A64I','shr','rv','rv',8))
    p.goto('ER.sep')
    g.on('ER.sep',[44],'ER.start',[('ADV',)]);g.on('ER.sep',[256],'ER.end',[])
    P('ER.end').a(('INPOP',)).goto('ER.trim')
    P('ER.trim').a(('COPYW','stored','dlen')).label('ET.loop').branch({1:'EH'},'ET.last',[('CMPI','stored',0)])
    P('ET.last').a(('ALUI','sub','di','stored',1),('LDX','db','di',DATA)).branch({1:'ET.drop'},'EH',[('CMPI','db',0)])
    P('ET.drop').a(('ALUI','sub','stored','stored',1)).goto('ET.loop')
    # Entry label is resolved from the same final layout as branch targets.
    P('EH').a(('LDX','ei','id_entry',LABD),('LDI','entryoff',0)).branch({1:'EH.write'},'EH.entry',[('CMPI','ei',0)])
    P('EH.entry').a(('ALUI','sub','ei','ei',1)).branch({0:'EH.in'},'EH.end',[('CMP','ei','npc')])
    P('EH.in').a(('LDX','entryoff','ei',OFF)).goto('EH.write')
    P('EH.end').a(('COPYW','entryoff','endo')).goto('EH.write')
    p=P('EH.write');p.a(('A64','add','entryva','text_va','entryoff'),('A64I','add','tend','endo',elf.HDRS('x86_64')),('A64I','sub','doff','data_va',elf.VADDR))
    for b in b'\x7fELF'+bytes([2,1,1,0])+bytes(8):byte(p,b)
    def field(width,v):
        p.a(('LDI' if isinstance(v,int) else 'COPYW','lb_v',v),('LDI','lb_n',width)).call('LEBYTES')
    for w,v in [(2,2),(2,elf.MACHINE['x86_64']),(4,1),(8,'entryva'),(8,elf.EHDR),(8,0),(4,0),(2,elf.EHDR),(2,elf.PHDR),(2,elf.NPH),(2,0),(2,0),(2,0)]:field(w,v)
    for flags,offset,va,fs,ms in [(5,0,elf.VADDR,'tend','tend'),(6,'doff','data_va','stored','memlen')]:
        for w,v in [(4,1),(4,flags),(8,offset),(8,va),(8,va),(8,fs),(8,ms),(8,elf.PAGE)]:field(w,v)
    p.a(('INPUSH','text_blob')).goto('EI.copy')
    g.on('EI.copy',[256],'EI.pad',[('INPOP',)])
    g.els('EI.copy','EI.copy',[('COPY',),('ADV',)])
    P('EI.pad').a(('OLEN','pos')).branch({0:'EI.zero'},'EI.data',[('C64','pos','doff')])
    byte(P('EI.zero'),0).goto('EI.pad')
    P('EI.data').a(('LDI','di',0)).label('EI.loop').branch({0:'EI.byte'},'RET',[('CMP','di','stored')])
    P('EI.byte').a(('LDX','db','di',DATA),('OUTW','db'),('ALUI','add','di','di',1)).goto('EI.loop')
    g.on('DEAD.image',range(257),'DEAD',E.rej('not covered: ELF input or relocation'),'r')
