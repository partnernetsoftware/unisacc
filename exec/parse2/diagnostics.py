"""Reference diagnostic positioning/rendering compiled to ordinary actions.
DIAG.report takes diag_pos (preprocessed byte offset), diag_message (blob),
and diag_warning (0=error, 1=warning). It returns diag_reported=0 for header
warnings, otherwise 1, and restores the input frame and stdout selection.
All scratch registers and decimal digits are private to this routine.
"""
from tokenlocations import SPLICES,INCLUDE_LINE,INCLUDE_LINES,INCLUDE_NAME
DIGITS=39<<40


def install(E,P):
    import json
    from pathlib import Path
    from finite_rules import install as install_rules
    root=Path(__file__).parent
    bindings=dict(SPLICES=SPLICES, INCLUDE_LINE=INCLUDE_LINE, INCLUDE_LINES=INCLUDE_LINES,
                  INCLUDE_NAME=INCLUDE_NAME, DIGITS=DIGITS)
    p=P('diagnostics.bindings')
    for line in (root/'diagnostics-fresh.tsv').read_text().splitlines()[1:]:
        owner,kind,key=line.split('\t')
        p.cur=owner
        bindings[key]=p.fresh(kind)
    sequences={}
    for line in (root/'diagnostics-text.tsv').read_text().splitlines()[1:]:
        name,value=line.split('\t')
        sequences[name]=E.O(json.loads(value))
    install_rules(E.g,root,'diagnostics',bindings=bindings,sequences=sequences)
