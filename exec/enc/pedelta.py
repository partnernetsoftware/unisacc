"""PE32+ fields and layout compiled into ordinary executor transitions.
No reference writer is called. Imports are declaration strings; addresses,
relocation sorting/grouping and section extents are computed at execution.
"""
from unisa.image import pe
from elfimage import DATA
RELOCS=4<<40
SORTED=5<<40


def install(E,byte,arch):
    P,g=E.P,E.g
    n=len(pe.IMPORTS);iat=40+8*(n+1);off=iat+8*(n+1)
    names=[]
    for name in pe.IMPORTS:
        raw=b'\0\0'+name.encode()+b'\0';raw+=b'\0'*(len(raw)%2)
        names.append((off,raw));off+=len(raw)
    dlloff=off;off+=len(pe.DLL)+1;cfg=(off+7)&-8;idlen=cfg+pe.LOADCFG
    def field(p,w,v):
        return p.a(('LDI' if isinstance(v,int) else 'COPYW','lb_v',v),('LDI','lb_n',w)).call('EI.bytes')
    def fields(p,items):
        for w,v in items:field(p,w,v)
        return p
    def literal(p,bs):
        for b in bs:byte(p,b)
        return p
    from pathlib import Path
    from finite_rules import install as install_rules, load as load_rules
    pe_bindings = dict(DATA=DATA, RELOCS=RELOCS, SORTED=SORTED,
                       TEXT_RVA=pe.TEXT_RVA, IMAGEBASE=pe.IMAGEBASE, HDR_FILE=pe.HDR_FILE,
                       section_minus_one=pe.SECT_ALIGN-1, section_mask=-pe.SECT_ALIGN,
                       file_minus_one=pe.FILE_ALIGN-1, file_mask=-pe.FILE_ALIGN,
                       import_vsize=(idlen+pe.SECT_ALIGN-1)&-pe.SECT_ALIGN,
                       import_fsize=(idlen+pe.FILE_ALIGN-1)&-pe.FILE_ALIGN,
                       cookie_at=cfg+pe.COOKIE_FIELD, cfg=cfg, iat=iat, dlloff=dlloff)
    def actions(section, **values):
        return load_rules(Path(__file__).with_name('pedelta-result.tsv'), {},
                          bindings=dict(pe_bindings, **values), section=section)['actions'][0][1]
    pe_sequences = {'zero_byte': byte(P('byte.binding'),0).acts}
    pe_bindings.update({'label'+str(i): P('PE').fresh(kind)
                        for i, kind in enumerate('bbrbrbbbbbbbrrbrbrbb')})
    install_rules(g, Path(__file__).parent, 'pedelta', bindings=pe_bindings,
                  sequences=pe_sequences, section='layout')
    p=P('PE.header');literal(p,b'MZ'+bytes(58));field(p,4,64);literal(p,b'PE\0\0')
    fields(p,[(2,pe.MACHINE[arch]),(2,4),(4,0),(4,0),(4,0),(2,240),(2,0x22)])
    p.a(actions('entry-value'))
    fields(p,[(2,0x20B),(1,14),(1,0),(4,'pe_tf'),(4,0),(4,0),(4,'pe_entry'),(4,pe.TEXT_RVA),(8,pe.IMAGEBASE),(4,pe.SECT_ALIGN),(4,pe.FILE_ALIGN)])
    fields(p,[(2,4),(2,0),(2,0),(2,0),(2,4),(2,0),(4,0),(4,'pe_img'),(4,pe.HDR_FILE),(4,0),(2,3),(2,0x8160),(8,0x100000),(8,0x1000),(8,0x100000),(8,0x1000),(4,0),(4,16)])
    p.a(actions('directory-values'))
    dirs={1:('pe_rd',40),5:('pe_rr','pe_rlsize'),10:('pe_cfg',pe.LOADCFG),12:('pe_iat',(n+1)*8)}
    for i in range(16):fields(p,[(4,dirs.get(i,(0,0))[0]),(4,dirs.get(i,(0,0))[1])])
    for name,rva,vs,fo,fs,flags in [(b'.text',pe.TEXT_RVA,'endo',pe.HDR_FILE,'pe_tf',0x60000020),(b'.rdata','pe_rd',idlen,'pe_rf',(idlen+511)&-512,0x40000040),(b'.data','pe_dt','pe_dvs','pe_df','pe_datafile',0xC0000040),(b'.reloc','pe_rr','pe_rlsize','pe_relf',None,0x42000040)]:
        if fs is None:p.a(actions('reloc-align'));fs='pe_rsize'
        literal(p,name.ljust(8,b'\0'));fields(p,[(4,vs),(4,rva),(4,fs),(4,fo),(4,0),(4,0),(2,0),(2,0),(4,flags)])
    pe_bindings.update(state=p.cur, label0=P('PE').fresh('b'))
    install_rules(g, Path(__file__).parent, 'pedelta', bindings=pe_bindings,
                  sequences={'pending': p.acts}, section='header-end')
    copy_labels = {'copy'+str(i): P('PE').fresh('r') for i in range(3)}
    install_rules(g, Path(__file__).parent, 'pedelta', section='text-copy',
                  bindings=dict(pe_bindings, **copy_labels))
    p=P(copy_labels['copy2']).a(actions('import-values'))
    fields(p,[(4,'pe_int'),(4,0),(4,0),(4,'pe_dll'),(4,'pe_iat')]);literal(p,bytes(20))
    for _ in range(2):
        for offset,raw in names:
            p.a(actions('name-value', offset=offset));field(p,8,'pe_name')
        field(p,8,0)
    for offset,raw in names:literal(p,raw)
    literal(p,pe.DLL+b'\0'+bytes(cfg-(dlloff+len(pe.DLL)+1)))
    field(p,4,pe.LOADCFG);literal(p,bytes(pe.COOKIE_FIELD-4));field(p,8,'pe_cookie');literal(p,bytes(pe.LOADCFG-pe.COOKIE_FIELD-8))
    install_rules(g, Path(__file__).parent, 'pedelta', section='imports-end',
                  bindings={'state': p.cur, 'resume': p.fresh('r')},
                  sequences={'pending': p.acts})
    pe_bindings.update({'label'+str(i): P('PE').fresh(kind)
                        for i, kind in enumerate('brrrb')})
    install_rules(g, Path(__file__).parent, 'pedelta', bindings=pe_bindings,
                  sequences=pe_sequences, section='output')
