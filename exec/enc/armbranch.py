"""Two passes over the source: measure actual instruction lengths, then resolve.
No relaxation on ARM64. Labels store byte offset+1, keeping zero as undefined.
Relocation field widths/shifts are read from emit_arm.RELFIELD.
"""
from unisa.emit_arm import RELFIELD
LABELS=74000000


def install(E, word, image=False):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Sections preserve the existing raw/image choice and generated state identities.
    sections = (('finish', (('FINISH_b0', 'FINISH', 'b'),)),
                ('image', (('ACCEPT_b0', 'ACCEPT', 'b'), ('IMAGE_r0', 'IMAGE', 'r'))) if image else ('raw', ()),
                ('body_pre', (('REWIND_r0', 'REWIND', 'r'),
                              ('LABEL_b0', 'LABEL', 'b'), ('LABEL_b1', 'LABEL', 'b'),
                              ('LABEL_b2', 'LABEL', 'b'), ('LABEL_b3', 'LABEL', 'b'),
                              ('EMIT_r0', 'EMIT', 'r'), ('EMIT_r1', 'EMIT', 'r'),
                              ('EMIT_r2', 'EMIT', 'r'), ('BR_b0', 'BR', 'b'),
                              ('BR_b1', 'BR', 'b'), ('BR_b2', 'BR', 'b'),
                              ('BR_b3', 'BR', 'b'), ('BR_b4', 'BR', 'b'))),
                ('label_end', ()), ('body_post', ()))
    bindings = {'LABELS': LABELS}
    bindings.update({tag+suffix: value for tag, pair in RELFIELD.items()
                     for suffix, value in zip(('_bits', '_shift'), pair)})
    sequences = {'word': word(E.P('word.binding')).acts}
    for section, labels in sections:
        bindings.update({key: E.P(owner).fresh(kind) for key, owner, kind in labels})
        install_rules(E.g, Path(__file__).parent, 'armbranch',
                      bindings=bindings, sequences=sequences, section=section)
