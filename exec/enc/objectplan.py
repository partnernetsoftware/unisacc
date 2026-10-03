"""Object header and ELF symbol-plan delta. All runtime work is generic actions.
Stage control lives in objectplan-byte.tsv / objectplan-result.tsv (finite_rules),
one section run per configuration (d0: offset-table labels, d1: direct labels).
objectplan-steps.tsv orders the steps per configuration: fresh labels (allocated in
recorded order and bound as $name), state renames, and rule sections. Python binds only
dynamic facts: table bases, the passed-in OFF/LABD/SYM/PRESENT addresses, and the tape
header sequence (arch). Residue: the rename of wrapped states into OBJ.original.* --
it creates states, which a table row cannot do.
"""
import pathlib
from finite_rules import install as install_rules
OBJ_RELOCS = 250 << 40
OBJ_NREL = 'obj_nrel'
NAMES, IDS, SEEN, FLAGS, KIND, POS, STRINGS, SYMBOLS, TEXT, TAPE = (i << 40 for i in range(251,261))

def install(E, byte, OFF, LABD, SYM, PRESENT, arch='x86_64', direct_labels=False):
    from elfimage import DATA
    from elfobject import install as writer
    g=E.g;root=pathlib.Path(__file__).parent
    cfg='d1' if direct_labels else 'd0'
    bindings=dict(OBJ_RELOCS=OBJ_RELOCS,NAMES=NAMES,IDS=IDS,SEEN=SEEN,FLAGS=FLAGS,KIND=KIND,POS=POS,
                  STRINGS=STRINGS,SYMBOLS=SYMBOLS,TEXT=TEXT,TAPE=TAPE,STRINGSHI=STRINGS+(1<<39),
                  OFF=OFF,LABD=LABD,SYM=SYM,PRESENT=PRESENT)
    sequences={'header':E.O('UNISATAPE1 lnx/'+arch+' ')}
    for line in (root/'objectplan-steps.tsv').read_text().splitlines():
        if line.startswith('#'):continue
        f=line.split('\t')
        if f[0]!=cfg:continue
        if f[1]=='fresh':bindings[f[2]]=E.P(f[4]).fresh(f[3])
        elif f[1]=='move':
            n='OBJ.original.'+f[2];assert n not in g.st
            g.st[n]=g.st.pop(f[2]);g.labels.add(n)
        else:
            assert f[1]=='rules',line
            install_rules(g,root,'objectplan',bindings,sequences,section=f[2])
    writer(E,dict(text=TEXT,data=DATA,strings=STRINGS,tape=TAPE,symbols=SYMBOLS,relocs=OBJ_RELOCS),arch)
