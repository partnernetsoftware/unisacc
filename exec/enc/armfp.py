"""ARM FP bit-pattern operations, with no FP primitive in the executor.
Opcode facts (derived from unisa/emit_arm.py) are in exec/facts/enc-armfp-*.tsv;
movement and packing transitions are in armfp-*.tsv.
"""
from exec.facts.load import facts
SPECS = {r['op']: (r['shape'], r['cls']) for r in facts('enc-armfp-specs')}


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = {r['name']: r['value'] for r in facts('enc-armfp-bindings')}
    sequences = {'word': word(E.P('word.binding')).acts}
    for op, (_, cls) in SPECS.items():
        install_rules(E.g, Path(__file__).parent, 'armfp',
                      bindings={**bindings, 'emit': 'EMIT.%d' % cls},
                      sequences=sequences, section=op)
