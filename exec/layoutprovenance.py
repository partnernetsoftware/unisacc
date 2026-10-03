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
    from modelinput import u64
    u64(E,'SF3.resource',RESOURCE,'sf3_flag','sf3_present','SF3.fail')
    _rules(E,P,'parser',dict(start=start))
    return 'SF3.start'


def units(E, P, locations=False):
    """Validate every unit before name isolation and conservatively merge proof.

    Whole-input prepass retains filename framing and old token payloads. A
    single unknown unit makes the merged source unknown, preventing provenance
    from leaking from one TU to another. Defaults keep the old wire untouched.
    Kept in Python: the u64 resource helper and accept_gates (one gate per
    ACCEPT transition already installed, a fact-dependent state set).
    """
    from modelinput import u64
    u64(E,'SFU.resource',RESOURCE,'sfu_flag','sfu_present','SFU.fail')
    _rules(E,P,'located' if locations else 'plain')
    return accept_gates(E,P,'SFU.accept')


def accept_gates(E, P, terminal):
    """Fact-dependent: one gate per already-installed ACCEPT transition."""
    g=E.g
    accept_rows=[]
    for name,(mode,row) in list(g.st.items()):
        if name.startswith('SFU.'):continue
        for key,(target,q) in list(row.items()):
            acts=list(g.seqs[q])
            if any(a[0]=='ACCEPT' for a in acts):accept_rows.append((name,key,target,acts))
    # Gate rows, the redirect of each ACCEPT edge and SFU.oldaccept: layoutprovenance-template.tsv.
    import json
    from finite_rules import install_template
    G=[]
    for index,(name,key,target,acts) in enumerate(accept_rows):
        assert acts[-1]==('ACCEPT',)
        G.append(dict(index=index,name=name,key=key,acts=json.dumps([list(a) for a in acts[:-1]])))
    install_template(g,Path(__file__).parent,'layoutprovenance',dict(G=G,terminal=[terminal]),E.P('SFU.fresh').fresh,section='gates')
    return 'SFU.start'
