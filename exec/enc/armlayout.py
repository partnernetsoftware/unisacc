"""ARM text/data address resolution. Header values are declarations, not answers.
Image format constants are read at generation; layout/ADRP arithmetic lives in
armlayout-*.tsv. This outputs text; retained data headers are not an image.
"""
from exec.facts.load import facts
BINDINGS = {r['name']: r['value'] for r in facts('enc-armlayout-bindings')}
SYM, PRESENT, HSEEN = BINDINGS['SYM'], BINDINGS['PRESENT'], BINDINGS['HSEEN']

def init(p):
    p.a(('LDI','target_os',1),('SBCLR',),[('SBOUT',c) for c in b'_start'],('SBINTERN','id_entry'))
    for key in facts('enc-armlayout-headers'):
        p.a(('SBCLR',),[('SBOUT',c) for c in ('@'+key).encode()],('SBINTERN','h_'+key))
    for key,value in ((r['key'],r['value']) for r in facts('enc-armlayout-targets')):
        p.a(('SBCLR',),[('SBOUT',c) for c in value.encode()],('SBINTERN','target_'+key))



def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    import assemble
    bindings = dict(BINDINGS)
    sequences = {"word": word(E.P("word.binding")).acts}
    names = Path(__file__).with_name("armlayout-names.tsv").read_text().splitlines()
    for section in ("headers", "address"):
        if section == "address":
            assemble.run(Path(__file__).with_name("memorylayout-manifest.tsv"), E, E.P, {}, {"fail": "FAIL", "code_size": "length"})
        for line in names:
            if not line.startswith("#"):
                selected, name, prefix, kind = line.split("\t")
                if selected == section:
                    bindings[name] = E.P(prefix).fresh(kind)
        install_rules(E.g, Path(__file__).parent, "armlayout", bindings=bindings,
                      sequences=sequences, section=section)
