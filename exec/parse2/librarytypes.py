"""USLSIG2 recursive layout serializer, executed as ordinary delta actions.
No host classification. Ordered member facts are the parser's actual layout.
"""
FRAME = 80 << 40

def install(E,P,b,integers,union_bank):
    regs=('d_depth','d_base','d_shape','d_array','d_arraybytes','d_class','d_width',
          'd_unsigned','d_align','d_tag','d_sid','d_members','d_i','d_key','d_old',
          'd_payload','d_blob','d_end','d_stride','d_return','d_dim','d_rank','d_fprank')
    assert len(regs)<64
    def frame(p,op):
        for i,r in enumerate(regs):
            p.a(('ALUI','mul','d_slot','lx_recursion',64),('ALUI','add','d_slot','d_slot',i))
            p.a((op,'d_slot',FRAME,r) if op=='STX' else (op,r,'d_slot',FRAME))
        return p
    def blob(p,r):
        return p.a(('INPUSH',r),('XLEN','d_end'),('SPAN2','lx_zero','d_end'),('INPOP',))
    def child(p):
        frame(p,'STX')
        p.call('LX.descriptor')
        return frame(p,'LDX')
    # Stage control lives in librarytypes-result.tsv (sections head, mid, tail); fresh
    # labels are declared in librarytypes-fresh.tsv. Python binds only dynamic facts:
    # the parent's bank constants and tape codes (b), the union bank, the i32 width,
    # and the frame save/restore sequences built from regs. Residue: the integer-type
    # branch (keys and state names come from the integers list).
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules
    g=E.g;root=Path(__file__).parent
    bindings={'union_bank':union_bank,'isize':next(size for name,code,size,uns,narrow in integers if name=='i32')}
    classes={}
    for k,v in b.items():
        if type(v) is int:bindings['b.'+k]=v;classes['b.'+k]=[v]
    def saved(op):
        p=_Scope('LTY');frame(p,op);return p.acts
    sequences={'frame.STX':saved('STX'),'frame.LDX':saved('LDX')}
    fresh=[l.split('\t') for l in (root/'librarytypes-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
        rules(g,root,'librarytypes',bindings,sequences,classes,name)
    section('head')
    P('LTY.scalar').branch({code:'LTY.int.'+str(code) for _,code,_,_,_ in integers},'LTY.other',[('RLD','d_base')])
    for _,code,size,uns,_ in integers:
        P('LTY.int.'+str(code)).a(('LDI','d_class',1),('LDI','d_width',size),('LDI','d_align',size),('LDI','d_unsigned',uns)).goto('LTY.arraycheck')
    section('mid')
    from librarycallbackgraph import install as graph_install
    graph_install(E,P,b,frame)
    section('tail')

    from librarytypesv3 import install as v3_install
    v3_install(E,P,b,frame,blob)
