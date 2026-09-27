"""Bind the shared declared optional little-endian u64 resource reader."""
from pathlib import Path
from finite_rules import install as install_rules


def u64(E, label, key, result, present, fail):
    bindings = dict(entry=label, length=label+'.length', read=label+'.read',
                    absent=E.P(label).fresh('b'), size=E.P(label+'.length').fresh('b'),
                    result=result, present=present, fail=fail)
    install_rules(E.g, Path(__file__).parent, 'modelinput', bindings=bindings,
                  sequences={'key': [('SBOUT', c) for c in key]}, section='u64')
