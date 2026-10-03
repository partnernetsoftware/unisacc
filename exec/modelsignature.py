"""Shared model USLSIG2/3 TypeGraph framing/canonicalisation, not a host parser.
MS.canonical: ms_blob/ms_len -> ms_canon/ms_canonlen, name/nameid, count,
variadic/mode/support, resultkind/resultwidth and ms_version=2/3. All scratch uses ms_.
V3 canonical begins USLSIG3\n; V2 canonical bytes retain their legacy framing.
Input/output restored; SB scratch unspecified. Recursive base/shape become zero.
"""
import sys as _sys, pathlib as _pl
_root=str(_pl.Path(__file__).resolve().parents[1])
if _root not in _sys.path: _sys.path.append(_root)
from exec.facts.load import facts
# Bank constants (descriptor 350.., depth 410.., V3 scratch 620..): exec/facts/top-modelsignature-banks.tsv.
globals().update({r['name']:r['bank']<<40 for r in facts('top-modelsignature-banks')})
_C={r['name']:r['value'] for r in facts('top-modelsignature-const')}
assert not set(range(410,414)) & (set(range(350,358)) | set(range(400,408)))
# V3 scratch is separate from legacy indexing and source LF banks 600..619.
assert not set(range(620,640)) & (set(range(350,358)) | set(range(400,408)) | set(range(410,414)) | set(range(430,438)) | set(range(600,620)))
def install(E,fail='DEAD'):
 """Stage control lives in modelsignature-result.tsv (section main): framing, records,
 descriptor walk (eq/maxv rows, LDX/STX bank moves with banks bound as constants), the
 byte/u64/write64 helpers. Python keeps only the guard, the reject text and
 u64(E,'MS.callablecap',...), which builds its own states and fresh labels; the table's
 fresh labels are allocated afterwards in the original order (global counter)."""
 import pathlib
 from finite_rules import install as install_rules
 P=E.P;root=pathlib.Path(__file__).parent
 if 'MS.canonical' in E.g.st:return
 install_rules(E.g,root,'modelsignature',dict(fail=fail),{},None,'head')
 P('MS.return').ret()
 from modelinput import u64
 u64(E,'MS.callablecap',_C['callablecap'].encode('latin-1'),'ms_callablecap','ms_callablepresent','MS.fail')
 bindings={k:v for k,v in globals().items() if k.isupper() and type(v) is int}
 p=P('MS.fresh')
 for line in (root/'modelsignature-fresh.tsv').read_text().splitlines()[1:]:
  part,key,kind=line.split('\t')
  if part=='main':bindings[key]=p.fresh(kind)
 install_rules(E.g,root,'modelsignature',bindings,{},None,'main')
