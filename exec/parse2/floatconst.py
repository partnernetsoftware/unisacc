"""Decimal and hexadecimal literal conversion in delta actions.

32-bit limbs keep M and powers of ten exact. Normalise their ratio, take
only the destination's significant bits, and round once (nearest/even),
including subnormals. The product's 160-limb bound is checked, never wrapped.
Integer and character token paths remain in the existing reader.
"""


import json
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules

A, D = 30 << 40, 31 << 40


def rows(name):
    return [line.split("\t") for line in Path(__file__).with_name("floatconst-" + name + ".tsv").read_text().splitlines()[1:]]


def install(E, P):
    g = E.g
    # Preserve the original numeric-reader replacement boundary.
    from finite_rules import install_template
    install_template(g, Path(__file__).parent, "floatconst", {}, None, section="boundary")
    sequences = {name: E.rej(message) for name, message in rows("reject")}
    def rules(section, tag="", base=0, size=""):
        bindings = dict(A=A, D=D, TK_FNUM=E.TK_FNUM, pool_base=base, pool_size=size)
        bindings.update((key, stem + tag) for part, key, stem in rows("pools") if part == section)
        for part, prefix, kind, key in rows("fresh"):
            if part == section:
                bindings[key] = P(prefix + ".float_" + section + tag + key).fresh(kind)
        install_rules(g, Path(__file__).parent, "floatconst", section=section,
                      bindings=bindings, sequences=sequences)
    # Register rejection identity before scanner defaults, preserving diagnostic order.
    rules("entry")
    # A single byte/value relation supplies hex and both decimal scanners.
    # Facts: each scanner context and the digits of its radix with their spellings.
    digits = [{"value": int(value), "bytes": [int(byte) for byte in encoded.split(",")]} for value, encoded in rows("digits")]
    for state, target, radix, extra in rows("digit-contexts"):
        ctx = {"state": state, "target": target, "digits": [d for d in digits if d["value"] < int(radix)],
               "extra": "".join("," + json.dumps(x) for x in json.loads(extra))}
        install_template(g, Path(__file__).parent, "floatconst", {"ctx": [ctx]}, None, section="digit", mode="b",
                         domain=[b for d in ctx["digits"] for b in d["bytes"]])

    rules("main0")
    for tag, base, size in (("A", A, "df_n"), ("D", D, "df_dn")):
        rules("limb", tag, base, size)
    rules("main2")
    for tag in ("A", "D"):
        rules("power", tag)
    rules("main4")
    for tag in ("A", "D"):
        rules("shift", tag)
    rules("main6")
