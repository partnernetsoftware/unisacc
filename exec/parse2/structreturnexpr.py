"""Name the unsupported source-call aggregate return combination."""


def install(E, P, errors=False):
    # Control lives in structreturnexpr-template.tsv; facts are the hooked entry,
    # the local-slot bank and the diagnostic text.
    from pathlib import Path
    from finite_rules import install_template
    g = E.g
    root = Path(__file__).parent
    reason = "not covered: struct return expression outside local lvalue"
    facts = {"sr": [{"entry": "LI.original.structreturn", "loc": E.LOC, "reason": reason}]}

    class _Scope:
        cur = "SR"
    install_template(g, root, "structreturnexpr", facts, None, section="move")
    install_template(g, root, "structreturnexpr", facts, lambda h: E.P.fresh(_Scope(), h), section="entry")
    install_template(g, root, "structreturnexpr", facts, None,
                     sequences={"reason": [("SBOUT", c) for c in reason.encode()]},
                     section="fail.errors" if errors else "fail")
