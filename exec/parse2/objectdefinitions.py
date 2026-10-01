"""Reject a second initialised file-scope definition in one translation unit."""

INITIALISED = 760 << 40


def install(E, P):
    g = E.g
    old = "OD.original.GV.record"
    g.st[old] = g.st.pop("GV.record")
    g.labels.add(old)
    P("OD.entry").branch(
        {1: "OD.lookup"}, old, [("CMPI", "tk", E.TK["="])])
    g.st["GV.record"] = g.st["OD.entry"]
    P("OD.lookup").a(("INTERN", "od_id", "fns", "fne"),
                     ("LDX", "od_seen", "od_id", INITIALISED)).branch(
                         {1: "OD.first"}, "OD.fail", [("CMPI", "od_seen", 0)])
    P("OD.first").a(("LDI", "od_one", 1),
                    ("STX", "od_id", INITIALISED, "od_one")).goto(old)
    P("OD.fail").a(E.rej(
        "redefinition of this object (it already has an initialiser)")).goto("DEAD")
