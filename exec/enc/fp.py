"""Generate x86 FP encoding transitions, not host-encoded instruction bytes.

Opcode/predicate declarations are read from emit_x86. Shared fp-*.tsv rules
declare SSE layout, register moves and unsigned conversion transitions. The executor has no FP-encoding primitive. Operand registers
are the seven non-stack tape registers; xmm0/1 and r11/rbx are scratch.
"""
from unisa.emit_x86 import NUM
from unisa.catalog import REGMAP
from exec.facts.load import facts

OPS = facts('enc-fp-ops')
FP_IDS = {r['op']: r['id'] for r in OPS}


def install(E, byte):
    from pathlib import Path
    from finite_rules import install as install_rules
    root = Path(__file__).parent
    names = [line.split('\t') for line in (root/'fp-names.tsv').read_text().splitlines() if not line.startswith('#')]
    sequences = {'byte'+value: byte(E.P('byte.binding'),int(value)).acts for value in
                 (root/'fp-bytes.tsv').read_text().splitlines() if not value.startswith('#')}
    registers = tuple(NUM[r] for r in REGMAP['x86_64'][:7])
    def install(section, bindings, count=2):
        bindings.update({name: E.P(owner).fresh(kind) for selected, name, owner, kind in names if selected==section})
        dynamic = {'byte_'+key: byte(E.P('byte.binding'),bindings[key]).acts
                   for key in ('prefix','inverseprefix','opcode','predicate') if key in bindings}
        install_rules(E.g, root, 'fp', section=section, bindings=bindings,
                      classes={'arity': (count,), 'registers': registers}, sequences={**sequences, **dynamic})
    install('shared', {})
    for r in OPS:
        op, section, width, count = r['op'], r['section'], r['width'], r['count']
        install('arity', dict(entry='FP.'+op, next='FP.'+op+'.r0'), count)
        for i in range(count):
            install('register', dict(entry='FP.'+op+'.r'+str(i), value='a'+str(i),
                next='FP.'+op+('.r'+str(i+1) if i+1<count else '.body')))
        install(section, dict(entry='FP.'+op+'.body', width=width, inversewidth=1-width,
            prefix=0xF2 if width else 0xF3, inverseprefix=0xF3 if width else 0xF2,
            opcode=r['opcode'], predicate=r['predicate']))
