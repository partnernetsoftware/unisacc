"""Bind integer facts and shared tape words to the bit-field control table.

Member metadata is the single width/offset/sign/unit description used by all
read, update, and initializer paths. bf_value separately tracks a value that
is still a bit-field expression for sizeof; it is not a modifiable proof.
"""
import json
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, tyinfo, tape, integers, bindings):
    root = Path(__file__).parent
    bindings = dict(bindings, BOOLUNIT=tyinfo["u8"][0], ENUM_UNSIGNED=next(code for name, code, *_ in integers if name == "u32"))
    texts = {name: E.O(json.loads(value)) for name, value in
             (line.split("\t") for line in (root / "bitfields-text.tsv").read_text().splitlines()[1:])}
    texts["reject"] = E.rej("not covered: bit-field storage unit")
    classes = {name: [E.TK[token]] for name, token in (("char", "type=char"), ("short", "type=short"), ("long", "type=long"), ("int", "type=int"), ("comma", ","), ("close", "}"))}
    install_rules(E.g, root, "bitfields", sequences=texts, bindings=bindings, classes=classes, section="core")
    for op in ("+", "-"):
        install_rules(E.g, root, "bitfields", bindings=dict(bindings, post="BF.post"+op, change="BF.change"+op, store="BF.store"+op, old="BF.old"+op), sequences=dict(texts, push=E.O(E.PUSH), step=E.O("  imm r1, 1\n"+E.optext(op).replace("r1, r0", "r0, r1"))), section="post")
    # Unit sizes come from the existing integer type facts, never test layouts.
    sizes = sorted({size for name, (size, _, _) in tyinfo.items() if name.startswith(("i", "u"))})
    for procedure, kind in (("BF.LOAD0", "load0"), ("BF.LOAD4", "load4"), ("BF.STORE4", "store4")):
        current = procedure
        for index, size in enumerate(sizes):
            state = procedure + "." + str(index)
            unit_bindings = dict(current=current, test=state + ".test", hit=state + ".hit",
                            next=state + ".next", size=size)
            load = tape["load8"] if size == 8 else tape["loadn"] % size
            store = tape["store8"] if size == 8 else tape["storen"] % size
            words = dict(load0=load, load4=load.replace("r0", "r4").replace("[r4+0]", "[r1+0]"),
                         store4=store.replace(", r0", ", r4"))
            install_rules(E.g, root, "bitfields", bindings=unit_bindings,
                          sequences={"unit": E.O(words[kind])}, section="unit")
            current = unit_bindings["next"]
        install_rules(E.g, root, "bitfields", bindings={"current": current},
                      sequences=texts, section="unit-end")

    current = "BF.decltype.rows"
    for index, (_, code, size, unsigned, _) in enumerate(integers):
        key = "BF.type." + str(index)
        facts = dict(bindings, current=current, test=key+".test", hit=key, next=key+".next",
                     code=code, unit=size, signed=1-unsigned)
        install_rules(E.g, root, "bitfields", bindings=facts, section="type")
        current = facts["next"]
    install_rules(E.g, root, "bitfields", bindings=dict(bindings,current=current),
                  sequences=texts, section="unit-end")

