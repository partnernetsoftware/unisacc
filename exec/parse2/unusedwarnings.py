"""Reference block-local unused-variable warnings on generic model actions.
Binding records follow the existing shared-size scope undo records. Reads use
reference primary()'s token-context rule, not generated tape liveness.
"""
from tokenlocations import TOKEN_POS
import sys as _s; from pathlib import Path as _P; _s.path.insert(0, str(_P(__file__).resolve().parent.parent / 'facts')); from load import facts
globals().update((r['name'], r['value']) for r in facts('unusedwarnings'))

def install(E,P,TIX,undo_size):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root=Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root/('unusedwarnings-'+suffix+'.tsv')).read_text().splitlines()[1:]]
    bindings=dict(TOKEN_POS=TOKEN_POS, ACTIVE=ACTIVE, PREVIOUS=PREVIOUS, USED=USED, ELIGIBLE=ELIGIBLE, NAME=NAME, POSITION=POSITION, NAME_TOKEN=NAME_TOKEN, KIND=KIND, TIX=TIX, undo_size=undo_size)
    p=P('unusedwarnings.bindings')
    for owner,kind,key in rows('fresh'):
        p.cur=owner;bindings[key]=p.fresh(kind)
    sequences={name:E.O(json.loads(value)) for name,value in rows('text')}
    sequences.update((name,[('SBOUT',byte) for byte in json.loads(value).encode()]) for name,value in rows('buffers'))
    tokens=dict(E.TK,identifier=E.TK_ID)
    classes={name:[tokens[token]] for name,token in rows('tokens')}
    bindings.update(('token_'+k,tokens[k]) for k in ('=',';','{','}',')'))
    install_rules(E.g,root,'unusedwarnings',bindings=bindings,sequences=sequences,classes=classes,section="main")
