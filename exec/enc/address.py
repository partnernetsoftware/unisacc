"""Deferred x86 RIP-relative encoding; layout runs after branch relaxation.

No reference layout or encoder is called at runtime. Format constants are read
at generation; page rounding, symbol resolution and displacements are delta
operations. This slice outputs text only; header data is not an image yet.
"""
from exec.facts.load import facts
BINDINGS = {r['name']: r['value'] for r in facts('enc-address-bindings')}
SYM, PRESENT, DEST, VALUE, NAMED, OPCODE, VALUE2 = (BINDINGS[k] for k in ('SYM', 'PRESENT', 'DEST', 'VALUE', 'NAMED', 'OPCODE', 'VALUE2'))

def install(E, byte, KND, SZ, OFF, LABD):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    import assemble
    root = Path(__file__).parent
    sequences = {name: [tuple(a) for a in json.loads(actions)] for name, actions in
                 (line.split('\t') for line in (root/'address-sequences.tsv').read_text().splitlines() if not line.startswith('#'))}
    sequences.update({'byte'+value: byte(E.P('byte.binding'), int(value)).acts for value in
                      (root/'address-bytes.tsv').read_text().splitlines() if not value.startswith('#')})
    sequences['reject'] = E.rej('not covered: address or target declaration')
    facts_ = dict(BINDINGS, KND=KND, SZ=SZ, OFF=OFF, LABD=LABD)
    layouts = {}
    for r in facts('enc-address-layouts'):
        layouts.setdefault(r['tag'], {})[r['name']] = r['value']
    for phase in ('pre', 'post'):
        if phase == 'post':
            assemble.run(root/'memorylayout-manifest.tsv', E, E.P, {}, {'fail': 'DEAD.addr', 'code_size': 'endo'})
            install_rules(E.g, root, 'address', section='lnx-dynamic')
        for line in (root/'address-instances.tsv').read_text().splitlines():
            if line.startswith('#'): continue
            selected, section, values, names, prepare = line.split('\t')
            if selected != phase: continue
            bindings = {**facts_, **json.loads(values)}
            if section == 'posix': bindings.update(layouts[bindings['platform']])
            if section == 'windows': bindings.update(layouts['win'])
            bindings.update({name: E.P(owner).fresh(kind) for name, owner, kind in json.loads(names)})
            install_rules(E.g, root, 'address', section=section, bindings=bindings,
                          sequences={**sequences, 'prepare': sequences[prepare]})
