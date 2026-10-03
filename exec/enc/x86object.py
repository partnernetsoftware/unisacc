"""ELF object address/undefined-call relocation capture, as delta actions.
Stage control lives in x86object-result.tsv (finite_rules), sections s1-s4; fresh labels are
pre-allocated in recorded order from x86object-fresh.tsv on an unregistered scope.
Python binds only dynamic facts: RELOCS and the caller's LABD/TGT bank bases.
Residue: move() renames LAYOUT, AD.label, RIP, WR.c to XO.original.* between the sections --
it creates states, which a table row cannot do.
"""
from pathlib import Path
from finite_rules import install as install_rules
RELOCS = 250 << 40

def install(E, byte, OFF, LABD, KND, TGT, SYM, PRESENT):
    g=E.g;root=Path(__file__).parent
    fresh=[l.split('\t') for l in (root/'x86object-fresh.tsv').read_text().splitlines()[1:]]
    bindings=dict(RELOCS=RELOCS,LABD=LABD,TGT=TGT)
    def move(state):
        other='XO.original.'+state;assert other not in g.st
        g.st[other]=g.st.pop(state);g.labels.add(other);return other
    def section(name):
        for part,key,kind,prefix in fresh:
            if part==name:bindings[key]=E.P.fresh(type('FreshScope',(),{'cur':prefix})(),kind)
        install_rules(g,root,'x86object',bindings,None,None,name)
    # Object addresses are section offsets, never executable-image virtual VAs.
    for state,name in [('LAYOUT','s1'),('AD.label','s2'),('RIP','s3'),('WR.c','s4')]:
        move(state);section(name)
