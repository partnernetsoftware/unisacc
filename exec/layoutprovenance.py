"""Stage-generated layout provenance framing, in ordinary model actions.

This is transport evidence about skipped syntax, not native ABI classification.
The storage policy describes this compiler; nativeabi must still certify it.
"""
import sys as _sys, pathlib as _pl
_root=str(_pl.Path(__file__).resolve().parents[1])
if _root not in _sys.path: _sys.path.append(_root)
from exec.facts.load import facts
# Frame constants: exec/facts/top-layoutprovenance-const.tsv.
_C={r['name']:r['value'] for r in facts('top-layoutprovenance-const')}
MAGIC=_C['MAGIC'].encode('latin-1')
RESOURCE=_C['RESOURCE'].encode('latin-1')
POLICY=_C['POLICY']


from pathlib import Path


def _rules(E, P, section, bindings=None):
    """Install one section of layoutprovenance-result.tsv (frame decoder, writer and
    unit loop recorded as rows). Fresh labels are pre-allocated in original order from
    layoutprovenance-fresh.tsv and bound as $name; the caller's start state binds as
    $start. Frame bytes in the rows are MAGIC + stage, then POLICY (see constants)."""
    from finite_rules import install
    root=Path(__file__).parent
    bindings=dict(bindings or {});holders={}
    for line in (root/'layoutprovenance-fresh.tsv').read_text().splitlines()[1:]:
        part,name,kind,prefix=line.split('\t')
        if part==section:bindings[name]=(holders.get(prefix) or holders.setdefault(prefix,P(prefix+'.fresh'))).fresh(kind)
    install(E.g,root,'layoutprovenance',bindings,None,None,section)


def parser(E, P, start):
    import assemble
    u64=lambda E,l,k,r,p,f:assemble.run(assemble.FACTS.parent/'modelinput-manifest.tsv',E,E.P,{},dict(entry=l,key=[('SBOUT',c) for c in k],result=r,present=p,fail=f))  # modelinput-manifest.tsv
    u64(E,'SF3.resource',RESOURCE,'sf3_flag','sf3_present','SF3.fail')
    _rules(E,P,'parser',dict(start=start))
    return 'SF3.start'


def units(E, P, locations=False, accept=None):
    """Validate every unit before name isolation and conservatively merge proof.

    Whole-input prepass retains filename framing and old token payloads. A
    single unknown unit makes the merged source unknown, preventing provenance
    from leaking from one TU to another. Defaults keep the old wire untouched.
    accept: named results of the producer (acc_state, acc_acts = its actions before ACCEPT);
    one gate per key of that state, no graph walk.
    """
    import assemble
    u64=lambda E,l,k,r,p,f:assemble.run(assemble.FACTS.parent/'modelinput-manifest.tsv',E,E.P,{},dict(entry=l,key=[('SBOUT',c) for c in k],result=r,present=p,fail=f))  # modelinput-manifest.tsv
    u64(E,'SFU.resource',RESOURCE,'sfu_flag','sfu_present','SFU.fail')
    _rules(E,P,'located' if locations else 'plain')
    return accept_gates(E,P,'SFU.accept',accept)


def accept_gates(E, P, terminal, accept):
    """One gate per key of the producer's accepting state (named results, no graph walk)."""
    import json
    from finite_rules import install_template
    acts=json.dumps([list(a) for a in accept['acc_acts']])
    G=[dict(index=k,name=accept['acc_state'],key=k,acts=acts) for k in range(257)]
    # Gate rows, the redirect of each ACCEPT edge and SFU.oldaccept: layoutprovenance-template.tsv.
    install_template(E.g,Path(__file__).parent,'layoutprovenance',dict(G=G,terminal=[terminal]),E.P('SFU.fresh').fresh,section='gates')
    return 'SFU.start'
