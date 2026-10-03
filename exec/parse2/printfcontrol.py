"""Bind shared output fragments, token facts and stacks to printf controls."""
import json
import re
from pathlib import Path
from finite_rules import install as install_rules, install_rows, install_template, load as load_rules


def install(E, P, section, warnings, templates, facts, alphabet):
    root = Path(__file__).parent
    def rows(suffix):
        return [line.split('\t') for line in (root / ('printfcontrol-' + suffix + '.tsv')).read_text().splitlines()[1:]]
    bindings = dict(facts, FND=E.FND, VAR=E.VAR, PFSLOTS=32 << 40, TKEOF=E.TK['eof'])
    p = P('printfcontrol.bindings.' + section)
    for part, mode, owner, kind, key in rows('fresh'):
        if part == section and (mode == 'all' or warnings):
            p.cur = owner
            bindings[key] = p.fresh(kind)
    sequences = {name: E.O(json.loads(value)) for name, value in rows('text')}
    for name, value in rows('template'):
        template, index = json.loads(value)
        sequences[name] = E.O(re.split(r'(\{[^}]*\})', templates[template])[index])
    for name, value in rows('stack'):
        method, slots = json.loads(value)
        p.acts = []
        sequences[name] = getattr(p, method)(*slots).acts
    tokens = dict(E.TK, string=E.TK_STR, identifier=E.TK_ID)
    classes = {name: [tokens[token]] for name, token in rows('tokens')}
    # One declared byte classifier supplies all three pool spelling consumers.
    spellings = load_rules(root / 'printfcontrol-spelling.tsv', {}, domain=range(256), section='escape')['escape']
    for part, state, target, mode, digits in rows('escape'):
        if part != section:
            continue
        letters = alphabet if digits == "HEX" else digits
        spelled = dict(sequences, **{'advance.b': [('ADV',)], 'advance.r': []})
        for byte, (kind, _) in spellings.items():
            spelled['spell.%s.%d' % (letters, byte)] = E.O(
                {'raw': chr(byte), 'quoted': '\\' + chr(byte),
                 'hex': '\\x' + letters[byte >> 4] + letters[byte & 15]}[kind])
        install_template(E.g, root, 'printfcontrol-escape',
                         {'esc': [dict(state=state, target=target, letters=letters, mode=mode)],
                          'byte': [dict(b=byte) for byte in spellings]},
                         None, sequences=spelled, section='escape', mode=mode, domain=range(256))
    if section == "part1":
        install_rows(E.g, root / "printfcontrol-spelling.tsv", domain=range(256), section="append", mode="r")
    sections = {line.split('\t')[0] for line in (root / 'printfcontrol-result.tsv').read_text().splitlines() if line and not line.startswith('#')}
    for mode in ('all', 'warnings' if warnings else 'plain'):
        if section + '.' + mode not in sections:
            continue
        install_rules(E.g, root, 'printfcontrol', bindings=bindings, sequences=sequences,
                      classes=classes, section=section + '.' + mode)
