"""Opt-in E3 sidecar for reachable unresolved calls in macOS -run.

The parser owns the prototype facts.  The host driver receives only framed
names and the existing USLSIG3 descriptor; it never guesses a C signature
from a call instruction.  Without the resource, this hook is inert.
"""

RECORDS = 770 << 40
SEEN = 771 << 40


def install(E, P, start, fps_fn, definitions):
    g = E.g
    key = b"\0cli/run-forward"
    P("FW.start").a(("SBCLR",), *(("SBOUT", c) for c in key),
                    ("SBFIND", "fw_flagblob"), ("BLEN", "fw_enabled", "fw_flagblob"),
                    ("LDI", "fw_count", 0), ("LDI", "fw_zero", 0)).goto(start)

    original = "FW.original.UD.error"
    g.st[original] = g.st.pop("UD.error")
    for how in ("P", "L"):
        if ("UD.error", how) in definitions:
            definitions[original, how] = definitions.pop(("UD.error", how))
    g.labels.add(original)
    P("UD.error").branch({0: original}, "FW.lookup", [("RLD", "fw_enabled")])
    P("FW.lookup").a(("INTERN", "fw_id", "ud_start", "ud_end"),
                     ("LDX", "fw_seen", "fw_id", SEEN)).branch(
                         {1: "UD.nextline"}, "FW.proto", [("RLD", "fw_seen")])
    P("FW.proto").a(("LDX", "lx_sig", "fw_id", fps_fn)).branch(
                         {0: "FW.missing"}, "FW.name", [("RLD", "lx_sig")])
    # The C reference does not forward an unprototyped call.  Keep the usual
    # named undefined-function diagnostic from UD.error in that case.
    P("FW.missing").goto(original)
    # UD is scanning a saved copy of the already emitted tape.  The current
    # output is empty, so it can hold one record until OCUT stores its blob.
    P("FW.name").branch({2: "FW.full"}, "FW.capture",
                          [("CMPI", "fw_count", 8192)])
    P("FW.full").a(E.rej("not covered: host-forwarded call count exceeds 8192")).goto("DEAD")
    P("FW.capture").a(("LDI", "fw_one", 1), ("STX", "fw_id", SEEN, "fw_one"),
                   ("SPAN2", "ud_start", "ud_end"),
                   ("OCUT", "lx_nameblob", "fw_zero"),
                   ("LDI", "lx_wireversion", 3)).call("LX.signature").goto("FW.record")
    (P("FW.record").a(("BLEN", "lx_v", "lx_nameblob")).call("LX.u64")
     .a(("INPUSH", "lx_nameblob"), ("XLEN", "fw_end"),
        ("SPAN2", "fw_zero", "fw_end"), ("INPOP",),
        ("BLEN", "lx_v", "lx_sigblob")).call("LX.u64")
     .a(("INPUSH", "lx_sigblob"), ("XLEN", "fw_end"),
        ("SPAN2", "fw_zero", "fw_end"), ("INPOP",),
        ("OCUT", "fw_record", "fw_zero"),
        ("STX", "fw_count", RECORDS, "fw_record"),
        ("ALUI", "add", "fw_count", "fw_count", 1)).goto("UD.nextline"))

    # LX.accept is the final normal acceptance after library envelope routing.
    accept = "FW.original.LX.accept"
    g.st[accept] = g.st.pop("LX.accept")
    for how in ("P", "L"):
        if ("LX.accept", how) in definitions:
            definitions[accept, how] = definitions.pop(("LX.accept", how))
    g.labels.add(accept)
    P("LX.accept").branch({0: accept}, "FW.envelope", [("RLD", "fw_enabled")])
    (P("FW.envelope").a(("OCUT", "fw_tape", "fw_zero")).o("USLFW1\n")
     .a(("BLEN", "lx_v", "fw_tape")).call("LX.u64")
     .a(("COPYW", "lx_v", "fw_count")).call("LX.u64")
     .a(("INPUSH", "fw_tape"), ("XLEN", "fw_end"),
        ("SPAN2", "fw_zero", "fw_end"), ("INPOP",),
        ("LDI", "fw_i", 0)).goto("FW.records"))
    P("FW.records").branch({0: "FW.copy"}, accept, [("CMP", "fw_i", "fw_count")])
    P("FW.copy").a(("LDX", "fw_record", "fw_i", RECORDS),
                   ("INPUSH", "fw_record"), ("XLEN", "fw_end"),
                   ("SPAN2", "fw_zero", "fw_end"), ("INPOP",),
                   ("ALUI", "add", "fw_i", "fw_i", 1)).goto("FW.records")
    return "FW.start"
