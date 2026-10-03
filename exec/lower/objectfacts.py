"""Object-route facts: tape encounter order and linkage, computed by delta actions.
No text parser or symbol plan executes in the Python constructor. Stage control lives in
objectfacts-byte.tsv / objectfacts-result.tsv (finite_rules). Python binds only dynamic
facts: the SHAPE-derived prelude sequence, the interned directive key registers, the
table bases, and fresh labels (objectfacts-fresh.tsv, created in recorded order).
Residue: the rename of the wrapped states (START, WORD.end, R.second, HEAD, H.end) into
OF.original.* -- it creates states, which a table row cannot do.
"""
import pathlib
from finite_rules import install as install_rules
ORD, NAMES, GLOBAL, EXTERN, SHAPES = (i << 40 for i in range(240,245))

def install(E):
    from unisa.tape import SHAPE
    g=E.g;root=pathlib.Path(__file__).parent
    for state in ('START','WORD.end','R.second','HEAD','H.end'):
        name='OF.original.'+state;assert name not in g.st
        g.st[name]=g.st.pop(state);g.labels.add(name)
    start=E.P('START').a(('MARK','of_zero'),('LDI','of_count',0),('SBCLR',),[('SBOUT',c) for c in b'\0cli/funit'],('SBFIND','of_resource'),('BLEN','of_unit','of_resource'))
    words=list(SHAPE)+['.global','.extern','.bss','.str','.unit','.gdef']
    bindings=dict(ORD=ORD,NAMES=NAMES,GLOBAL=GLOBAL,EXTERN=EXTERN,SHAPES=SHAPES)
    for i,w in enumerate(words):
        start.a(('SBCLR',),[('SBOUT',c) for c in w.encode()],('SBINTERN','of_key'+str(i)))
        if w in SHAPE:
            for j,k in enumerate(SHAPE[w]):
                if k in 'Ls':start.a(('ALUI','mul','of_ix','of_key'+str(i),8),('ALUI','add','of_ix','of_ix',j),('LDI','of_one',1),('STX','of_ix',SHAPES,'of_one'))
        else:bindings['key'+w]='of_key'+str(i)
    for line in (root/'objectfacts-fresh.tsv').read_text().splitlines()[1:]:
        name,kind,prefix=line.split('\t');bindings[name]=E.P(prefix).fresh(kind)
    install_rules(g,root,'objectfacts',bindings,{'start':start.acts},None)
