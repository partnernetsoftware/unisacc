"""Bind conditional null proofs and dereference statements to shared controls."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, enum_values, enum_defined):
    root = Path(__file__).parent
    tokens = dict(E.TK, number=E.TK_NUM, identifier=E.TK_ID)
    classes = {name: [tokens[token]] for name, token in (line.split('\t') for line in (root / 'conditional-tokens.tsv').read_text().splitlines()[1:])}
    install_rules(E.g, root, 'conditional', bindings=dict(ENV=enum_values, ENDEF=enum_defined, first="SS.check"+E.CASOPS[0]), classes=classes, section='main')
    for i, op in enumerate(E.CASOPS):
        install_rules(E.g, root, 'conditional', bindings=dict(check='SS.check'+op, test='SS.test'+op, entry='SS.compound'+op, operation='LV.c'+op, next='SS.check'+E.CASOPS[i+1] if i+1<len(E.CASOPS) else 'SS.rv'), classes=dict(operator=[E.TK[op+'=']]), section='compound')
