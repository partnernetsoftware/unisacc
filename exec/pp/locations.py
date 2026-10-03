"""Optional preprocessing diagnostic envelope, emitted by ordinary actions.
UNIPP1 NUL; five u32-le words (text length, forced lines, automatic lines,
splice count, include count); splice offsets; includes as (line, line count,
name length, name bytes); then exactly text-length preprocessed bytes.
"""
from pathlib import Path
from finite_rules import install as install_rules
from exec.facts.load import facts

IRNAME = {r["name"]: r["value"] for r in facts("pp-layout")}["IRNAME"]


def install(g, spl, irln, irnl):
    install_rules(g, Path(__file__).parent, "location",
                  {"SPLB": spl, "IRLN": irln, "IRNL": irnl, "IRNAME": IRNAME})
