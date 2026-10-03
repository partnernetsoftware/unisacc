"""Load the declared pp.locations -> tokens.locations transition rules."""
from pathlib import Path
from finite_rules import load
from exec.facts.load import facts


def install(d):
    here = Path(__file__).resolve().parent
    for filename, mode, domain in ((r["file"], r["mode"], range(r["domain"]))
                                   for r in facts("lex-installs") if r["group"] == "locations"):
        for name, entries in load(here / filename, {}, domain).items():
            row = d.state(name, mode)
            for key, (target, actions) in entries.items():
                row[key] = target, d.seq(actions)
    return "LOC.magic0"
