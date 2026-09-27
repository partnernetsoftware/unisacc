"""Bind function-signature facts to the existing parser and conversion controls."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, bindings, integers):
    root = Path(__file__).parent
    contexts = [line.split('\t') for line in (root / 'functiontypes-context.tsv').read_text().splitlines()[1:]]
    sequences = dict(reject=E.rej('not covered: function signature pool exhausted'))
    p = P('functiontypes.bindings')
    for group, slots in contexts:
        for name, method in (('save', 'vpush'), ('restore', 'vpop')):
            p.acts = []
            sequences[name if group == 'params' else group + '_' + name] = getattr(p, method)(*slots.split(',')).acts
    install_rules(E.g, root, 'functiontypes', bindings=bindings, sequences=sequences, classes=dict(narrow=[code for _,code,size,_,_ in integers if size < 8]), section='main')
