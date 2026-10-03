"""USLSIG2 recursive layout serializer, executed as ordinary delta actions.
No host classification. Ordered member facts are the parser's actual layout.
"""
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('librarytypes'))

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
    # and the frame save/restore sequences built from regs. The integer-type branch
    # is a template (librarytypes-template.tsv).
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules, install_template
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
    # Integer-type branch: librarytypes-template.tsv section ints, one case per integer fact.
    install_template(g,root,'librarytypes',{'I':[{'code':code,'size':size,'uns':int(uns)} for _,code,size,uns,_ in integers]},
                     lambda kind:E.P.fresh(_Scope('LTY'),kind),section='ints')
    section('mid')
    from librarycallbackgraph import install as graph_install
    graph_install(E,P,b,frame)
    section('tail')

    from librarytypesv3 import install as v3_install
    v3_install(E,P,b,frame,blob)
