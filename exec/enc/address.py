"""Deferred x86 RIP-relative encoding; layout runs after branch relaxation.

No reference layout or encoder is called at runtime. Format constants are read
at generation; page rounding, symbol resolution and displacements are delta
operations. This slice outputs text only; header data is not an image yet.
"""
from unisa.image import elf, macho
from unisa.tape import DATA_BASE
SYM, PRESENT, DEST, VALUE, NAMED, OPCODE, VALUE2 = (i * 10**6 for i in range(85, 92))


def install(E, byte, KND, SZ, OFF, LABD):
    P, g = E.P, E.g
    # Header: scan key and value separately; unknown headers rejected.
    g.on('HDR.key', [32], 'HDR.ws', [('MARK','he'),('ADV',)])
    g.on('HDR.key', [10,256], 'DEAD.addr', [])
    g.els('HDR.key','HDR.key',[('ADV',)])
    g.on('HDR.ws',[32],'HDR.ws',[('ADV',)])
    g.els('HDR.ws','HDR.val',[('MARK','hv')])
    g.on('HDR.val',[10,256],'HDR.end',[('MARK','hend')])
    g.els('HDR.val','HDR.val',[('ADV',)])
    p=P('HDR.end');p.a(('INTERN','hk','ws','he'))
    for key in ('target','sym','data','src_os','data_len','bss','relocs'):
        p.branch({1:'HDR.'+key},'HDR.next.'+key,[('CMP','hk','id_h_'+key)])
        p=P('HDR.next.'+key)
    p.goto('DEAD.addr')
    p=P('HDR.target');p.a(('INTERN','ht','hv','hend'))
    for i,t in enumerate(('lnx/x86_64','osx/x86_64'),1):
        p.branch({1:'HDR.target'+str(i)},'HDR.tnext'+str(i),[('CMP','ht','id_target'+str(i))])
        P('HDR.target'+str(i)).a(('LDI','target_os',i)).goto('SKIPL')
        p=P('HDR.tnext'+str(i))
    p.goto('DEAD.addr')
    for key in ('data','src_os','data_len','bss','relocs'):
        # Keep the exact declaration blob for the future image writer; no output.
        P('HDR.'+key).a(('BLOBSAVE','header_'+key,'hv','hend')).goto('SKIPL')
    P('HDR.sym').a(('JUMP','hv')).goto('HDR.sn')
    g.on('HDR.sn',[32],'HDR.sa',[('MARK','hne'),('ADV',),('LDI','ha',0)])
    g.on('HDR.sn',[10,256],'DEAD.addr',[])
    g.els('HDR.sn','HDR.sn',[('ADV',)])
    g.on('HDR.sa',range(48,58),'HDR.sa',[('BYTE','ht'),('ALUI','sub','ht','ht',48),('A64I','mul','ha','ha',10),('A64','add','ha','ha','ht'),('ADV',)])
    g.on('HDR.sa',[10,256],'HDR.save',[])
    g.els('HDR.sa','DEAD.addr',[])
    P('HDR.save').a(('INTERN','hs','hv','hne'),('LDX','ht','hs',PRESENT)).branch({1:'HDR.store'},'DEAD.addr',[('CMPI','ht',0)])
    P('HDR.store').a(('STX','hs',SYM,'ha'),('LDI','ht',1),('STX','hs',PRESENT,'ht')).goto('SKIPL')
    # Store an unresolved fixed-size RIP instruction. No early encoding.
    P('AD.store').a(('STX','npc',DEST,'ad_r'),('STX','npc',VALUE,'ad_v'),('STX','npc',NAMED,'anamed'),('STX','npc',OPCODE,'ad_o'),('LDI','t',4),('STX','npc',KND,'t'),('LDI','t',7),('STX','npc',SZ,'t'),('ALUI','add','npc','npc',1)).goto('SKIPL')
    P('AD.lea').a(('COPYW','ad_r','a0'),('COPYW','ad_v','a1'),('LDI','ad_o',0x8d)).goto('AD.store')
    P('AD.setmem').a(('COPYW','ad_r','a1'),('COPYW','ad_v','a0'),('LDI','ad_o',0x89),('LDI','anamed',0)).goto('AD.store')
    P('AD.mem').a(('COPYW','ad_r','a0'),('COPYW','ad_v','a1'),('LDI','ad_o',0x8b),('LDI','anamed',0)).goto('AD.store')
    P('AD.addr').a(('LDI','anamed',0)).goto('AD.lea')
    # Layout format declarations read here, algorithm remains explicit.
    p=P('LAYOUT');p.branch({1:'LAY.lnx',2:'LAY.osx'},'DEAD.addr',[('RLD','target_os')])
    for tag,m,base in [('lnx',elf,elf.VADDR),('osx',macho,macho.VMADDR)]:
        h=m.HDRS('x86_64'); page=m.PAGE
        P('LAY.'+tag).a(('LDI','text_va',base+h),('A64I','add','data_va','endo',h+page-1),('A64I','and','data_va','data_va',-page),('A64I','add','data_va','data_va',base),('A64I','sub','data_shift','data_va',DATA_BASE)).ret()
    # Resolve names after offsets are final; unknown symbols are errors.
    p=P('WR.addr');p.a(('LDX','ad_v','q',VALUE),('LDX','t','q',NAMED)).branch({1:'AD.name'},'AD.data',[('CMPI','t',1)])
    P('AD.name').a(('LDX','t','ad_v',PRESENT)).branch({1:'AD.sym'},'AD.label',[('CMPI','t',1)])
    P('AD.sym').a(('LDX','ad_v','ad_v',SYM)).goto('AD.data')
    P('AD.label').a(('LDX','t','ad_v',LABD)).branch({1:'DEAD.addr'},'AD.li',[('CMPI','t',0)])
    P('AD.li').a(('ALUI','sub','t','t',1)).branch({0:'AD.lin'},'AD.lend',[('CMP','t','npc')])
    P('AD.lin').a(('LDX','ad_v','t',OFF)).goto('AD.code')
    P('AD.lend').a(('COPYW','ad_v','endo')).goto('AD.code')
    P('AD.code').a(('A64','add','ad_v','ad_v','text_va')).goto('AD.emit')
    P('AD.data').a(('A64','add','ad_v','ad_v','data_shift')).goto('AD.emit')
    P('AD.emit').a(('LDX','ad_r','q',DEST),('LDX','ad_o','q',OPCODE)).call('RIP').goto('WR.nx')
    # RIP uses the actual output length, so multi-instruction forms remain exact.
    p=P('RIP');p.a(('OLEN','o_'),('A64','add','o_','o_','text_va'),('A64I','add','o_','o_',7),('A64','sub','lb_v','ad_v','o_'),('COPYW','rx_r','ad_r'),('LDI','rx_b',0),('LDI','rx_w',1)).call('REX')
    p.a(('OUTW','ad_o'),('COPYW','mr_r','ad_r'),('LDI','mr_m',0),('LDI','mr_b',5)).call('MODRM').a(('LDI','lb_n',4)).call('LEBYTES').ret()
    P('AD.argsave').a(('STX','npc',VALUE,'a0'),('STX','npc',VALUE2,'a1'),('STX','npc',NAMED,'a2'),('LDI','t',5),('STX','npc',KND,'t')).branch({1:'AD.as23'},'AD.as14',[('CMPI','a2',1)])
    P('AD.as23').a(('LDI','t',23)).goto('AD.finish')
    P('AD.as14').a(('LDI','t',14)).goto('AD.finish')
    P('AD.finish').a(('STX','npc',SZ,'t'),('ALUI','add','npc','npc',1)).goto('SKIPL')
    P('AD.argvget').a(('STX','npc',DEST,'a0'),('STX','npc',VALUE2,'a1'),('STX','npc',VALUE,'a2'),('LDI','t',6),('STX','npc',KND,'t'),('LDI','t',14)).goto('AD.finish')
    P('WR.argsave').a(('LDX','t','q',NAMED)).branch({1:'AS.stack'},'AS.reg',[('CMPI','t',1)])
    p=P('AS.stack')
    for b in (0x48,0x8b,0x04,0x24): byte(p,b)
    p.a(('LDI','ad_r',0),('LDI','ad_o',0x89),('LDX','ad_v','q',VALUE),('A64','add','ad_v','ad_v','data_shift')).call('RIP')
    for b in (0x48,0x8d,0x44,0x24,0x08): byte(p,b)
    p.a(('LDI','ad_r',0)).goto('AS.second')
    P('AS.reg').a(('LDI','ad_r',7),('LDI','ad_o',0x89),('LDX','ad_v','q',VALUE),('A64','add','ad_v','ad_v','data_shift')).call('RIP').a(('LDI','ad_r',6)).goto('AS.second')
    P('AS.second').a(('LDI','ad_o',0x89),('LDX','ad_v','q',VALUE2),('A64','add','ad_v','ad_v','data_shift')).call('RIP').goto('WR.nx')
    p=P('WR.argvget');p.a(('LDI','ad_r',11),('LDI','ad_o',0x8b),('LDX','ad_v','q',VALUE),('A64','add','ad_v','ad_v','data_shift')).call('RIP')
    p.a(('LDX','ix','q',VALUE2),('ALUI','sar','t','ix',3),('ALUI','shl','t','t',1),('ALUI','or','t','t',0x4d),('OUTW','t'))
    for b in (0x8b,0x1c): byte(p,b)
    p.a(('ALUI','and','t','ix',7),('ALUI','shl','t','t',3),('ALUI','or','t','t',0xc3),('OUTW','t'),('LDI','al_o',0x89),('LDX','al_d','q',DEST),('LDI','al_s',11)).call('ALU').goto('WR.nx')
    g.on('DEAD.addr',range(257),'DEAD',E.rej('not covered: address or target declaration'),'r')
