"""Windows ARM64 gate rules live in armwin-*.tsv; imports/templates stay bound.
No reference encoder is called. Address math uses the shared ADRP transitions.
"""
from unisa.image.pe import IMPORTS
OPS=('exit','read','write','mmap','mprotect','munmap','close','open','lseek','unlink','rename')
RCS=('none','wcount','bool_inv','bool_neg','dword_sx')
IMP=80000000


def init(p):
    for key in OPS+RCS+("'none",):
        p.a(('SBCLR',),[('SBOUT',c) for c in key.encode()],('SBINTERN','wi_'+key))
    for i,name in enumerate(IMPORTS):
        p.a(('SBCLR',),[('SBOUT',c) for c in name.encode()],('SBINTERN','t'),('LDI','u',i+1),('STX','t',IMP,'u'))


def reset(p):
    for r in ('catop','winimp','retconv','hstd','written','scr0','scr1'):
        p.a(('LDI','wm_'+r,0))
    return p


def install(E,word):
    from pathlib import Path
    from finite_rules import install as install_rules
    from unisa.emit_arm import WINARGS_BODY
    bindings = {"IMP": IMP}
    bindings.update(("import_"+name, IMPORTS.index(name))
                    for name in ("FlushInstructionCache", "GetCommandLineA"))
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
