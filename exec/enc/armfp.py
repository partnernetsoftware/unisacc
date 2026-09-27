"""ARM FP bit-pattern operations, with no FP primitive in the executor.
FARITH/FCMP_INV/FP_OPS remain external declarations; movement and packing
transitions are in armfp-*.tsv. Runtime emits bytes using integer ops.
"""
from unisa.emit_arm import FARITH, FCMP_INV, FP_OPS
SPECS={op:('rrr' if op[:4] in FARITH or op[:3] in FCMP_INV else 'rr',100+i) for i,op in enumerate(FP_OPS)}


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Bind current opcode facts; fixed instruction control and packing live in TSV.
    bindings = {op+width+'_word': value | ty | (17 << 16) | (16 << 5) | 16
                for op, value in FARITH.items() for width, ty in (('32', 0), ('64', 0x00400000))}
    bindings.update({op+'_word': 0x9A9F07E0 | (value << 12) for op, value in FCMP_INV.items()})
    sequences = {'word': word(E.P('word.binding')).acts}
    for op, (_, cls) in SPECS.items():
        install_rules(E.g, Path(__file__).parent, 'armfp',
                      bindings={**bindings, 'emit': 'EMIT.%d' % cls},
                      sequences=sequences, section=op)
