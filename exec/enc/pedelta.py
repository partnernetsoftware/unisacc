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
    def align(p,d,s,a):return p.a(('A64I','add',d,s,a-1),('A64I','and',d,d,-a))
    def pad(p,v):return p.a(('LDI' if isinstance(v,int) else 'COPYW','pe_pad',v)).call('PE.pad')
    p=P('PE').a(('COPYW','pe_full','vlen'),('COPYW','pe_stored','stored'))
    p.branch({1:'PE.fullzero'},'PE.full',[('CMPI','pe_full',0)])
    P('PE.fullzero').a(('LDI','pe_full',1)).goto('PE.full')
    p=P('PE.full');align(p,'pe_ck','pe_full',8)
    p.a(('A64I','add','pe_dvs','pe_ck',8),('A64','add','pe_dvs','pe_dvs','extra_bss'))
    align(p,'pe_rd','endo',pe.SECT_ALIGN).a(('A64I','add','pe_rd','pe_rd',pe.TEXT_RVA),('A64I','add','pe_dt','pe_rd',(idlen+pe.SECT_ALIGN-1)&-pe.SECT_ALIGN),('A64','add','pe_cookie','pe_dt','pe_ck'),('A64I','add','pe_cookie','pe_cookie',pe.IMAGEBASE))
    align(p,'pe_tf','endo',pe.FILE_ALIGN).a(('A64I','add','pe_rf','pe_tf',pe.HDR_FILE),('A64I','add','pe_df','pe_rf',(idlen+pe.FILE_ALIGN-1)&-pe.FILE_ALIGN))
    align(p,'pe_rr','pe_dvs',pe.SECT_ALIGN).a(('A64','add','pe_rr','pe_rr','pe_dt'))
    p.branch({1:'PE.storezero'},'PE.store',[('CMPI','pe_stored',0)])
    P('PE.storezero').a(('LDI','pe_stored',1)).goto('PE.store')
    p=P('PE.store');align(p,'pe_ds','pe_stored',8);align(p,'pe_datafile','pe_ds',pe.FILE_ALIGN)
    p.a(('A64','add','pe_relf','pe_df','pe_datafile'),('LDI','pe_sn',0),('LDI','pe_i',0),('A64I','add','pe_val','pe_rd',cfg+pe.COOKIE_FIELD)).call('PE.insert').goto('PE.relinput')
    P('PE.relinput').branch({0:'PE.relnext'},'PE.relwrite',[('CMP','pe_i','nr_relocs')])
    P('PE.relnext').a(('LDX','pe_val','pe_i',RELOCS),('A64','add','pe_val','pe_val','pe_dt')).call('PE.insert').a(('ALUI','add','pe_i','pe_i',1)).goto('PE.relinput')
    # Sorted unique relocation RVAs. Input count is small for ordinary tables;
    # insertion does not scan logical BSS or any address-sized zero range.
    P('PE.insert').a(('LDI','pe_probe',0)).goto('PE.duplicate')
    P('PE.duplicate').branch({0:'PE.dupcheck'},'PE.insbegin',[('CMP','pe_probe','pe_sn')])
    P('PE.dupcheck').a(('LDX','pe_old','pe_probe',SORTED)).branch({1:'RET'},'PE.dupmore',[('C64','pe_old','pe_val')])
    P('PE.dupmore').a(('ALUI','add','pe_probe','pe_probe',1)).goto('PE.duplicate')
    P('PE.insbegin').a(('COPYW','pe_j','pe_sn')).goto('PE.insloop')
    P('PE.insloop').branch({1:'PE.insput'},'PE.insprev',[('CMPI','pe_j',0)])
    P('PE.insprev').a(('ALUI','sub','pe_prev','pe_j',1),('LDX','pe_old','pe_prev',SORTED)).branch({1:'RET',0:'PE.insput'},'PE.insshift',[('C64','pe_old','pe_val')])
    P('PE.insshift').a(('STX','pe_j',SORTED,'pe_old'),('COPYW','pe_j','pe_prev')).goto('PE.insloop')
    P('PE.insput').a(('STX','pe_j',SORTED,'pe_val'),('ALUI','add','pe_sn','pe_sn',1)).ret()
    P('PE.relwrite').a(('LDI','pe_i',0)).goto('PE.group')
    P('PE.group').branch({0:'PE.groupstart'},'PE.relend',[('CMP','pe_i','pe_sn')])
    P('PE.groupstart').a(('LDX','pe_val','pe_i',SORTED),('A64I','and','pe_page','pe_val',-4096),('COPYW','pe_j','pe_i')).goto('PE.scan')
    P('PE.scan').branch({0:'PE.scanpage'},'PE.block',[('CMP','pe_j','pe_sn')])
    P('PE.scanpage').a(('LDX','pe_val','pe_j',SORTED),('A64I','and','pe_test','pe_val',-4096)).branch({1:'PE.scanmore'},'PE.block',[('C64','pe_test','pe_page')])
    P('PE.scanmore').a(('ALUI','add','pe_j','pe_j',1)).goto('PE.scan')
    p=P('PE.block').a(('ALU','sub','pe_count','pe_j','pe_i'),('ALUI','add','pe_bs','pe_count',1),('ALUI','and','pe_bs','pe_bs',-2),('ALUI','mul','pe_bs','pe_bs',2),('ALUI','add','pe_bs','pe_bs',8))
    fields(p,[(4,'pe_page'),(4,'pe_bs')]).goto('PE.wordloop')
    P('PE.wordloop').branch({0:'PE.word'},'PE.wordpad',[('CMP','pe_i','pe_j')])
    p=P('PE.word').a(('LDX','pe_val','pe_i',SORTED),('ALUI','and','pe_val','pe_val',4095),('ALUI','or','pe_val','pe_val',40960));field(p,2,'pe_val').a(('ALUI','add','pe_i','pe_i',1)).goto('PE.wordloop')
    P('PE.wordpad').a(('ALUI','and','pe_odd','pe_count',1)).branch({1:'PE.odd'},'PE.group',[('CMPI','pe_odd',1)])
    field(P('PE.odd'),2,0).goto('PE.group')
    p=P('PE.relend').a(('OLEN','pe_rlsize'),('LDI','zero',0),('OCUT','pe_relblob','zero'))
    align(p,'pe_img','pe_rlsize',pe.SECT_ALIGN).a(('A64','add','pe_img','pe_img','pe_rr'))
    align(p,'pe_end','pe_rlsize',pe.FILE_ALIGN).a(('A64','add','pe_end','pe_end','pe_relf'),('LDI','limit',4294967295)).branch({2:'DEAD.image'},'PE.extent',[('C64U','pe_img','limit')])
    P('PE.extent').branch({2:'DEAD.image'},'PE.header',[('C64U','pe_end','limit')])
    p=P('PE.header');literal(p,b'MZ'+bytes(58));field(p,4,64);literal(p,b'PE\0\0')
    fields(p,[(2,pe.MACHINE[arch]),(2,4),(4,0),(4,0),(4,0),(2,240),(2,0x22)])
    p.a(('A64I','add','pe_entry','entryoff',pe.TEXT_RVA))
    fields(p,[(2,0x20B),(1,14),(1,0),(4,'pe_tf'),(4,0),(4,0),(4,'pe_entry'),(4,pe.TEXT_RVA),(8,pe.IMAGEBASE),(4,pe.SECT_ALIGN),(4,pe.FILE_ALIGN)])
    fields(p,[(2,4),(2,0),(2,0),(2,0),(2,4),(2,0),(4,0),(4,'pe_img'),(4,pe.HDR_FILE),(4,0),(2,3),(2,0x8160),(8,0x100000),(8,0x1000),(8,0x100000),(8,0x1000),(4,0),(4,16)])
    p.a(('A64I','add','pe_cfg','pe_rd',cfg),('A64I','add','pe_iat','pe_rd',iat))
    dirs={1:('pe_rd',40),5:('pe_rr','pe_rlsize'),10:('pe_cfg',pe.LOADCFG),12:('pe_iat',(n+1)*8)}
    for i in range(16):fields(p,[(4,dirs.get(i,(0,0))[0]),(4,dirs.get(i,(0,0))[1])])
    for name,rva,vs,fo,fs,flags in [(b'.text',pe.TEXT_RVA,'endo',pe.HDR_FILE,'pe_tf',0x60000020),(b'.rdata','pe_rd',idlen,'pe_rf',(idlen+511)&-512,0x40000040),(b'.data','pe_dt','pe_dvs','pe_df','pe_datafile',0xC0000040),(b'.reloc','pe_rr','pe_rlsize','pe_relf',None,0x42000040)]:
        if fs is None:align(p,'pe_rsize','pe_rlsize',pe.FILE_ALIGN);fs='pe_rsize'
        literal(p,name.ljust(8,b'\0'));fields(p,[(4,vs),(4,rva),(4,fs),(4,fo),(4,0),(4,0),(2,0),(2,0),(4,flags)])
    p.a(('OLEN','pos')).branch({1:'PE.text'},'DEAD.image',[('CMPI','pos',488)])
    p=P('PE.text');pad(p,pe.HDR_FILE).a(('INPUSH','text_blob')).call('PE.copy');pad(p,'pe_rf')
    p.a(('A64I','add','pe_int','pe_rd',40),('A64I','add','pe_dll','pe_rd',dlloff))
    fields(p,[(4,'pe_int'),(4,0),(4,0),(4,'pe_dll'),(4,'pe_iat')]);literal(p,bytes(20))
    for _ in range(2):
        for offset,raw in names:
            p.a(('A64I','add','pe_name','pe_rd',offset));field(p,8,'pe_name')
        field(p,8,0)
    for offset,raw in names:literal(p,raw)
    literal(p,pe.DLL+b'\0'+bytes(cfg-(dlloff+len(pe.DLL)+1)))
    field(p,4,pe.LOADCFG);literal(p,bytes(pe.COOKIE_FIELD-4));field(p,8,'pe_cookie');literal(p,bytes(pe.LOADCFG-pe.COOKIE_FIELD-8))
    pad(p,'pe_df').a(('LDI','pe_i',0)).goto('PE.data')
    P('PE.data').branch({0:'PE.databyte'},'PE.relcopy',[('CMP','pe_i','stored')])
    P('PE.databyte').a(('LDX','db','pe_i',DATA),('OUTW','db'),('ALUI','add','pe_i','pe_i',1)).goto('PE.data')
    p=P('PE.relcopy');pad(p,'pe_relf').a(('INPUSH','pe_relblob')).call('PE.copy');pad(p,'pe_end').ret()
    g.on('PE.copy',[256],'RET',[('INPOP',)]);g.els('PE.copy','PE.copy',[('COPY',),('ADV',)])
    P('PE.pad').a(('OLEN','pos')).branch({0:'PE.zero',1:'RET'},'DEAD.image',[('C64','pos','pe_pad')])
    byte(P('PE.zero'),0).goto('PE.pad')
