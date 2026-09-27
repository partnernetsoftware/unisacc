"""Local VLA storage and scope restoration, on existing generic actions.
The runtime length is evaluated once. Static frame slots retain its byte
count and address; each allocating scope saves the pre-allocation stack.
"""
import json
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, SIZE, FRAME, DEP, ENUM, bad, UNS):
    bindings = dict(SIZE=SIZE, FRAME=FRAME, DEP=DEP, ENUM=ENUM, UNS8=UNS + 8)
    root = Path(__file__).parent
    def rows(name):
        return [line.split("\t") for line in (root / ("vla-" + name + ".tsv")).read_text().splitlines()[1:]]
    p = P("vla.bindings")
    sequences = {name: E.O(json.loads(text)) for name, text in rows("text")}
    for name, method, slots in rows("stack"):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots.split(",")).acts
    for prefix, kind, key in rows("fresh"):
        p.cur = prefix
        bindings[key] = p.fresh(kind)
    for name, kind, message in rows("reject"):
        if kind == "callback":
            failure = bad(message)
            bindings[name + "_target"] = "DEAD" if isinstance(failure, tuple) else failure
            sequences[name + "_actions"] = E.rej(failure[1]) if isinstance(failure, tuple) else []
        else:
            sequences[name] = E.rej(message)
    classes = {name: [E.TK_ID if token == "identifier" else E.TK[token]] for name, token in rows("tokens")}
    install_rules(E.g, root, "vla", bindings=bindings, sequences=sequences, classes=classes, section="main")
