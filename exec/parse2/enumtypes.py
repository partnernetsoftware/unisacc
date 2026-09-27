"""Bind persistent enum descriptors and shared type-query adapters."""
import json
from pathlib import Path
from finite_rules import install as install_rules

def install(E, P, bindings, parameter_types):
    root = Path(__file__).parent
    sequences = {name: E.rej("not covered: " + reason) for name, reason in
                 (("incomplete", "incomplete enum type"), ("capacity", "enum descriptor pool exhausted"),
                  ("redefinition", "enum tag redefinition"), ("kind", "conflicting tag kind"),
                  ("parameter", "parameter"), ("enum", "enum"))}
    install_rules(E.g, root, "enumtypes", bindings=bindings, sequences=sequences,
                  classes=dict(semicolon=[E.TK[";"]], types=parameter_types, identifier=[E.TK_ID],
                               tagged=[E.TK[k] for k in ("struct", "union", "enum")],
                               void=[E.TK["type=void"]], close=[E.TK[")"]],
                               dots=[E.TK["..."]], open=[E.TK["{"]]), section="core")
    p = P("enumtypes.adapter")
    for state, base, depth, preserve, facts in (line.split("\t") for line in
            (root / "enumtypes-hooks.tsv").read_text().splitlines()[1:]):
        b = dict(bindings, entry=state, ready=state+".enumready", done=state+".enumdone",
                 raw=state+".enumraw", base=base, depth=depth)
        p.acts = []
        save = p.vpush(base).acts if preserve == "yes" else []
        p.acts = []
        restore = p.vpop(base).acts if preserve == "yes" else []
        install_rules(E.g, root, "enumtypes", bindings=b,
                      sequences=dict(save=save, restore=restore, facts=json.loads(facts)), section="hook")
