"""Windows ARM64 gate rules live in armwin-*.tsv; imports/templates stay bound.
No reference encoder is called. Address math uses the shared ADRP transitions.
"""
from unisa.image.pe import IMPORTS
from exec.facts.load import facts
BINDINGS = {r['name']: r['value'] for r in facts('enc-armwin-bindings')}
IMP = BINDINGS['IMP']


def init(p):
    for key in facts('enc-armwin-keys'):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
    for i,name in enumerate(IMPORTS):
        p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))


def reset(p):
    for r in facts('enc-armwin-reset'):
        p.a(('LDI','wm_'+r,0))
    return p


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    from unisa.emit_arm import WINARGS_BODY
    bindings = dict(BINDINGS)
    for line in Path(__file__).with_name("armwin-names.tsv").read_text().splitlines():
        if not line.startswith("#"):
            name, prefix, kind = line.split("\t")
            bindings[name] = E.P(prefix).fresh(kind)
    # WINARGS_BODY is the shared declared template; its algorithm is not migrated here.
    template = E.P("winargs.binding")
    for value in WINARGS_BODY:
        word(template.a(("LDI", "w", value)))
    install_rules(E.g, Path(__file__).parent, "armwin", bindings=bindings,
                  sequences={"word": word(E.P("word.binding")).acts, "winargs": template.acts})
