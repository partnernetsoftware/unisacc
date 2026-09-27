"""Integer/frame transition declarations loaded from armint-*.tsv."""
SPECS={'.div':('rrr',16),'.udiv':('rrr',17),'.mod':('rrr',18),'.umod':('rrr',19),
       'sext':('rri',20),'addi':('rri',21),'subi':('rri',22),'lsli':('rri',23),'.frame':('i',24)}


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Allocate legacy branch/return identities in the same order; rules live in TSV.
    labels = (('EMIT', 'b'), ('DIV', 'b'), ('EMIT', 'b'), ('DIV', 'b'),
              ('EMIT', 'b'), ('SEXT', 'b'), ('EMIT', 'b'), ('EMIT', 'b'),
              ('EMIT', 'b'), ('EMIT', 'b'), ('FRAME', 'b'), ('FRAME', 'r'))
    bindings = {'label'+str(i): E.P(owner).fresh(kind)
                for i, (owner, kind) in enumerate(labels)}
    install_rules(E.g, Path(__file__).parent, 'armint',
                  bindings=bindings, section='integer')
