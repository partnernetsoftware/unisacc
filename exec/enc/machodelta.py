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
    from pathlib import Path
    from finite_rules import install as install_rules, load as load_rules
    mh_bindings = dict(DATA=DATA, HDRS=M.HDRS(arch), VMADDR=M.VMADDR, STRTAB=M.STRTAB,
                       page_minus_one=M.PAGE-1, page_mask=-M.PAGE,
                       page4_minus_one=M.PAGE4-1, PAGE4=M.PAGE4,
                       signature_base=20+88+len(M.IDENT), DLPREFIX=M.DLPREFIX, DLBINDLEN=len(M.DLBIND))
    mh_sequences = {'zero_byte': byte(P('byte.binding'),0).acts}
    mh_bindings.update({'label'+str(i): P(owner).fresh(kind)
                        for i, (owner, kind) in enumerate((('MACHO', 'b'), ('MH', 'b')))})
    install_rules(g, Path(__file__).parent, 'machodelta', bindings=mh_bindings,
                  sequences=mh_sequences, section='layout')
    p=P('MH.header')
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
    sect(b'__data',b'__DATA','mh_datava','mh_stored','mh_text',0)
    sect(b'__bss',b'__DATA','mh_bssva','mh_bsslen',0,1)
    seg(b'__LINKEDIT','mh_linkva','mh_linkvm','mh_link','mh_linksz',1,0)
    fields([(4,M.LC_LOAD_DYLINKER),(4,32),(4,12)])
    for b in M.DYLD.ljust(20,b'\0'):byte(p,b)
    lib=(M.LIBSYS+b'\0');lib+=bytes((-len(lib))%8)
    fields([(4,M.LC_LOAD_DYLIB),(4,24+len(lib)),(4,24),(4,0),(4,0x10000),(4,0x10000)])
    for b in lib:byte(p,b)
    fields([(4,M.LC_MAIN),(4,24),(8,'mh_entry'),(8,0),(4,M.LC_BUILD_VERSION),(4,24),(4,1),(4,13<<16),(4,13<<16),(4,0)])
    fields([(4,M.LC_DYLD_INFO_ONLY),(4,48),(4,0),(4,0),(4,'mh_bindoff'),(4,len(M.DLBIND))]+[(4,0)]*6)
    fields([(4,M.LC_SYMTAB),(4,24),(4,'mh_link'),(4,0),(4,'mh_link'),(4,M.STRTAB)])
    fields([(4,M.LC_DYSYMTAB),(4,80)]+[(4,0)]*18)
    fields([(4,M.LC_CODE_SIGNATURE),(4,16),(4,'mh_sigoff'),(4,'mh_siglen')])
    for _ in range(M.SLACK):byte(p,0)
    mh_bindings['state'] = p.cur
    mh_bindings.update({'label'+str(i): P(owner).fresh(kind)
                        for i, (owner, kind) in enumerate((('MH', 'b'), ('MH', 'r'), ('MH', 'b'), ('MH', 'r'), ('MH', 'b'), ('MH', 'r'), ('MH', 'r')))})
    install_rules(g, Path(__file__).parent, 'machodelta', bindings=mh_bindings,
                  sequences={'pending': p.acts}, section='copy')
    p=P('MH.dataprefix')
    for _ in range(M.DLPREFIX):byte(p,0)
    install_rules(g, Path(__file__).parent, 'machodelta', section='prefix-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    p=P('MH.binddata')
    for value in M.DLBIND:byte(p,value)
    p.a(('COPYW','mh_pad','mh_sigoff')).call('MH.pad',ret=mh_bindings['label5'])
    p = P(mh_bindings['label6'])
    fields([(4,M.CS_MAGIC_EMBEDDED),(4,'mh_siglen'),(4,1),(4,0),(4,20)],True)
    p.a(load_rules(Path(__file__).with_name('machodelta-result.tsv'), {},
                   section='signature-length')['actions'][0][1])
    fields([(4,M.CS_MAGIC_CODEDIRECTORY),(4,'mh_cdlen'),(4,0x20400),(4,M.CS_ADHOC),(4,88+len(M.IDENT)),(4,88),(4,0),(4,'mh_slots'),(4,'mh_sigoff'),(1,32),(1,2),(1,0),(1,12),(4,0),
            (4,0),(4,0),(4,0),(8,0),(8,0),(8,'mh_text'),(8,M.CS_EXECSEG_MAIN_BINARY)],True)
    for b in M.IDENT:byte(p,b)
    install_rules(g, Path(__file__).parent, 'machodelta', section='signature-end',
                  bindings={'state': p.cur}, sequences={'pending': p.acts})
    mh_bindings.update({'label'+str(i): P(owner).fresh(kind)
                        for i, (owner, kind) in enumerate((('MH', 'b'), ('MH', 'b'), ('MH', 'r'), ('MH', 'b')))})
    install_rules(g, Path(__file__).parent, 'machodelta', bindings=mh_bindings,
                  sequences=mh_sequences, section='output')
    for direction, step in (('little', 'MB.lebyte'), ('big', 'MB.bebyte')):
        install_rules(g, Path(__file__).parent, 'machodelta', section='endian',
                      bindings={'entry': 'MB.'+direction, 'check': P('MB').fresh('b'), 'step': step})
