"""Fold parenthesised declarator names at E3's token-reader boundary.

The E1 stream remains the lexer stream.  The reference folds tokens between
lexing and parsing, so this wrapper gives every E3 consumer the same stream
without adding five separate function-pointer/declarator cases.
"""


def install(E, ordinal_table, locations=False):
    g = E.g
    # Location and static-object readers replace NEXT during construction.
    # Wrap their final entry, while their internal lexer entry stays TN.raw.
    assert "NEXT" in g.st and "TN.checked" not in g.st
    g.st["TN.checked"] = g.st.pop("NEXT")
    peek = "TN.raw"
    if locations:
        # The located reader consumes an @position prefix before NX.  Clone
        # only its entry and return to NX before ordinal assignment; the
        # shared prefix decoder and its validation remain unchanged.
        mode, source = g.st["TN.checked"]
        assert mode == "r"
        copied = {}
        for key, (target, seqid) in source.items():
            acts = list(g.seqs[seqid])
            assert target == "DL.prefix" and acts[0] == ("MARK", "tpos")
            assert acts[1][0] == "PUSH" and len(acts) == 2
            copied[key] = (target, g.seq([acts[0], ("PUSH", "PF.locdone")]))
        g.st["PF.locraw"] = [mode, copied]
        g.labels.add("PF.locdone")
        peek = "PF.locraw"
    TK = E.TK
    lp, rp, ident = TK["("], TK[")"], E.TK_ID
    followers = tuple(TK[x] for x in ("(", "=", ";", ",", "[", ")"))
    # TYPEV's lexical class is one token in the reference.  The typed stream
    # spells each type word separately; qualifiers have already been skipped
    # by the shared E3 reader.
    types = {value for word, value in TK.items() if word == "type" or word.startswith("type=")}
    previous = types | {TK["*"]}
    g.labels.update(("PF.got", "PF.id", "PF.close", "PF.follow"))

    def any_r(state, target, actions=()):
        g.on(state, range(257), target, actions, "r")

    if locations:
        any_r("PF.locdone", "NX")
    any_r("NEXT", "TN.checked", [("PUSH", "PF.got")])
    any_r("PF.got", "PF.open", [("RLD", "tk")])
    g.on("PF.open", [lp], "PF.previous", [("RLD", "pf_previous")], "r")
    any_r("PF.open", "PF.done")
    g.on("PF.previous", previous, "PF.readid", [], "r")
    any_r("PF.previous", "PF.done")
    any_r("PF.readid", peek, [("MARK", "pf_afteropen"),
                                   ("COPYW", "pf_openpos", "tpos"), ("PUSH", "PF.id")])
    any_r("PF.id", "PF.idtest", [("RLD", "tk")])
    g.on("PF.idtest", [ident], "PF.readclose", [
        ("COPYW", "pf_ids", "ps"), ("COPYW", "pf_ide", "pe"),
        ("COPYW", "pf_idpos", "tpos")], "r")
    any_r("PF.idtest", "PF.fail")
    any_r("PF.readclose", peek, [("PUSH", "PF.close")])
    any_r("PF.close", "PF.closetest", [("RLD", "tk")])
    g.on("PF.closetest", [rp], "PF.readfollow", [], "r")
    any_r("PF.closetest", "PF.fail")
    any_r("PF.readfollow", peek, [("PUSH", "PF.follow")])
    any_r("PF.follow", "PF.followtest", [("RLD", "tk")])
    g.on("PF.followtest", followers, "PF.fold", [], "r")
    any_r("PF.followtest", "PF.fail")
    any_r("PF.fold", "RET", [("LDX", "pf_neword", "pf_openpos", ordinal_table),
                             ("STX", "pf_idpos", ordinal_table, "pf_neword"),
                             ("COPYW", "ixn", "pf_neword"),
                             ("JUMP", "tpos"), ("LDI", "tk", ident),
                             ("COPYW", "ps", "pf_ids"), ("COPYW", "pe", "pf_ide"),
                             ("COPYW", "tpos", "pf_idpos"), ("LDI", "pf_previous", ident)])
    any_r("PF.fail", "RET", [("JUMP", "pf_afteropen"), ("LDI", "tk", lp),
                             ("COPYW", "tpos", "pf_openpos"), ("LDI", "pf_previous", lp)])
    any_r("PF.done", "RET", [("COPYW", "pf_previous", "tk")])
