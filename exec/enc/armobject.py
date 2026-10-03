"""ARM ELF relocation capture via existing generic model actions.
Only the emitting pass records relocations. Sizing has identical word lengths.
Stage control lives in armobject-result.tsv (finite_rules), sections s1-s7; fresh labels are
pre-allocated in recorded order from armobject-fresh.tsv on an unregistered scope.
Python binds only dynamic facts: OBJ_RELOCS, LABELS, DATA_BASE, UNDEF_BASE.
Residue: replace() renames LAYOUT, EMIT.60, EMIT.61, LEA.code, ADRP, BR.undef to
AO.original.* between the sections -- it creates states, which a table row cannot do.
All object targets are Linux: no host runtime cells exist in an object (EMIT.60/61 -> FAIL);
internal calls retain their section-relative BL field; only unresolved calls relocate.
"""
from pathlib import Path
from finite_rules import install as install_rules
from armbranch import LABELS
from objectplan import OBJ_RELOCS
DATA_BASE = 1 << 32
UNDEF_BASE = 1 << 35


def install(E, word):
    g = E.g
    root = Path(__file__).parent
    assert 'SKIPL' not in g.st
    fresh = [l.split('\t') for l in (root/'armobject-fresh.tsv').read_text().splitlines()[1:]]
    bindings = dict(OBJ_RELOCS=OBJ_RELOCS, LABELS=LABELS, DATA_BASE=DATA_BASE, UNDEF_BASE=UNDEF_BASE)
    def replace(name):
        assert name in g.st, name
        old = 'AO.original.' + name
        assert old not in g.st
        g.st[old] = g.st.pop(name)
        g.labels.add(old)
    def section(name):
        for part, key, kind, prefix in fresh:
            if part == name:
                bindings[key] = E.P.fresh(type('FreshScope', (), {'cur': prefix})(), kind)
        install_rules(g, root, 'armobject', bindings, None, None, name)
    section('s1')
    for state, name in [('LAYOUT', 's2'), ('EMIT.60', 's3'), ('EMIT.61', 's4'),
                        ('LEA.code', 's5'), ('ADRP', 's6'), ('BR.undef', 's7')]:
        replace(state)
        section(name)
