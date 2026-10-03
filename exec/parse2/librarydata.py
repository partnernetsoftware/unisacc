"""Borrowed writable scalar data: declaration facts and address selection in δ.
Only resource-enabled externs get deferred addresses; real definitions win later.
"""
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('librarydata'))

class _Holder:
    def __init__(self,cur):self.cur=cur

def _template(E,name,facts,cur,sequences=None,bindings=None):
    from pathlib import Path
    from finite_rules import install_template
    holder=_Holder(cur)
    install_template(E.g,Path(__file__).parent,'librarydata',facts,lambda k:E.P.fresh(holder,k),
                     bindings=bindings,sequences=sequences,section=name)

def global_address(E,P,entry,classic,done):
    _template(E,'address',dict(entry=[entry],classic=[classic],done=[done]),entry,
              dict(addr=E.O('  .libraryaddr r0, g_'),comma=E.O(', '),nl=E.O('\n')),
              dict(EXTERN=EXTERN,USED=USED,DEPTH=DEPTH,BASE=BASE))

def install(E,P,b,integers):
    """Stage control lives in librarydata-result.tsv / -byte.tsv (sections storage, record,
    rest); fresh labels are declared in librarydata-fresh.tsv. Python binds only dynamic
    facts: graph constants (this module's and libraryimports'), the parent's banks and tape
    codes (b, E.TK, E.LOC, E.GMARK). The parent-row edits (TOP.st/FN prefixes, GV.storage/GV.record
    moved aside), the integer-type branch and global_address are librarydata-template.tsv."""
    class _Scope:
        a=E.P.a
        def __init__(self,cur):self.cur=cur;self.acts=[]
    from pathlib import Path
    from finite_rules import install as rules
    import libraryimports
    from libraryimports import STRIDE
    g=E.g;root=Path(__file__).parent
    g.labels.add('LD.match')
    bindings={'STRIDE':STRIDE,'E.LOC':E.LOC,'E.GMARK':E.GMARK,'TK.=':E.TK['='],'TK.type=extern':E.TK['type=extern']}
    for k,v in list(globals().items())+[('libraryimports.'+k,v) for k,v in vars(libraryimports).items()]:
        if type(v) is int and v>=1<<40:bindings[k]=v
    classes={}
    for k,v in b.items():
        if type(v) is int:bindings['b.'+k]=v;classes['b.'+k]=[v]
    fresh=[l.split('\t') for l in (root/'librarydata-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
        rules(g,root,'librarydata',bindings,None,classes,name)
    _template(E,'prefix',{},'LD.x')
    _template(E,'hook',{'S':['GV.storage']},'LD.x');section('storage')
    _template(E,'hook',{'S':['GV.record']},'LD.x');section('record')
    _template(E,'scalar',{'I':[dict(code=code,width=width,uns=uns) for _,code,width,uns,_ in integers]},'LD.scalar')
    section('rest')
