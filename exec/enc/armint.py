"""Integer/frame transition declarations loaded from armint-*.tsv; facts in exec/facts/enc-armint-*.tsv."""
from exec.facts.load import facts
SPECS = {r['op']: (r['shape'], r['cls']) for r in facts('enc-armint-specs')}


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Allocate legacy branch/return identities in the same order; rules live in TSV.
    bindings = {'label'+str(i): E.P(r['owner']).fresh(r['kind'])
                for i, r in enumerate(facts('enc-armint-labels'))}
    install_rules(E.g, Path(__file__).parent, 'armint',
                  bindings=bindings, sequences={'word': word(E.P('word.binding')).acts}, section='integer')
