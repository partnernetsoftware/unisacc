"""Track external and unit-local definitions across a multi-source tape."""

INITIALISED = 760 << 40
STATIC_INITIALISED = 761 << 40
FUNCTION_DEFINED = 762 << 40
STATIC_FUNCTION_DEFINED = 763 << 40


def install(E, P):
    g = E.g
    old = "OD.original.GV.record"
    g.st[old] = g.st.pop("GV.record")
    g.labels.add(old)
    P("OD.entry").branch(
        {1: "OD.lookup"}, old, [("CMPI", "tk", E.TK["="])])
    g.st["GV.record"] = g.st["OD.entry"]
    P("OD.lookup").a(("INTERN", "od_id", "fns", "fne"),
                     ("ALUI", "add", "od_epoch", "unit_epoch", 1)).branch(
                         {1: "OD.static"}, "OD.external",
                         [("CMPI", "fstat_cur", 1)])
    P("OD.external").a(("LDX", "od_seen", "od_id", INITIALISED)).branch(
        {1: "OD.external.first"}, "OD.external.known",
        [("CMPI", "od_seen", 0)])
    P("OD.external.known").branch(
        {1: "OD.same"}, "OD.cross", [("CMP", "od_seen", "od_epoch")])
    P("OD.external.first").a(
        ("STX", "od_id", INITIALISED, "od_epoch")).goto(old)
    P("OD.static").a(("LDX", "od_seen", "od_id", STATIC_INITIALISED)).branch(
        {1: "OD.same"}, "OD.static.first",
        [("CMP", "od_seen", "od_epoch")])
    P("OD.static.first").a(
        ("STX", "od_id", STATIC_INITIALISED, "od_epoch")).goto(old)
    P("OD.same").a(E.rej(
        "redefinition of this object (it already has an initialiser)")).goto("DEAD")
    P("OD.cross").a(E.rej(
        "multiple definitions of this object across units (each has an initialiser)"
    )).goto("DEAD")

    function = "FD.original.FN.def1"
    g.st[function] = g.st.pop("FN.def1")
    g.labels.add(function)
    P("FD.entry").a(("INTERN", "fd_id", "fns", "fne"),
                  ("ALUI", "add", "fd_epoch", "unit_epoch", 1)).branch(
                      {1: "FD.static"}, "FD.external",
                      [("CMPI", "fstat_cur", 1)])
    g.st["FN.def1"] = g.st["FD.entry"]
    P("FD.external").a(("LDX", "fd_seen", "fd_id", FUNCTION_DEFINED)).branch(
        {1: "FD.external.first"}, "FD.fail", [("CMPI", "fd_seen", 0)])
    P("FD.external.first").a(
        ("STX", "fd_id", FUNCTION_DEFINED, "fd_epoch")).goto(function)
    P("FD.static").a(("LDX", "fd_seen", "fd_id", STATIC_FUNCTION_DEFINED)).branch(
        {1: "FD.fail"}, "FD.static.first", [("CMP", "fd_seen", "fd_epoch")])
    P("FD.static.first").a(
        ("STX", "fd_id", STATIC_FUNCTION_DEFINED, "fd_epoch")).goto(function)
    P("FD.fail").a(E.rej("multiple definitions of this function")).goto("DEAD")
