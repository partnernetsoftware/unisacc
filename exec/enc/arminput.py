"""ARM input/setup rules live in arminput-*.tsv; META initialization stays dynamic."""
from tins import META
KEYS=75000000
SEEN=76000000


def init(p):
    for key in ('imm','reg','mem','addr','setreg','gate','jump','jumpz','call','true','false','svc0','svc80','svc','arm','winapi','arm19','arm26'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','id_'+key))
    for n,key in enumerate(META,1):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','t'),('LDI','u',n),('STX','t',KEYS,'u'))


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    bindings = {"KEYS": KEYS, "SEEN": SEEN}
    for line in Path(__file__).with_name("arminput-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            name, prefix, kind = line.split("\t")
            bindings[name] = E.P(prefix).fresh(kind)
    install_rules(E.g, Path(__file__).parent, "arminput", bindings=bindings,
                  classes={"meta_"+key: [index] for index, key in enumerate(META, 1)},
                  sequences={"word": word(E.P("word.binding")).acts})
