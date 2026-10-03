"""Opt-in E2 provenance envelope; all work uses existing generic actions."""
from pathlib import Path
from finite_rules import install as install_rules, install_template


def _nofresh(kind):
    raise ValueError("sourcefacts allocates no fresh labels")


def install(g):
    here = Path(__file__).parent
    # START moved aside, every edge ending in ACCEPT finishes through SF.finish
    install_template(g, here, 'sourcefacts', {}, _nofresh, section='pre')
    install_rules(g, here, 'sourcefacts')
    install_template(g, here, 'sourcefacts', {}, _nofresh, section='post')
