"""USBIND2 candidate selection in delta, normalized to the existing USBIND1 decoder.
Winner priority is (origin, ordinal), independent of record order. Output names
retain first-seen order; no host parser or host winner selection participates.
V1 passthrough does not alter the caller's input/output/string builder.
"""
import sys as _sys, pathlib as _pl
_root=str(_pl.Path(__file__).resolve().parents[1])
if _root not in _sys.path: _sys.path.append(_root)
from exec.facts.load import facts
# Bank constants: exec/facts/top-modelcandidates-banks.tsv; resource key: -const.tsv.
globals().update({r['name']:r['value'] for r in facts('top-modelcandidates-banks')})
_C={r['name']:r['value'] for r in facts('top-modelcandidates-const')}

def install(E, fail='DEAD'):
    """Stage control lives in modelcandidates-result.tsv (sections pre, post). Python binds only
    dynamic facts: the bank constants, the fail continuation, and fresh labels pre-allocated in
    original order (modelcandidates-fresh.tsv) without registering any extra named state.
    Kept in Python: install_signature (MS.*) and the modelinput.u64 helper for MC.callablecap,
    which expands its own fact-dependent states between the two sections."""
    from pathlib import Path
    from finite_rules import install as rules
    if 'MC.normalize' in E.g.st: return
    import assemble
    assemble.run(assemble.FACTS.parent/'modelsignature-manifest.tsv',E,E.P,{},dict(fail=fail))
    root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'modelcandidates-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(LIST=LIST,SEEN=SEEN,DUP=DUP,RANK=RANK,BEGIN=BEGIN,END=END,ORIGIN=ORIGIN,LENGTH=LENGTH,fail=fail)
    holder=type('FreshScope',(),{'cur':'MC'})()
    def section(name):
        for part,key,kind in fresh:
            if part==name:bindings[key]=E.P.fresh(holder,kind)
        rules(E.g,root,'modelcandidates',bindings,None,None,name)
    section('pre')
    import assemble
    u64=lambda E,l,k,r,p,f:assemble.run(assemble.FACTS.parent/'modelinput-manifest.tsv',E,E.P,{},dict(entry=l,key=[('SBOUT',c) for c in k],result=r,present=p,fail=f))  # modelinput-manifest.tsv
    u64(E,'MC.callablecap',_C['callablecap'].encode('latin-1'),'mc_callablecap','mc_callablepresent','MC.fail')
    section('post')
