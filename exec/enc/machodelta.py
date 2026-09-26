"""Mach-O layout/load commands and ad-hoc signature as ordinary transitions.
Uses common decoded/relocated DATA, stored/memlen/text_blob and entryoff.
Templates/constants follow unisa.image.macho; that writer is not called.
"""
from unisa.image import macho as M
from elfimage import DATA
from sha256delta import install as install_sha


def install(E,byte,arch):
    P,g=E.P,E.g
    install_sha(E)
    p=P('MACHO')
    def rnd(dst,src,align):p.a(('A64I','add',dst,src,align-1),('A64I','and',dst,dst,-align))
    p.a(('A64I','add','mh_end','endo',M.HDRS(arch)))
    rnd('mh_text','mh_end',M.PAGE)
    p.a(('COPYW','mh_vm','memlen')).branch({1:'MH.empty'},'MH.layout',[('CMPI','mh_vm',0)])
    P('MH.empty').a(('LDI','mh_vm',1)).goto('MH.layout')
    p=P('MH.layout');rnd('mh_vm','mh_vm',M.PAGE);rnd('mh_data','stored',M.PAGE)
    p.a(('A64','add','mh_link','mh_text','mh_data'),('A64I','add','mh_sigoff','mh_link',M.STRTAB))
    rnd('mh_sigoff','mh_sigoff',16)
    p.a(('A64I','add','mh_slots','mh_sigoff',M.PAGE4-1),('A64I','shr','mh_slots','mh_slots',12),
        ('A64I','mul','mh_siglen','mh_slots',32),('A64I','add','mh_siglen','mh_siglen',20+88+len(M.IDENT)),
        ('A64','sub','mh_linksz','mh_sigoff','mh_link'),('A64','add','mh_linksz','mh_linksz','mh_siglen'))
    rnd('mh_linkvm','mh_linksz',M.PAGE)
    p.a(('A64I','add','mh_datava','mh_text',M.VMADDR),('A64','add','mh_linkva','mh_datava','mh_vm'),
        ('A64','add','mh_bssva','mh_datava','stored'),('A64','sub','mh_bsslen','memlen','stored'),
        ('A64I','add','mh_entry','entryoff',M.HDRS(arch)))
    def field(w,v,big=False):
        p.a(('LDI' if isinstance(v,int) else 'COPYW','mb_v',v),('LDI','mb_n',w)).call('MB.big' if big else 'MB.little')
    def fields(items,big=False):
        for w,v in items:field(w,v,big)
    def name(v):
        for b in v.ljust(16,b'\0'):byte(p,b)
    def seg(n,va,vm,fo,fs,prot,ns):
        fields([(4,M.LC_SEGMENT_64),(4,M.SEG+M.SECT*ns)]);name(n)
        fields([(8,va),(8,vm),(8,fo),(8,fs),(4,prot),(4,prot),(4,ns),(4,0)])
    def sect(n,s,va,sz,off,flags):
        name(n);name(s);fields([(8,va),(8,sz),(4,off),(4,2),(4,0),(4,0),(4,flags),(4,0),(4,0),(4,0)])
    fields([(4,0xFEEDFACF),(4,M.CPU[arch][0]),(4,M.CPU[arch][1]),(4,2),(4,M.NCMDS),(4,M._cmdsz(arch)),(4,0x200085),(4,0)])
    seg(b'__PAGEZERO',0,M.VMADDR,0,0,0,0)
    seg(b'__TEXT',M.VMADDR,'mh_text',0,'mh_text',5,1)
    sect(b'__text',b'__TEXT',M.VMADDR+M.HDRS(arch),'endo',M.HDRS(arch),0x80000400)
    seg(b'__DATA','mh_datava','mh_vm','mh_text','mh_data',3,2)
    sect(b'__data',b'__DATA','mh_datava','stored','mh_text',0)
    sect(b'__bss',b'__DATA','mh_bssva','mh_bsslen',0,1)
    seg(b'__LINKEDIT','mh_linkva','mh_linkvm','mh_link','mh_linksz',1,0)
    fields([(4,M.LC_LOAD_DYLINKER),(4,32),(4,12)])
    for b in M.DYLD.ljust(20,b'\0'):byte(p,b)
    lib=(M.LIBSYS+b'\0');lib+=bytes((-len(lib))%8)
    fields([(4,M.LC_LOAD_DYLIB),(4,24+len(lib)),(4,24),(4,0),(4,0x10000),(4,0x10000)])
    for b in lib:byte(p,b)
    fields([(4,M.LC_MAIN),(4,24),(8,'mh_entry'),(8,0),(4,M.LC_BUILD_VERSION),(4,24),(4,1),(4,13<<16),(4,13<<16),(4,0)])
    fields([(4,M.LC_DYLD_INFO_ONLY),(4,48)]+[(4,0)]*10)
    fields([(4,M.LC_SYMTAB),(4,24),(4,'mh_link'),(4,0),(4,'mh_link'),(4,M.STRTAB)])
    fields([(4,M.LC_DYSYMTAB),(4,80)]+[(4,0)]*18)
    fields([(4,M.LC_CODE_SIGNATURE),(4,16),(4,'mh_sigoff'),(4,'mh_siglen')])
    for _ in range(M.SLACK):byte(p,0)
    p.a(('INPUSH','text_blob')).call('MH.copy').a(('COPYW','mh_pad','mh_text')).call('MH.pad').a(('LDI','di',0)).goto('MH.data')
    P('MH.data').branch({0:'MH.byte'},'MH.afterdata',[('CMP','di','stored')])
    P('MH.byte').a(('LDX','db','di',DATA),('OUTW','db'),('ALUI','add','di','di',1)).goto('MH.data')
    p=P('MH.afterdata').a(('COPYW','mh_pad','mh_sigoff')).call('MH.pad')
    # Snapshot the signed bytes. The signature itself is never hashed.
    p.a(('LDI','zero',0),('OCUT','mh_blob','zero'),('INPUSH','mh_blob')).call('MH.copy')
    fields([(4,M.CS_MAGIC_EMBEDDED),(4,'mh_siglen'),(4,1),(4,0),(4,20)],True)
    p.a(('A64I','sub','mh_cdlen','mh_siglen',20))
    fields([(4,M.CS_MAGIC_CODEDIRECTORY),(4,'mh_cdlen'),(4,0x20400),(4,M.CS_ADHOC),(4,88+len(M.IDENT)),(4,88),(4,0),(4,'mh_slots'),(4,'mh_sigoff'),(1,32),(1,2),(1,0),(1,12),(4,0),
            (4,0),(4,0),(4,0),(8,0),(8,0),(8,'mh_text'),(8,M.CS_EXECSEG_MAIN_BINARY)],True)
    for b in M.IDENT:byte(p,b)
    p.a(('LDI','mh_page',0)).goto('MH.hash')
    P('MH.hash').branch({0:'MH.slice'},'RET',[('C64','mh_page','mh_sigoff')])
    P('MH.slice').a(('A64I','add','mh_pageend','mh_page',M.PAGE4)).branch({2:'MH.last'},'MH.hashpage',[('C64','mh_pageend','mh_sigoff')])
    P('MH.last').a(('COPYW','mh_pageend','mh_sigoff')).goto('MH.hashpage')
    P('MH.hashpage').a(('INPUSH','mh_blob'),('BLOBSAVE','sh_blob','mh_page','mh_pageend'),('INPOP',)).call('SHA256').a(('COPYW','mh_page','mh_pageend')).goto('MH.hash')
    g.on('MH.copy',[256],'RET',[('INPOP',)]);g.els('MH.copy','MH.copy',[('COPY',),('ADV',)])
    P('MH.pad').a(('OLEN','mh_pos')).branch({0:'MH.zero'},'RET',[('C64','mh_pos','mh_pad')])
    byte(P('MH.zero'),0).goto('MH.pad')
    P('MB.little').branch({2:'MB.lebyte'},'RET',[('CMPI','mb_n',0)])
    P('MB.lebyte').a(('OUTW','mb_v'),('A64I','shr','mb_v','mb_v',8),('ALUI','sub','mb_n','mb_n',1)).goto('MB.little')
    P('MB.big').branch({2:'MB.bebyte'},'RET',[('CMPI','mb_n',0)])
    P('MB.bebyte').a(('ALUI','sub','mb_n','mb_n',1),('ALUI','mul','mb_shift','mb_n',8),('A64','shr','mb_byte','mb_v','mb_shift'),('OUTW','mb_byte')).goto('MB.big')
