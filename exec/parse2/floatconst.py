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
    g.st['SPANNUM.integer'] = g.st.pop('SPANNUM')
    del g.st['NUMF']
    del g.st['NUMFL']
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
    for state, target, radix, extra in rows("digit-contexts"):
        for value, encoded in rows("digits"):
            if int(value) >= int(radix):
                continue
            digits = [int(byte) for byte in encoded.split(",")]
            for name, row in load_rules(Path(__file__).with_name("floatconst-byte.tsv"),
                    {"digit_extra": json.loads(extra)}, domain=digits, classes={"digit": digits},
                    bindings=dict(digit_state=state, digit_target=target, digit=int(value)), section="digit").items():
                for key, (target_state, actions) in row.items():
                    g.on(name, [key], target_state, actions)

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
