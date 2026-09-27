"""Reference intptr_check for local scalar initializers and assignments.
This preserves the reference's syntactic lone-zero exemption and call flag;
it is not a general C constant-expression or conversion checker.
"""
from tokenlocations import TOKEN_POS
MESSAGE=b'incompatible integer to pointer conversion [-Wint-conversion]'

def install(E,P,DBL,FLT,FPB,SBB):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('intwarnings-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    bindings=dict(TOKEN_POS=TOKEN_POS, DBL=DBL, FLT=FLT, FPB=FPB, SBB=SBB)
    p=P('intwarnings.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    sequences.update((name,[('SBOUT',byte) for byte in json.loads(value).encode()]) for name,value in rows('buffers'))
    sequences['message']=[('SBOUT',byte) for byte in MESSAGE]
    for name,value in rows('stack'):
        method,slots=json.loads(value);p.acts=[]
        sequences[name]=getattr(p,method)(*slots).acts
    tokens=dict(E.TK,number=E.TK_NUM)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    install_rules(E.g,root,'intwarnings',bindings=bindings,sequences=sequences,classes=classes,section="main")
