"""Name the unsupported source-call aggregate return combination."""


def install(E, P):
    g = E.g
    old = "SR.original"
    entry = "LI.original.structreturn"
    g.st[old] = g.st.pop(entry)
    g.labels.add(old)
    P("SR.entry").a(("INTERN", "sr_id", "ps", "pe"),
                  ("LDX", "sr_local", "sr_id", E.LOC)).branch(
                      {1: "SR.fail"}, old, [("CMPI", "sr_local", 0)])
    g.st[entry] = g.st["SR.entry"]
    P("SR.fail").a(E.rej(
        "not covered: struct return expression outside local lvalue")).goto("DEAD")
