"""The reference's laststmt/return warning, not a new flow analysis.
Enabled only by the development --warnings mode. The development CLI uses it with single- and multi-unit location maps.
"""
from tokenlocations import TOKEN_POS
import sys as _s; from pathlib import Path as _P; _s.path.insert(0, str(_P(__file__).resolve().parent.parent / 'facts')); from load import facts
MESSAGE=facts('returnwarnings')[0].encode()


def install(E,P,SBB):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('returnwarnings-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    bindings=dict(TOKEN_POS=TOKEN_POS, SBB=SBB)
    p=P('returnwarnings.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    sequences.update((name,[('SBOUT',byte) for byte in json.loads(value).encode()]) for name,value in rows('buffers'))
    sequences['message']=[('SBOUT',byte) for byte in MESSAGE]
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,identifier=E.TK_ID,number=E.TK_NUM,floating=E.TK_FNUM)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    install_rules(E.g,root,'returnwarnings',bindings=bindings,sequences=sequences,classes=classes,section="main")
