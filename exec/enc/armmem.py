"""ARM memory/zero-fill transitions declared in TSV; opcode facts in exec/facts/enc-armmem-*.tsv."""
from exec.facts.load import facts


def install(E, word):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Preserve existing generated branch/return identities; TSV owns the control.
    bindings = {'label'+str(i): E.P(r['owner']).fresh(r['kind'])
                for i, r in enumerate(facts('enc-armmem-labels'))}
    bindings.update({r['name']: r['value'] for r in facts('enc-armmem-bindings')})
    install_rules(E.g, Path(__file__).parent, 'armmem', bindings=bindings,
                  sequences={'word': word(E.P('word.binding')).acts}, section='memory')
