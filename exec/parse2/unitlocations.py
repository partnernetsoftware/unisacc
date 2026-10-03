"""Located extension of the existing unit-framing/name-isolation model.
Frame payload = LE32 filename length, filename bytes, UNITOK1 envelope.
Output = UNITOK2 map directory plus positioned, statically renamed tokens.
No declaration logic is duplicated here; units.py remains the scanner.
"""
import json
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules, install_template

MAPS=49<<40


def rows(name):
    return [line.split("\t") for line in Path(__file__).with_name("unitlocations-" + name + ".tsv").read_text().splitlines()[1:]]


def install(E,P):
    import assemble
    assemble.run(Path(__file__).parent / 'tokenlocations-manifest.tsv', E, P, dict(multi=False, record=False, ordinal=False),
                 dict(ready='LS.ready', token_record='RET', ordinal_table=0))   # K2: tokenlocations
    install_template(E.g, Path(__file__).parent, "unitlocations", {}, None)
    bindings = dict(MAPS=MAPS)
    for prefix, kind, key in rows("fresh"):
        bindings[key] = P(prefix + ".unitlocations_" + key).fresh(kind)
    sequences = {name:E.O(json.loads(text)) for name,text in rows("text")}
    def word(register):
        return load_rules(Path(__file__).with_name("unitlocations-actions.tsv"), {}, domain=[0],
                          bindings=dict(word_register=register), section="word")["word"][0][1]
    for kind, register in rows("bindings"):
        sequences[kind + "_" + register] = word(register) if kind == "word" else load_rules(
            Path(__file__).with_name("unitlocations-actions.tsv"),
            dict(unit_word=word("unit"), offset_word=word(register)), domain=[0],
            section="prefix")["prefix"][0][1]
    install_rules(E.g, Path(__file__).parent, "unitlocations", bindings=bindings, sequences=sequences)
    # named results: the accepting state of the located unit loop and its actions before ACCEPT (this row)
    return dict(acc_state="LS.tokens", acc_acts=load_rules(Path(__file__).with_name("unitlocations-result.tsv"),
                sequences, bindings=bindings)["LS.tokens"][0][1][:-1])
