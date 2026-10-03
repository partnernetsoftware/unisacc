"""Typed borrowed data addresses resolved by the lowering delta.
Real tape data definitions retain ordinary .lea semantics. Other names must
match a writable USBIND1 data record and all four canonical ABI facts.
Stage control lives in librarydata-byte/result.tsv (finite_rules), fresh labels in
librarydata-fresh.tsv, pre-allocated per section in the original global order.
Python binds dynamic facts: token ids (.libraryaddr, .lea), table bases, the
os capability target. Residue: the renames of C.dispatch and LBI.emit into
LBD.original.* (they create states, which a table row cannot do).
"""
from pathlib import Path
def install(E,os_,ids):
 from finite_rules import install as rules
 from code import REG,TXT
 from data import DEFINED
 from modelbindings import IDS,ADDRESS,DESC,KIND,SUPPORTED,WRITABLE,EXTENT,STRIDE
 g=E.g;root=Path(__file__).parent
 b=dict(addr=ids['.libraryaddr'],lea=ids['.lea'],REG=REG,TXT=TXT,DEFINED=DEFINED,IDS=IDS,ADDRESS=ADDRESS,DESC=DESC,
        KIND=KIND,SUPPORTED=SUPPORTED,WRITABLE=WRITABLE,EXTENT=EXTENT,STRIDE=STRIDE,
        arity='LBD.arity' if os_ in ('osx','lnx','win') else 'LBI.fail')
 fresh=[l.split('\t') for l in (root/'librarydata-fresh.tsv').read_text().splitlines()[1:]];ps={}
 def section(name):
  for sec,k,kind,prefix in fresh:
   if sec==name:b[k]=ps.setdefault(prefix,E.P(prefix)).fresh(kind)
  rules(g,root,'librarydata',b,section=name)
 g.st['LBD.original.dispatch']=g.st.pop('C.dispatch');g.labels.add('LBD.original.dispatch')
 section('s1')
 g.st['LBD.original.emit']=g.st.pop('LBI.emit');g.labels.add('LBD.original.emit')
 for i in range(2,11):section('s%d'%i)
