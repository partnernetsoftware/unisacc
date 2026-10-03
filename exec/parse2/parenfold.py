"""Fold parenthesised declarator names at E3's token-reader boundary.

The E1 stream remains the lexer stream.  The reference folds tokens between
lexing and parsing, so this wrapper gives every E3 consumer the same stream
without adding five separate function-pointer/declarator cases.
"""
from pathlib import Path
from finite_rules import install as install_rules, install_template


def install(E, ordinal_table, locations=False):
    g = E.g
    # Location and static-object readers replace NEXT during construction.
    # Wrap their final entry, while their internal lexer entry stays TN.raw.
    assert "NEXT" in g.st and "TN.checked" not in g.st
    install_template(g, Path(__file__).parent, "parenfold", {},
                     lambda kind: None, section="entry")
    peek = "TN.raw"
    if locations:
        # The located reader consumes an @position prefix before NX.  Clone
        # only its entry and return to NX before ordinal assignment; the
        # shared prefix decoder and its validation remain unchanged.
        # Warning/error variants wrap NEXT with their own checks before the
        # located decoder (DL.read).  In the locations-only variant NEXT is
        # already that decoder.  Lookahead skips only the outer checks.
        reader = "DL.read" if "DL.read" in g.st else "TN.checked"
        mode, source = g.st[reader]
        assert mode == "r"
        for target, seqid in source.values():
            acts = list(g.seqs[seqid])
            assert target == "DL.prefix" and acts[0] == ("MARK", "tpos")
            assert acts[1][0] == "PUSH" and len(acts) == 2
        install_template(g, Path(__file__).parent, "parenfold",
                         dict(ctx=[dict(reader=reader)]), lambda kind: None,
                         section="located")
        peek = "PF.locraw"
    TK = E.TK
    lp, rp, ident = TK["("], TK[")"], E.TK_ID
    followers = tuple(TK[x] for x in ("(", "=", ";", ",", "[", ")"))
    # TYPEV's lexical class is one token in the reference.  The typed stream
    # spells each type word separately; qualifiers have already been skipped
    # by the shared E3 reader.
    types = {value for word, value in TK.items() if word == "type" or word.startswith("type=")}
    previous = types | {TK["*"]}
    g.labels.update(("PF.got", "PF.id", "PF.close", "PF.follow", "PF.prevadj", "PF.prevtypedef"))
    # The transition choices live in parenfold-result.tsv.  Python supplies
    # only the current token codes and the reader selected by the location mode.
    bindings = dict(peek=peek, ordinal_table=ordinal_table, TDN=E.TDN,
                    lp=lp, ident=ident)
    classes = dict(open=[lp], previous=sorted(previous), identifier=[ident],
                   close=[rp], followers=followers, yes=[1])
    if locations:
        install_rules(g, Path(__file__).parent, 'parenfold', bindings=bindings,
                      classes=classes, section='located')
    install_rules(g, Path(__file__).parent, 'parenfold', bindings=bindings,
                  classes=classes, section='main')
