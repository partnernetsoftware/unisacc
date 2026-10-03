"""Install table-declared setjmp temporary-depth scan."""
from pathlib import Path
from finite_rules import install as install_rules


def install(E, P):
    g = E.g
    # The scan starts after the function's frame reservation.
    mode, rows = g.st['FN.go']
    g.st['FN.go'] = (mode, {key: (target, g.seq([('OLEN', 'sj_body'), *g.seqs[seq]]))
                              for key, (target, seq) in rows.items()})
    install_rules(g, Path(__file__).parent, 'setjmpcontrol', section='setjmp')
