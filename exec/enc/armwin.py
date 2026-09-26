"""Windows ARM64 gate encoding rules; imports/argument template are declarations.
No reference encoder is called. Address math uses the shared ADRP transitions.
"""
from unisa.image.pe import IMPORTS
OPS=('exit','read','write','mmap','mprotect','munmap','close','open','lseek','unlink','rename')
RCS=('none','wcount','bool_inv','bool_neg','dword_sx')
IMP=80000000


def init(p):
    for key in OPS+RCS+("'none",):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
    for i,name in enumerate(IMPORTS):
        p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))


def reset(p):
    for r in ('catop','winimp','retconv','hstd','written','scr0','scr1'):
        p.a(('LDI','wm_'+r,0))
    return p


def install(E,word):
    P,g=E.P,E.g
    for key in ('catop','winimp','retconv'):
        P('META.'+key).a(('COPYW','wm_'+key,'mv')).goto('AFTER')
    for key in ('hstd','written','scr0','scr1'):
        P('META.'+key).a(('JUMP','vs'),('LDI','wn',0)).goto('WM.'+key)
        g.on('WM.'+key,range(48,58),'WM.bound.'+key,[('BYTE','t'),('ALUI','sub','t','t',48),('A64I','mul','wn','wn',10),('A64','add','wn','wn','t')])
        P('WM.bound.'+key).a(('LDI','limit',2147483647)).branch({2:'FAIL'},'WM.next.'+key,[('C64U','wn','limit')])
        P('WM.next.'+key).a(('ADV',)).goto('WM.'+key)
        g.on('WM.'+key,[32,10,256],'AFTER',[('COPYW','wm_'+key,'wn')]);g.els('WM.'+key,'FAIL',[])
    P('WGATE').branch({1:'WG.import'},'FAIL',[('CMPI','target_os',3)])
    P('WG.import').a(('LDX','imp_index','wm_winimp',IMP)).branch({1:'FAIL'},'WG.ops',[('CMPI','imp_index',0)])
    p=P('WG.ops').a(('ALUI','sub','wi_imp','imp_index',1))
    for op in OPS:
        p.branch({1:'WG.'+op},'WG.next.'+op,[('CMP','wm_catop','wi_'+op)]);p=P('WG.next.'+op)
    p.goto('FAIL')
    def words(p,*vs):
        for v in vs:word(p.a(('LDI','w',v)))
        return p
    def addr(p,r,value):
        return p.a(('LDI','ad_r',r),('COPYW','ad_v','wm_'+value)).call('AD.data').call('ADRP')
    def call(p,name=None):
        p.a(('COPYW','imp_index','wi_imp') if name is None else ('LDI','imp_index',IMPORTS.index(name))).call('WIN.call')
        return p
    p=P('WG.fd');words(p,0xF1000C1F,0x54000082)
    addr(p,16,'hstd');words(p,0xF8607800|(16<<5)).ret()
    for op in ('exit','mmap','unlink'):
        call(P('WG.'+op)).goto('WG.tail')
    for op in ('read','write'):
        p=P('WG.'+op).call('WG.fd');addr(p,3,'written');words(p,0xAA1F03E4);call(p).goto('WG.tail')
    p=P('WG.close').call('WG.fd');call(p).goto('WG.tail')
    p=P('WG.mprotect');addr(p,3,'written');call(p);words(p,0x92800000)
    addr(p,16,'scr0');words(p,0xF9400201)
    addr(p,16,'scr1');words(p,0xF9400202);call(p,'FlushInstructionCache').goto('WG.tail')
    p=P('WG.munmap');words(p,0xD2900002,0xAA1F03E1);call(p).goto('WG.tail')
    p=P('WG.open');words(p,0xAA0203E4,0xD2800062,0xAA1F03E3,0xD2801005,0xAA1F03E6);call(p).goto('WG.tail')
    p=P('WG.lseek');words(p,0xAA0203E3,0xAA1F03E2);p.call('WG.fd');call(p).goto('WG.tail')
    p=P('WG.rename');words(p,0xD2800022);call(p).goto('WG.tail')
    p=P('WG.tail').branch({1:'LINE'},'WG.tail.named',[('CMPI','wm_retconv',0)])
    p=P('WG.tail.named')
    for rc in RCS+("'none",):
        p.branch({1:'WT.'+rc},'WT.next.'+rc,[('CMP','wm_retconv','wi_'+rc)]);p=P('WT.next.'+rc)
    p.goto('FAIL')
    P('WT.none').goto('LINE');P("WT.'none").goto('LINE')
    p=P('WT.wcount');addr(p,16,'written');words(p,0xF9400200).goto('LINE')
    words(P('WT.bool_inv'),0xF100001F,0x9A9F17E0).goto('LINE')
    words(P('WT.bool_neg'),0x7100001F,0xDA9F13E0).goto('LINE')
    words(P('WT.dword_sx'),0x93407C00).goto('LINE')
    # The established command-line parser is a declared machine-code template.
    # Migration of this template's algorithm is not claimed here.
    from unisa.emit_arm import WINARGS_BODY
    p=P('EMIT.36').branch({1:'WA.emit'},'FAIL',[('CMPI','target_os',3)])
    p=P('WA.emit');call(p,'GetCommandLineA');words(p,0xAA0003E1,0xD2800002)
    p.a(('LDI','ad_r',3),('COPYW','ad_v','a2')).call('AD.data').call('ADRP')
    words(p,*WINARGS_BODY)
    for arg,r in (('a0',2),('a1',3)):
        p.a(('LDI','ad_r',16),('COPYW','ad_v',arg)).call('AD.data').call('ADRP')
        words(p,0xF9000000|(16<<5)|r)
    p.goto('LINE')
