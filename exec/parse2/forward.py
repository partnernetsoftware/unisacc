"""Opt-in E3 sidecar for reachable unresolved calls in macOS -run.

The parser owns the prototype facts.  The host driver receives only framed
names and the existing USLSIG3 descriptor; it never guesses a C signature
from a call instruction.  Without the resource, this hook is inert.
"""

RECORDS = 770 << 40
SEEN = 771 << 40


def install(E, P, start, fps_fn, definitions):
    # Stage control lives in forward-result.tsv (sections s1 start, s2 UD.error hook,
    # s3 LX.accept envelope); fresh labels are declared in forward-fresh.tsv.
    # Notes kept from the handwritten form: the C reference does not forward an
    # unprototyped call (FW.missing keeps UD.error's diagnostic); UD scans a saved
    # copy of the emitted tape so the empty output holds one record until OCUT;
    # LX.accept is the final normal acceptance after library envelope routing.
    # Python binds only dynamic facts (start label, FPS_FN bank, record banks) and
    # keeps the residue: renaming UD.error / LX.accept and moving their definitions.
    from pathlib import Path
    from finite_rules import install as rules, install_template
    g = E.g
    root = Path(__file__).parent
    bindings = {"RECORDS": RECORDS, "SEEN": SEEN, "fps_fn": fps_fn, "start": start}

    class _Scope:
        def __init__(self, cur): self.cur = cur
    fresh = [l.split("\t") for l in (root / "forward-fresh.tsv").read_text().splitlines()[1:]]

    def section(name):
        for part, key, kind, prefix in fresh:
            if part == name: bindings[key] = E.P.fresh(_Scope(prefix), kind)
        rules(g, root, "forward", bindings, None, None, name)

    def rename(state):
        # The move itself is declared in forward-template.tsv (one section per hook).
        original = "FW.original." + state
        install_template(g, root, "forward", {}, None, section=state)
        for how in ("P", "L"):
            if (state, how) in definitions:
                definitions[original, how] = definitions.pop((state, how))

    section("s1")
    rename("UD.error")
    section("s2")
    rename("LX.accept")
    section("s3")
    return "FW.start"
