"""Retain ordered parser layout facts without changing SMEM/tape/USLSIG2.
These are actual parser projections, not complete modifier/packing declarations.
Enabled by explicit u64 \\0library/layoutfacts=1 or library symbols presence.
Entry key = sid*128+ordinal; count/union/flatten/seen are keyed only by sid.
"""
import sys
import sys as _s,pathlib as _p
_s.path.insert(0,str(_p.Path(__file__).parents[1]/'facts'))
from load import facts as _facts
_t=lambda v:tuple(map(_t,v)) if isinstance(v,list) else v
globals().update((_r['name'],_t(_r['value'])) for _r in _facts('layoutfacts'))
def entry_key(sid,ordinal):
 assert 1<=sid<=RECORD_LIMIT and 0<=ordinal<ENTRY_LIMIT
 return sid*ENTRY_STRIDE+ordinal

def install(E,P,b,start):
 # Stage control lives in layoutfacts-result.tsv (s1 fact procedures, s2-s6 one hook
 # each); fresh labels are declared in layoutfacts-fresh.tsv. Python binds only dynamic
 # facts (start label, the fact banks, the parent's pools b) and keeps the residue: the
 # resource reader u64, the bank-collision assert, and each hook's rename of the hooked
 # state to LF.original.* plus aliasing it to LF.entry.* (existing actions/observations
 # run after fact capture unchanged).
 from modelinput import u64
 from pathlib import Path
 from finite_rules import install as rules, install_template
 g=E.g;root=Path(__file__).parent
 assert not set(RESERVED_BANKS).intersection(v for m in tuple(sys.modules.values()) if m is not None and m.__name__!=__name__ for v in vars(m).values() if type(v) is int and v>=1<<40)
 u64(E,'LF.resource',b'\0library/layoutfacts','lf_flag','lf_present','LF.fail')
 bindings={'start':start,'COUNT':COUNT,'FLATTEN':FLATTEN,'SEEN':SEEN,'UNION':UNION}
 for name,bank in ENTRY_FIELDS:bindings[name.upper()]=bank
 for pool in ('MBS','MPT','MAR','MOF','BFW','MSZ','SHAPE_IDS','SHAPE','BFO','BFS','SBB'):bindings['b.'+pool]=b[pool]
 class _Scope:
  def __init__(self,cur):self.cur=cur
 fresh=[l.split('\t') for l in (root/'layoutfacts-fresh.tsv').read_text().splitlines()[1:]]
 def section(name):
  for part,key,kind,prefix in fresh:
   if part==name:bindings[key]=E.P.fresh(_Scope(prefix),kind)
  rules(g,root,'layoutfacts',bindings,None,None,name)
 section('s1')
 for part,state in (('s2','SB.go'),('s3','SB.memberput'),('s4','BF.padding'),('s5','SB.anonbegin'),('s6','SB.anonend')):
  # hook: move state to LF.original.*, install the wrapper, re-enter via LF.entry.* (layoutfacts-template.tsv)
  install_template(g,root,'layoutfacts',{'S':[state]},None,section='hook')
  section(part)
  install_template(g,root,'layoutfacts',{'S':[state]},None,section='entry')
 return 'LF.start'
