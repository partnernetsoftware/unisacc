"""E2 feasibility, MINIMUM slice: the preprocessor as one finite delta.

    python3 exec/pp/gen.py [out.json] [OS/ARCH]      -> writes the table, prints sizes

Machine and primitives: exec/pp/sim.py (generic, option A); design:
docs/exec/e2-pp-delta.md.  The delta runs the reference's passes in order,
each pass reading x and writing o, SWAP between passes:

  P0 shebang + splice      src/front_pp.c splice(), fe_load's shebang blank
  P1 decomment             decomment()            (reject: unterminated comment)
  P3 directives            preprocess(): #ifdef #ifndef #else #endif #define
                           #undef #include, unknown directives and dead lines
                           blanked; a live #include splices the file in and
                           re-runs P0, P1 on the whole buffer (as incdo does)
  P4 token rescan          object/function macros, argument pre-expansion,
                           zero/variadic arguments, # and covered ## forms;
                           replacement frames can continue into their parent
                           balanced _Pragma calls are ignored as in the reference

NOT covered (the delta rejects with a `not covered: ...` code, never guesses):
unterminated _Pragma calls; more than MAXP parameters; general punctuator/literal ## operands
(the identifier/digit and object-like `# ## #` forms are covered).
#if lines reuse P4 macro expansion before typed constant evaluation.
autoinc() (P2, the on-demand header prepend) is modelled: build_autoinc,
its trigger names read from include/*.h (E2_AUTOINC=0 builds without it).
Pragma macro stacks accept literal identifier names; escaped names are rejected.
The reference's prefix match `push_macroX` is not reproduced. Also not modelled:
file:line:col rendering of diagnostics (the reject kind is compared, not the
text).

Directive vocabulary and decisions come from weights/gold/pp.tsv, including
its schema. Target predefinitions come from predefines.tsv. No old kernel
file supplies generation data. Transition rules live in the adjacent TSVs;
see rules.md for their inputs and the remaining Python assembly boundary.
This is source migration, not a claim of complete C99 coverage.

--shared-predefines is an optional experiment: a target selector resource
chooses the predefines.tsv-derived macro-name resource; this same delta is
constructed for all six targets. Plain, located and no-autoinc tokenpp have
different output contracts and remain separate networks. The default product
path still uses its existing target-bound construction.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
_bgspec = __import__("importlib.util").util.spec_from_file_location("k2_build_graph_pp", os.path.join(ROOT, "exec", "build", "graph.py"))
_build_graph = __import__("importlib.util").util.module_from_spec(_bgspec)
_bgspec.loader.exec_module(_build_graph)
EOF = 256

sys.path.insert(0, ROOT)
from unisa.tsvgold import load_table
from pathlib import Path
sys.path.insert(0, os.path.join(ROOT, "exec"))
from finite_rules import install as install_rules
from finite_rules import install_template
from exec.facts.load import facts

# layout constants, byte classes, targets, reserved spellings: exec/facts/pp-*.tsv
LAYOUT = {r["name"]: r["value"] for r in facts("pp-layout")}
globals().update(LAYOUT)
TARGETS = tuple(r["os"] + "/" + r["arch"] for r in facts("pp-targets"))
_BYTES = {r["class"]: {c for a, b in r["ranges"] for c in range(a, b + 1)} for r in facts("pp-bytes")}

_name, _fields, _heads, _rows = load_table(os.path.join(ROOT, "weights/gold/pp.tsv"))
assert _name == "pp" and [n for n, _ in _fields] == ["dir", "defined"]
assert _fields[1][1] == ("0", "1") and [n for n, _, _ in _heads] == ["y"]
DIRV = _fields[0][1]
PPHEAD = list(_heads[0][1])
PPT = {(d, int(b)): labels["y"] for (d, b), labels in _rows.items()}
assert PPHEAD == ["take", "skip", "pop", "macro"], PPHEAD


def load_predefines():
    path = os.path.join(HERE, "predefines.tsv")
    rows = {}
    for line, text in enumerate(open(path, encoding="utf-8"), 1):
        if text.startswith("#") or not text.strip():
            continue
        fields = text.rstrip("\n").split("\t")
        key, names = tuple(fields[:2]), fields[2:]
        if (len(fields) < 3 or key in rows or len(set(names)) != len(names) or
                any(not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) for name in names)):
            raise ValueError(f"{path}:{line}: invalid or duplicate predefinition row")
        rows[key] = names
    expected = {("os", x) for x in ("lnx", "osx", "win")} | {
        ("arch", x) for x in ("x86_64", "arm64")} | {("common", "*")}
    if set(rows) != expected:
        raise ValueError(f"{path}: expected OS, architecture and common declarations")
    return rows


PREDEF = load_predefines()


# autoinc (src/front_pp.c autoinc/hdrneeded): the trigger data build_autoinc
# puts in the delta.  Order is the reference's; trigger names are derived from
# include/*.h by hdrneeded's line rule (a line opening `static` with `(` and
# a `{` after it names the identifier before the first `(`).
AUTOINC_ORDER = tuple(facts("pp-autoinc"))


def autoinc_map():
    m = {}
    for h in AUTOINC_ORDER:
        names = []
        for ln in open(os.path.join(ROOT, "include", h), encoding="latin-1").read().split("\n"):
            if len(ln) > 7 and ln.startswith("static") and "(" in ln and "{" in ln[ln.index("("):]:
                mm = re.search(r"([A-Za-z0-9_]+)\s*$", ln[:ln.index("(")])
                if mm:
                    names.append(mm.group(1))
        m[h] = names
    return m


AUTOINC = os.environ.get("E2_AUTOINC", "1") != "0"   # E2_AUTOINC=0: the delta without it
# AIB: W[AIB + id]: bit 1 called, bit 2 defined (srcuse)
# -ftrim-libc (src/front_pp.c ftrim_libc_scan/ftrim_libc_define): W[LNSB + id] = 1 when
# the identifier occurs anywhere in the unit (srcfind), kept apart from AIB so
# the autoinc status compare is untouched; W[NEEDB + b] = 1 marks carried body b.


def build_autoinc(g, locations=False):
    """P2 autoinc + -ftrim-libc: exec/pp/autoinc-manifest.tsv over facts/pp-autoinc-gen."""
    import assemble
    from types import SimpleNamespace
    assemble.run(Path(HERE) / "autoinc-manifest.tsv", SimpleNamespace(g=g), None, {})


AL, DI = _BYTES["alpha"], _BYTES["digit"]
ID = AL | DI
# W regions (addresses; plain named slots are strings)
# #line records (R14-3): W[LDRAW+k] raw line, W[LDUSER+k] user line, W[LDNUM+k] N, W[LDNM+k] name blob or -1
# s13: a function-like call's record, a stack (CL deep, CR the top's address):
# the macro, the argument index, the registers a pre-expansion saves, and per
# argument the raw blob and the fully expanded one
# F_FN: 0 object-like, 1 function-like (covered), 2 function-like not covered
# (more than MAXP or malformed)


G = _build_graph.G   # generic graph primitives (exec/build/graph.py)


def sbconst(s):
    return [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]


# ---- XE: #if expression evaluator (docs/exec/e2-pp-delta.md s11.2) ---------
# shunting-yard over an operator stack (XOB) and value/poison stacks (XVB,
# XPB) in W; signed intmax literals/ordinary signed-char constants via
# literal-*.tsv, with checked uint64 accumulation and XUB type slots. Wide
# and multichar constants remain explicit refusals. A64/C64 handle both types.
# A division by zero sets the value's
# poison bit; && || ?: drop the poison of the operand they do not evaluate.
# XUB uses a separate sparse region (IRNAME is 1 << 40); all its indexing is A64I.
# code: (spelling, prec, arity)
XOPS = {}
for line in (Path(HERE) / "operators.tsv").read_text().splitlines():
    if not line or line.startswith("#"):
        continue
    code, spelling, precedence, arity = line.split("\t")
    code, precedence, arity = int(code), int(precedence), int(arity)
    assert code not in XOPS and 0 < code < 257 and precedence >= 0 and arity in (0, 1, 2, 3)
    assert spelling and spelling not in {v[0] for v in XOPS.values()}
    XOPS[code] = spelling, precedence, arity
assert XOPS, "empty expression operator declarations"


def xe_init():
    a = []
    for c, (_, p, _) in list(XOPS.items()) + [(0, (None, -1, 0))]:
        a += [("LDI", "xc", XPRB + c), ("LDI", "xq", p), ("STX", "xc", 0, "xq")]
    return a


def build_xe(g):
    layout = {name: globals()[name] for name in ['XOB', 'XVB', 'XPB', 'XPRB', 'XUB']}
    layout["XOB_PREV"] = XOB - 1
    layout.update(("PREC_" + str(c), p) for c, (_, p, _) in XOPS.items())
    from unisa.front.lex import ESC
    simple = [{"code": ord(ch), "value": ord(value)} for ch, value in ESC.items() if ch not in '01234567x']
    install_template(g, HERE, "escape", {"esc": simple}, None, section="escape", mode="b",
                     domain=[e["code"] for e in simple])   # the rest of XCESC is literal-byte.tsv
    install_rules(g, HERE, "literal", layout)
    install_rules(g, HERE, "expression", layout)
    install_rules(g, HERE, "reduce", layout)


# ---- # and ## (docs/exec/e2-pp-delta.md s12) -------------------------------
# HSCAN (at #define): F_HASH := 1 when the body has `#` outside literals.
# HX (at expansion, `me` = the macro, arguments in ARGB): the body is
# rewritten into the string builder and saved as blob NB, which is then
# rescanned by the declared replacement-frame actions:
#   `# p`     -> `"` + the argument's spelling, blanks trimmed, inner blank
#               runs one space, `"` and `\` escaped inside literals + `"`
#   `a ## b`  -> the two spellings concatenated (no space); a placemarker
#               (empty argument) contributes nothing
#   other parameters -> the argument text (blank runs one space), padded by
#               a space each side, so the rescan tokenises it on its own
# Covered paste operands: identifier/digit boundaries, empty arguments and
# object-like # ## #. General punctuator/literal pastes remain not covered
# (the reference re-tokenises concatenated spellings).
# Vars: GLUE (1 ordinary paste, 2 comma/variadic special case), PSP (space
# owed), LASTK (1 identifier/digit, 2 placemarker, 3 hash, 4 comma, 0 other).
def build_hx(g):
    install_rules(g, HERE, "hash", {name: globals()[name] for name in ['C_EXP', 'C_RAW', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_VAR']})



def build_cli(g, locations=False):
    layout = {name: globals()[name] for name in ['F_BODY', 'F_TO', 'NEWB', 'FSZ', 'MACB']}
    # Without autoinc the ftrim-libc scan is absent; continue with forced includes.
    layout['after_flags'] = "CLI.LN" if AUTOINC else "CLI.INC"
    install_rules(g, HERE, "cli", layout,
        {"location_line": [("ALUI", "add", "CLI_PRELINES", "CLI_PRELINES", 1)]})   # every build: __LINE__ needs it (N17a)



def target_predefines(target):
    """Ordered declaration bytes for the optional shared-target E2 variant."""
    target_os, target_arch = target.split("/")
    names = PREDEF["os", target_os] + PREDEF["arch", target_arch] + PREDEF["common", "*"]
    if len(set(names)) != len(names):
        raise ValueError("overlapping target predefinitions: " + target)
    return b"".join(name.encode("ascii") + b"\0" for name in names)


def predefine_resources():
    return {b"\0predefines/" + (osname+"/"+arch).encode("ascii"):
            target_predefines(osname+"/"+arch)
            for osname, arch in (t.split("/") for t in sorted(TARGETS, key=lambda t: (t.split("/")[0], t.split("/")[1])))}


def build(target="lnx/x86_64", locations=False, shared_predefines=False):
    if target not in TARGETS:
        raise ValueError("unsupported preprocessor target: "+target)
    target_os, target_arch = target.split("/")
    predef = PREDEF["os", target_os] + PREDEF["arch", target_arch] + PREDEF["common", "*"]
    if len(set(predef)) != len(predef):
        raise ValueError("overlapping target predefinitions: " + target)
    g = G()

    # ---- init: constant ids (built byte by byte, then interned) ---------
    init = []
    for k, w in enumerate(DIRV):
        init += sbconst(w) + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", DIRB),
                              ("LDI", "v", k + 1), ("STX", "a", 0, "v")]
    for r in facts("pp-init"):   # reserved spellings: #pragma/#line/#error ids, interned names
        if r["kind"] == "dir":
            init += sbconst(r["word"]) + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", DIRB),
                                          ("LDI", "v", r["arg"]), ("STX", "a", 0, "v")]
        else:
            init += sbconst(r["word"]) + [("SBINTERN", r["arg"])]
    init += [("LDI", "RUN", 0), ("LDI", "FP", 0)] + xe_init()
    g.els("START", "CLI.FLAGS", init)
    build_cli(g, locations)

    # Declared text normalisation; only layout and inter-stage links are bound here.
    install_rules(g, HERE, "text", {"SPLB": SPLB, "after_comments": "AISTART" if AUTOINC else "P3START"})

    # Macro history and definition rules; bindings describe record layout only.
    macro_layout = {'NEWB': NEWB, 'FSZ': FSZ, 'MACB': MACB, 'F_TO': F_TO, 'SEGINF': SEGINF, 'F_FROM': F_FROM, 'F_PREV': F_PREV, 'F_NAME': F_NAME, 'F_BODY': F_BODY, 'F_FN': F_FN, 'F_VAR': F_VAR}
    install_rules(g, HERE, "macro", macro_layout)
    install_rules(g, HERE, "pragma", dict(macro_layout, F_NP=F_NP,
                  PMHEAD=PMHEAD, PMSTACK=PMSTACK))

    if AUTOINC:
        build_autoinc(g, locations)

    install_rules(g, HERE, "directive-scan", {"TAKEB": TAKEB, "SEENB": SEENB, "DIRB": DIRB})
    install_rules(g, HERE, "object-predefine")
    install_rules(g, HERE, "assembly", {"entry": "OOBJ.DEF", "resume": "OOBJ.RESUME",
        "next": "CLI.U", "F_BODY": F_BODY}, {"name": sbconst("__UNISA_OBJECT")}, section="predefine")
    _IRNAME = {r["name"]: r["value"] for r in facts("pp-layout")}["IRNAME"]
    install_rules(g, HERE, "linedir", {"LDRAW": LDRAW, "LDUSER": LDUSER, "LDNUM": LDNUM, "LDNM": LDNM, "IRNAME": _IRNAME, "F_FN": F_FN, "F_BODY": F_BODY, "FSZ": FSZ, "MACB": MACB})
    if shared_predefines:
        # One network per output format/autoinc mode; target data is supplied
        # as resources. The legacy default remains byte-for-byte unchanged.
        install_rules(g, HERE, "shared-predefine", {"F_BODY": F_BODY, "PD_SEEN": PD_SEEN})
    else:
        for k, nm in enumerate(predef):
            nxt = "P3PD%d" % (k + 1) if k + 1 < len(predef) else "OOBJ.start"
            install_rules(g, HERE, "assembly", {"entry": "P3PD%d" % k, "resume": "P3PDR%d" % k,
                "next": nxt, "F_BODY": F_BODY}, {"name": sbconst(nm)}, section="predefine")

    cases = {0: ("P3BLANK", [("JUMP", "LS")]), 100: ("PRAG", [("RLD", "LIVE")]), 101: ("LDIR", [("RLD", "LIVE")]),
             102: ("D_ERROR", [("RLD", "LIVE")])}
    cases.update((k + 1, ("D_" + w, [])) for k, w in enumerate(DIRV))
    g.r("DSW", cases)
    for w in DIRV:
        st = "D_" + w
        for fl in (0, 1):
            name = st + "_a%d" % fl
            links = {suffix or "entry": name + suffix for suffix in ("", "b", "c", "m", "n")}
            links.update({key: globals()[key] for key in ['TAKEB', 'SEENB', 'F_TO', 'FSZ', 'MACB']})
            install_rules(g, HERE, "directive-action", links, section=w + "/" + PPT[(w, fl)])

    body_layout = {name: globals()[name] for name in ['F_BODY', 'F_FN', 'F_NP', 'F_P0', 'F_VAR', 'IRLN', 'IRNL', 'MAXP']}
    # every include is recorded (name for the location envelope, resolved path for nested quoted
    # includes, R13-0b #24): a quoted include inside a header is looked up beside that header,
    # which the reference finds through its include-region table (front_pp.c hdr_find).
    body_layout["include_body"] = "INC.body"
    body_layout["IRPATH"] = IRPATH
    install_rules(g, HERE, "directive-body", body_layout)
    IRNAME = _IRNAME
    install_rules(g, HERE, "include-location", {"IRNAME": IRNAME, "IRPATH": IRPATH})

    install_rules(g, HERE, "rescan", dict({name: globals()[name] for name in ['LDRAW', 'LDUSER', 'LDNUM', 'LDNM', 'ARGB', 'ARGE', 'CRB', 'CRS', 'C_BDEP', 'C_EDEP', 'C_EXP', 'C_K', 'C_ME', 'C_OST', 'C_PRE', 'C_RAW', 'C_SB', 'C_SB0', 'C_SEP', 'FSZ', 'F_ACT', 'F_BODY', 'F_FN', 'F_HASH', 'F_NP', 'F_P0', 'F_UP', 'F_VAR', 'MACB', 'MAXP', 'IRLN', 'IRNL', 'SPLB']}, IRNAME=IRNAME))   # IRLN/IRNL/SPLB/IRNAME: __LINE__/__FILE__ (N17a/b)
    if locations:
        import assemble   # diagnostic envelope: locations-manifest.tsv (K2 trace translation)
        from types import SimpleNamespace
        assemble.run(Path(HERE) / "locations-manifest.tsv", SimpleNamespace(g=g), None, {},
                     dict(spl=SPLB, irln=IRLN, irnl=IRNL))
    else:
        install_rules(g, HERE, "assembly", section="accept")
    build_xe(g)
    build_hx(g)
    import assemble   # E2 provenance envelope: sourcefacts-manifest.tsv (K2 trace translation)
    from types import SimpleNamespace
    assemble.run(Path(HERE) / "sourcefacts-manifest.tsv", SimpleNamespace(g=g), None, {})
    g.finish()
    return g


def sizes(g):
    ent = sum(len(r) for _, r in g.st.values())
    sparse = 0
    for mode, row in g.st.values():
        vals = list(row.values())
        common = max(set(vals), key=vals.count)
        sparse += 1 + 4 + 5 * sum(1 for v in vals if v != common)
    pool = sum(len(s) for s in g.seqs)
    poolb = sum(1 + sum(1 + 4 * (len(a) - 1) for a in s) for s in g.seqs)
    modes = {}
    for mode, _ in g.st.values():
        modes[mode] = modes.get(mode, 0) + 1
    return dict(states=len(g.st), modes=modes, entries=ent, unreachable=g.unreach,
                seqs=len(g.seqs), pool_actions=pool, pool_bytes=poolb,
                sparse_bytes=sparse + poolb, dense_bytes=4 * ent + poolb + len(g.st),
                gamma=len(g.labels) + 1,
                action_kinds=sorted(set(a[0] for s in g.seqs for a in s)))


def main():
    locations="--locations" in sys.argv
    if locations: sys.argv.remove("--locations")
    shared_predefines="--shared-predefines" in sys.argv
    if shared_predefines: sys.argv.remove("--shared-predefines")
    if len(sys.argv)>3:
        sys.exit("usage: gen.py [OUT.json] [OS/ARCH] [--locations] [--shared-predefines]")
    g = build(sys.argv[2] if len(sys.argv)>2 else "lnx/x86_64", locations=locations,
              shared_predefines=shared_predefines)
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/e2delta.json"
    json.dump({"start": "START", "states": {k: [m, {str(kk): list(v) for kk, v in r.items()}]
                                            for k, (m, r) in g.st.items()},
               "seqs": [list(map(list, s)) for s in g.seqs]}, open(out, "w"))
    for k, v in sizes(g).items():
        print("%-14s %s" % (k, v))


if __name__ == "__main__":
    main()
