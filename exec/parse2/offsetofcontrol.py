"""Install table-declared offsetof parsing over E3 type and member facts."""
from pathlib import Path
from finite_rules import install as install_rules, install_template


def install(E, P, SBB, MEMBER_STRIDE, MOF, MSZ, MBS, MAR):
    g = E.g
    install_template(g, Path(__file__).parent, 'offsetofcontrol', {}, None, section='entry')
    g.labels.add('OFF.original')
    install_rules(g, Path(__file__).parent, 'offsetofcontrol', section='offsetof',
                  bindings=dict(SBB=SBB, MEMBER_STRIDE=MEMBER_STRIDE, MOF=MOF,
                                MSZ=MSZ, MBS=MBS, MAR=MAR))
