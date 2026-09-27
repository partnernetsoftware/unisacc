"""ARM memory/zero-fill transitions declared in TSV; opcode facts read from seed."""
from unisa.emit_arm import LDS, STS, LDU, STU, IP0


def install(E, word):
    from pathlib import Path
    from finite_rules import install as install_rules
    # Preserve existing generated branch/return identities; TSV owns the control.
    labels = (('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'),
              ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'),
              ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'),
              ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'b'), ('MEM', 'r'),
              ('MEM', 'b'), ('EMIT', 'b'), ('ZERO', 'b'), ('ZERO', 'b'),
              ('ZERO', 'b'), ('ZERO', 'b'))
    bindings = {'label'+str(i): E.P(owner).fresh(kind)
                for i, (owner, kind) in enumerate(labels)}
    bindings.update({name+str(width): value for name, table in
                     (('LDS', LDS), ('STS', STS), ('LDU', LDU), ('STU', STU))
                     for width, value in table.items()})
    bindings.update(IP0=IP0, IP0_fields=(IP0 << 16) | IP0)
    install_rules(E.g, Path(__file__).parent, 'armmem', bindings=bindings,
                  sequences={'word': word(E.P('word.binding')).acts}, section='memory')
