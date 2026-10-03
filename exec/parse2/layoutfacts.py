"""Retain ordered parser layout facts without changing SMEM/tape/USLSIG2.
These are actual parser projections, not complete modifier/packing declarations.
Enabled by explicit u64 \\0library/layoutfacts=1 or library symbols presence.
Entry key = sid*128+ordinal; count/union/flatten/seen are keyed only by sid.
"""
import sys
COUNT,KIND,BASE,DEPTH,ARRAY,OFFSET,BITOFFSET,BITWIDTH,STORAGE,ALIGN,CHILD,MEMBER,SHAPE,SIGNED,UNION,FLATTEN,SEEN=(i<<40 for i in range(600,617))
RESERVED_BANKS=tuple(i<<40 for i in range(600,620))
ENTRY_STRIDE=128
RECORD_LIMIT=128
ENTRY_LIMIT=128
ORDINARY,NAMED_BITFIELD,ANONYMOUS_BITFIELD,ZERO_WIDTH_BARRIER,ANONYMOUS_AGGREGATE=range(5)
ENTRY_FIELDS=(('kind',KIND),('base',BASE),('depth',DEPTH),('array',ARRAY),('offset',OFFSET),('bitoffset',BITOFFSET),('bitwidth',BITWIDTH),('storage',STORAGE),('align',ALIGN),('child',CHILD),('member',MEMBER),('shape',SHAPE),('signed',SIGNED))
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
 from finite_rules import install as rules
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
  orig='LF.original.'+state;assert orig not in g.st
  g.st[orig]=g.st.pop(state);g.labels.add(orig)
  section(part)
  g.st[state]=g.st['LF.entry.'+state]
 return 'LF.start'
