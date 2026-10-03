"""Bind function-signature facts to the existing parser and conversion controls."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P, bindings, integers):
    from libraryexports import PARAMDEPTH, PARAMBASE, PARAMSHAPE, PARAMMARK, SIGEPOCH, VARIADIC, RETURNRANK, PARAMRANK, PREVRETURN, RETURNPENDING
    bindings = dict(bindings, PARAMDEPTH=PARAMDEPTH, PARAMBASE=PARAMBASE, PARAMSHAPE=PARAMSHAPE, VARIADIC=VARIADIC, RETURNRANK=RETURNRANK, PARAMRANK=PARAMRANK)
    root = Path(__file__).parent
    contexts = [line.split('\t') for line in (root / 'functiontypes-context.tsv').read_text().splitlines()[1:]]
    sequences = dict(reject=E.rej('not covered: function signature pool exhausted'))
    p = P('functiontypes.bindings')
    for group, slots in contexts:
        for name, method in (('save', 'vpush'), ('restore', 'vpop')):
            p.acts = []
            sequences[name if group == 'params' else group + '_' + name] = getattr(p, method)(*slots.split(',')).acts
    install_rules(E.g, root, 'functiontypes', bindings=bindings, sequences=sequences, classes=dict(narrow=[code for _,code,size,_,_ in integers if size < 8]), section='main')

    # Stage control after the main/compound rules lives in functiontypes-result.tsv
    # sections s1..s4 (fresh labels declared in functiontypes-fresh.tsv, allocated in
    # the original order on an unregistered scope). Residue in Python: the removal of
    # the expanded FS.query row and rankhook, which renames existing states and aliases
    # each hooked state to its FS.rank.hook entry (fact-dependent states).
    bindings.update(PARAMMARK=PARAMMARK, SIGEPOCH=SIGEPOCH, PREVRETURN=PREVRETURN, RETURNPENDING=RETURNPENDING)
    class _Scope:
        def __init__(self, cur): self.cur = cur
    fresh = [line.split('\t') for line in (root / 'functiontypes-fresh.tsv').read_text().splitlines()[1:]]
    def section(name):
        for part, key, kind, prefix in fresh:
            if part == name: bindings[key] = E.P.fresh(_Scope(prefix), kind)
        install_rules(E.g, root, 'functiontypes', bindings=bindings, sequences=sequences, section=name)
    E.g.st.pop('FS.query')
    section('s1')
    def rankhook(state, part):
        old='FS.rank.original.'+state;E.g.st[old]=E.g.st.pop(state);E.g.labels.add(old)
        alias='FS.rank.hook.'+state;section(part+'h');E.g.st[state]=E.g.st[alias]
        section(part)
    rankhook('FS.eq.field2.test', 's2')
    rankhook('FS.eq.param.base', 's3')
    rankhook('FS.decl.test', 's4')
