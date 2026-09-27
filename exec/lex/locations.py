"""Load the declared pp.locations -> tokens.locations transition rules."""
from pathlib import Path
from finite_rules import load


def install(d):
    here = Path(__file__).resolve().parent
    for filename, mode, domain in (("location-byte.tsv", "b", range(257)),
                                   ("location-result.tsv", "r", range(3))):
        for name, entries in load(here / filename, {}, domain).items():
            row = d.state(name, mode)
            for key, (target, actions) in entries.items():
                row[key] = target, d.seq(actions)
    return "LOC.magic0"
