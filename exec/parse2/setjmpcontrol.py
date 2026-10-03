"""Install table-declared setjmp temporary-depth scan."""
from pathlib import Path
from finite_rules import install as install_rules, install_template


def install(E, P):
    g = E.g
    # The scan starts after the function's frame reservation.
    install_template(g, Path(__file__).parent, 'setjmpcontrol', {}, None, section='frame')
    install_rules(g, Path(__file__).parent, 'setjmpcontrol', section='setjmp')
