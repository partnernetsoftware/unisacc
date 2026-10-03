"""Two passes over the source: measure actual instruction lengths, then resolve.
No relaxation on ARM64. Labels store byte offset+1, keeping zero as undefined.
Relocation field widths/shifts are read from emit_arm.RELFIELD.
"""
from exec.facts.load import facts
LABELS = next(r['value'] for r in facts('enc-armbranch-bindings') if r['name'] == 'LABELS')


def install(E, word, image=False):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Sections (and image/raw choice) and fresh-identity order come from facts.
    mode = 'image' if image else 'raw'
    labels = facts('enc-armbranch-labels')
    sections = [(r['section'], [(l['key'], l['owner'], l['kind']) for l in labels if l['section'] == r['section']])
                for r in facts('enc-armbranch-sections') if r['mode'] in ('any', mode)]
    bindings = {r['name']: r['value'] for r in facts('enc-armbranch-bindings')}
    sequences = {'word': word(E.P('word.binding')).acts}
    for section, labels in sections:
        bindings.update({key: E.P(owner).fresh(kind) for key, owner, kind in labels})
        install_rules(E.g, Path(__file__).parent, 'armbranch',
                      bindings=bindings, sequences=sequences, section=section)
