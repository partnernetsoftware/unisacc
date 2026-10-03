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
