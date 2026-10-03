"""Signed decimal printer encoding. Runtime algorithm stays in emitted ARM code.
ADRP targets are resolved by the delta, using the final section layout. No
encoder oracle or numeric formatting primitive is called by the executor.
Scratch x9-x17 are outside the allocated tape value registers.
"""
from exec.facts.load import facts
_W = facts('enc-armitoa-words')
PREFIX, COUNT_STORE, SUFFIX = ([r['value'] for r in _W if r['part'] == p] for p in ('prefix', 'count', 'suffix'))

def install(E,word):
    from pathlib import Path
    from functools import partial
    from finite_rules import install as install_rules
    rules = partial(install_rules, E.g, Path(__file__).parent, 'armitoa')
    entry = 'EMIT.37'
    for arg, values in (('a0',()), ('a2',PREFIX), ('a1',COUNT_STORE)):
        prefix = E.P('itoa.template')
        for value in values: word(prefix.a(('LDI','w',value)))
        first, nxt = E.P('EMIT').fresh('r'), E.P('EMIT').fresh('r')
        rules(section='address', bindings=dict(entry=entry, first=first, next=nxt, arg=arg),
              sequences={'prefix': prefix.acts})
        entry = nxt
    suffix = E.P('itoa.template')
    for value in SUFFIX: word(suffix.a(('LDI','w',value)))
    rules(section='finish', bindings={'entry': entry}, sequences={'suffix': suffix.acts})
