"""Signed decimal formatter as fixed x86 instruction templates plus delta RIP
relocations. The templates implement count/write loops; they do not contain
input-dependent precomputed addresses, digits, or reference encoder results.
"""
from address import VALUE, VALUE2, DEST

from exec.facts.load import facts
_B = {r['part']: bytes.fromhex(r['hex']) for r in facts('enc-x86itoa-bytes')}
PREFIX, SUFFIX = _B['prefix'], _B['suffix']
SIZE=21+len(PREFIX)+len(SUFFIX)

def install(E,KND,SZ):
    from pathlib import Path
    from functools import partial
    from finite_rules import install as install_rules
    rules = partial(install_rules, E.g, Path(__file__).parent, 'x86itoa')
    rules(section='arity', bindings={'test': E.P('ITO').fresh('b')})
    for i in range(3):
        rules(section='argument', bindings=dict(entry='ITO.args' if i == 0 else 'ITO.bound'+str(i-1),
            kindtest=E.P('ITO').fresh('b'), kindentry='ITO.kind'+str(i), kind='ak'+str(i),
            boundtest=E.P('ITO').fresh('b'), value='a'+str(i), next='ITO.bound'+str(i)))
    rules(section='store', bindings=dict(VALUE=VALUE, VALUE2=VALUE2, DEST=DEST, KND=KND, SZ=SZ, SIZE=SIZE))
    entry = 'WR.itoa'
    tables, prefixes = dict(VALUE=VALUE, DEST=DEST, VALUE2=VALUE2), dict(none=b'', prefix=PREFIX)
    for reg, opcode, table, prefix in ((r['reg'], r['opcode'], tables[r['table']], prefixes[r['prefix']])
                                       for r in facts('enc-x86itoa-rip')):
        nxt = E.P('WR').fresh('r')
        rules(section='rip', bindings=dict(entry=entry, next=nxt, reg=reg, opcode=opcode, table=table),
              sequences={'prefix': [('OUT', b) for b in prefix]})
        entry = nxt
    rules(section='finish', bindings={'entry': entry}, sequences={
        'suffix': [('OUT', b) for b in SUFFIX], 'reject': E.rej('not covered: itoa address/operands')})
