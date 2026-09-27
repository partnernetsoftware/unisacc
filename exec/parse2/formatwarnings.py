"""Format checks consume the real argument type; no speculative parsing.
The format scanner preserves the existing warning policy, not full printf
validation. Conversion identity comes from bytes, never from emitted tape.
"""
from tokenlocations import TOKEN_POS
from unusedwarnings import NAME_TOKEN

def install(E,P,DBL,FLT,FPB,SBB):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('formatwarnings-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    bindings=dict(TOKEN_POS=TOKEN_POS, NAME_TOKEN=NAME_TOKEN, DBL=DBL, FLT=FLT, FPB=FPB, SBB=SBB)
    p=P('formatwarnings.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    sequences.update((name,[('SBOUT',byte) for byte in json.loads(value).encode()]) for name,value in rows('buffers'))
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,string=E.TK_STR)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    install_rules(E.g,root,'formatwarnings',bindings=bindings,sequences=sequences,classes=classes,section="main")
    from finite_rules import load as load_rules
    for name,message in rows('messages'):
        rules=load_rules(root/'formatwarnings-message-rule.tsv',
                         {'message':[('SBOUT',byte) for byte in json.loads(message).encode()]},bindings={'entry':'WF.msg.'+name})
        for state,row in rules.items():
            for key,(target,acts) in row.items():E.g.on(state,[key],target,acts,'r')
