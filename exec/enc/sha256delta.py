"""SHA-256 transitions for Mach-O page signatures; no hash runtime primitive.
Input: sh_blob. Appends 32 digest bytes, restores input, returns via RET.
Round/initial constants are declarations read from src/back_image.c. The
compression and padding algorithm is explicitly compiled into ordinary actions.
"""
import re
from pathlib import Path
W, K = 95000000, 96000000


def constants():
    s=(Path(__file__).resolve().parents[2]/'src/back_image.c').read_text()
    blocks=re.findall(r'long SHA_K\[64\]\s*=\s*\{([^}]+)\}',s)
    assert len(blocks)==1
    k=[int(x.strip()) for x in blocks[0].split(',') if x.strip()]
    init=re.findall(r'int sha_init\(void\)\s*\{(.*?)return 0;',s,re.S)
    assert len(init)==1
    pairs=re.findall(r'sha_h\[(\d+)\]\s*=\s*(\d+);',init[0])
    assert len(k)==64 and [int(i) for i,_ in pairs]==list(range(8))
    h=[int(v) for _,v in pairs]
    assert all(0<=x<2**32 for x in k+h)
    return k,h


def install(E):
    P=E.P
    ks,hs=constants()
    def rotate(p,src,n,dst):
        return p.a(('A64I','shr',dst,src,n),('A64I','shl','sh_rot',src,32-n),
                   ('A64','or',dst,dst,'sh_rot'),('A64I','and',dst,dst,4294967295))
    def sigma(p,src,ns,dst,shift=False):
        rotate(p,src,ns[0],dst);rotate(p,src,ns[1],'sh_tmp')
        p.a(('A64','xor',dst,dst,'sh_tmp'))
        if shift:p.a(('A64I','shr','sh_tmp',src,ns[2]))
        else:rotate(p,src,ns[2],'sh_tmp')
        return p.a(('A64','xor',dst,dst,'sh_tmp'))
    p=P('SHA256').a(('INPUSH','sh_blob'),('XLEN','sh_len'),('LDI','sh_i',0),
        ('A64I','mul','sh_bits','sh_len',8),('A64I','add','sh_total','sh_len',72),
        ('A64I','and','sh_total','sh_total',-64),('A64I','sub','sh_tail','sh_total',8))
    for i,v in enumerate(ks):p.a(('LDI','sh_x',i),('LDI','sh_y',v),('STX','sh_x',K,'sh_y'))
    for i,v in enumerate(hs):p.a(('LDI','sh_h'+str(i),v))
    p.goto('SH.block')
    P('SH.block').a(('LDI','sh_j',0)).goto('SH.byte')
    P('SH.byte').branch({0:'SH.input',1:'SH.pad'},'SH.tail',[('C64','sh_i','sh_len')])
    P('SH.input').a(('BYTE','sh_v'),('ADV',)).goto('SH.put')
    P('SH.pad').a(('LDI','sh_v',128)).goto('SH.put')
    P('SH.tail').branch({0:'SH.zero'},'SH.length',[('C64','sh_i','sh_tail')])
    P('SH.zero').a(('LDI','sh_v',0)).goto('SH.put')
    P('SH.length').a(('A64','sub','sh_shift','sh_total','sh_i'),('ALUI','sub','sh_shift','sh_shift',1),
        ('ALUI','mul','sh_shift','sh_shift',8),('A64','shr','sh_v','sh_bits','sh_shift'),('ALUI','and','sh_v','sh_v',255)).goto('SH.put')
    P('SH.put').a(('ALUI','and','sh_mod','sh_j',3)).branch({1:'SH.wordzero'},'SH.wordload',[('CMPI','sh_mod',0)])
    P('SH.wordzero').a(('LDI','sh_word',0)).goto('SH.word')
    P('SH.wordload').a(('ALUI','sar','sh_idx','sh_j',2),('LDX','sh_word','sh_idx',W)).goto('SH.word')
    P('SH.word').a(('A64I','shl','sh_word','sh_word',8),('A64','or','sh_word','sh_word','sh_v'),
        ('ALUI','sar','sh_idx','sh_j',2),('STX','sh_idx',W,'sh_word'),('ALUI','add','sh_j','sh_j',1),('ALUI','add','sh_i','sh_i',1)).branch({1:'SH.expandinit'},'SH.byte',[('CMPI','sh_j',64)])
    P('SH.expandinit').a(('LDI','sh_j',16)).goto('SH.expand')
    p=P('SH.expand').a(('ALUI','sub','sh_idx','sh_j',15),('LDX','sh_x','sh_idx',W))
    sigma(p,'sh_x',(7,18,3),'sh_s0',True)
    p.a(('ALUI','sub','sh_idx','sh_j',2),('LDX','sh_x','sh_idx',W));sigma(p,'sh_x',(17,19,10),'sh_s1',True)
    p.a(('ALUI','sub','sh_idx','sh_j',16),('LDX','sh_x','sh_idx',W),('ALUI','sub','sh_idx','sh_j',7),('LDX','sh_y','sh_idx',W),
        ('A64','add','sh_word','sh_x','sh_y'),('A64','add','sh_word','sh_word','sh_s0'),('A64','add','sh_word','sh_word','sh_s1'),
        ('A64I','and','sh_word','sh_word',4294967295),('STX','sh_j',W,'sh_word'),('ALUI','add','sh_j','sh_j',1)).branch({1:'SH.roundinit'},'SH.expand',[('CMPI','sh_j',64)])
    p=P('SH.roundinit').a(('LDI','sh_j',0))
    for i,c in enumerate('abcdefgh'):p.a(('COPYW','sh_'+c,'sh_h'+str(i)))
    p.goto('SH.round')
    p=P('SH.round');sigma(p,'sh_e',(6,11,25),'sh_s1')
    p.a(('A64','and','sh_ch','sh_e','sh_f'),('A64I','xor','sh_x','sh_e',4294967295),('A64','and','sh_x','sh_x','sh_g'),('A64','xor','sh_ch','sh_ch','sh_x'),
        ('LDX','sh_x','sh_j',K),('LDX','sh_y','sh_j',W),('A64','add','sh_t1','sh_h','sh_s1'),('A64','add','sh_t1','sh_t1','sh_ch'),
        ('A64','add','sh_t1','sh_t1','sh_x'),('A64','add','sh_t1','sh_t1','sh_y'))
    sigma(p,'sh_a',(2,13,22),'sh_s0')
    p.a(('A64','and','sh_maj','sh_a','sh_b'),('A64','and','sh_x','sh_a','sh_c'),('A64','xor','sh_maj','sh_maj','sh_x'),('A64','and','sh_x','sh_b','sh_c'),('A64','xor','sh_maj','sh_maj','sh_x'),('A64','add','sh_t2','sh_s0','sh_maj'),
        ('COPYW','sh_h','sh_g'),('COPYW','sh_g','sh_f'),('COPYW','sh_f','sh_e'),('A64','add','sh_e','sh_d','sh_t1'),('A64I','and','sh_e','sh_e',4294967295),
        ('COPYW','sh_d','sh_c'),('COPYW','sh_c','sh_b'),('COPYW','sh_b','sh_a'),('A64','add','sh_a','sh_t1','sh_t2'),('A64I','and','sh_a','sh_a',4294967295),('ALUI','add','sh_j','sh_j',1)).branch({1:'SH.accumulate'},'SH.round',[('CMPI','sh_j',64)])
    p=P('SH.accumulate')
    for i,c in enumerate('abcdefgh'):p.a(('A64','add','sh_h'+str(i),'sh_h'+str(i),'sh_'+c),('A64I','and','sh_h'+str(i),'sh_h'+str(i),4294967295))
    p.branch({0:'SH.block'},'SH.digest',[('C64','sh_i','sh_total')])
    p=P('SH.digest').a(('INPOP',))
    for i in range(8):
        for shift in (24,16,8,0):p.a(('A64I','shr','sh_x','sh_h'+str(i),shift),('OUTW','sh_x'))
    p.ret()
