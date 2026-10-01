"""Name the unsupported source-call aggregate return combination."""


def install(E, P, errors=False):
    g = E.g
    old = "SR.original"
    entry = "LI.original.structreturn"
    g.st[old] = g.st.pop(entry)
    g.labels.add(old)
    P("SR.entry").a(("INTERN", "sr_id", "ps", "pe"),
                  ("LDX", "sr_local", "sr_id", E.LOC)).branch(
                      {1: "SR.fail"}, old, [("CMPI", "sr_local", 0)])
    g.st[entry] = g.st["SR.entry"]
    reason="not covered: struct return expression outside local lvalue"
    if errors:
        P("SR.fail").a(('LDI','uc_kind',1),('COPYW','er_at','tpos'),('LDI','er_recover',0),
                       ('SBCLR',),*(('SBOUT',c) for c in reason.encode()),
                       ('SBSAVE','diag_message')).goto('ER.frames')
    else:
        P("SR.fail").a(E.rej(reason)).goto("DEAD")
