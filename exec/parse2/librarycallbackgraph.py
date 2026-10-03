"""Complete declaration-owned callback graph serializer in ordinary delta.
Record-local first traversal IDs are registered before child descriptors.
Only signature declaration facts are consumed; no callable bridge is claimed.
"""
SIGWIREID,SIGWIREMARK,SIGFRAME=(i<<40 for i in range(420,423))
assert not set(range(420,423)) & (set(range(72,83))|set(range(300,358))|set(range(400,414))|set(range(430,438)))
def install(E,P,b,frame):
 import libraryexports
 from pathlib import Path
 from finite_rules import install as rules
 regs=('lcg_sig','lcg_count','lcg_epoch','lcg_wireid','lcg_i','lcg_parent_support','lcg_var','lcg_mode')
 def sigframe(p,op):
  for i,r in enumerate(regs):
   p.a(('ALUI','mul','lcg_slot','lx_recursion',16),('ALUI','add','lcg_slot','lcg_slot',i))
   p.a((op,'lcg_slot',SIGFRAME,r) if op=='STX' else (op,r,'lcg_slot',SIGFRAME))
  return p
 # Stage control lives in librarycallbackgraph-result.tsv; fresh labels are declared in
 # librarycallbackgraph-fresh.tsv. Python binds only dynamic facts: the parent's bank
 # constants (b), libraryexports ranks/marks, and the child save/restore sequences built
 # from the parent's frame and regs. No residue.
 class _Scope:
  a=E.P.a
  def __init__(self,cur):self.cur=cur;self.acts=[]
 g=E.g;root=Path(__file__).parent
 bindings={'SIGWIREID':SIGWIREID,'SIGWIREMARK':SIGWIREMARK,'SIGFRAME':SIGFRAME}
 for k in ('RETURNRANK','PARAMRANK','VARIADIC','PARAMMARK','PARAMDEPTH','PARAMBASE','PARAMSHAPE','SIGEPOCH'):
  bindings['libraryexports.'+k]=getattr(libraryexports,k)
 for k,v in b.items():
  if type(v) is int:bindings['b.'+k]=v
 def saved(op):
  p=_Scope('LCG')
  if op=='STX':frame(p,op);sigframe(p,op)
  else:sigframe(p,op);frame(p,op)
  return p.acts
 sequences={'child.STX':saved('STX'),'child.LDX':saved('LDX')}
 for part,key,kind in (l.split('\t') for l in (root/'librarycallbackgraph-fresh.tsv').read_text().splitlines()[1:]):
  bindings[key]=E.P.fresh(_Scope(key.split('.')[0]),kind)
 rules(g,root,'librarycallbackgraph',bindings,sequences,None,'s1')
