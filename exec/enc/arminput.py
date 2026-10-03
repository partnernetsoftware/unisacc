"""ARM input/setup rules live in arminput-*.tsv; META initialization stays dynamic."""
from tins import META
from exec.facts.load import facts
CONST = {r['name']: r['value'] for r in facts('enc-arminput-bindings')}
KEYS, SEEN = CONST['KEYS'], CONST['SEEN']


def init(p):
    for key in facts('enc-arminput-keys'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','id_'+key))
    for n,key in enumerate(META,1):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','t'),('LDI','u',n),('STX','t',KEYS,'u'))


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = dict(CONST)
    for line in Path(__file__).with_name("arminput-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            name, prefix, kind = line.split("\t")
            bindings[name] = E.P(prefix).fresh(kind)
    install_rules(E.g, Path(__file__).parent, "arminput", bindings=bindings,
                  classes={"meta_"+key: [index] for index, key in enumerate(META, 1)},
                  sequences={"word": word(E.P("word.binding")).acts})
