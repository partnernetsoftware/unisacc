#!/usr/bin/env python3
"""Export domain facts to exec/facts/*.tsv (K2; format in exec/assemble.py).

TABLES declares (fact stem, input files, producer).  A producer returns a list
of lines in the facts format; the written file starts with one
`# input PATH sha256:PREFIX` line per input, like weights/gold/*.tsv.

  python3 exec/facts/export.py            write every declared table
  python3 exec/facts/export.py --check    exit 1 when a committed table differs
"""
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


# _autonames went with k2-gen2 to tools/facts-gen.cx on 2026-10-30.


# _module, _parse_e and _gen2ns (the parse2 namespace) went with the k2 tables to tools/facts-gen.cx on 2026-10-30.


# _ppsrc (E2 source facts for pp-gen/pp-autoinc-gen) went with them to tools/facts-gen.cx on 2026-10-30.


# lex-gen moved to tools/facts-gen.cx on 2026-10-30.


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


# k2-constexpr and structreturnexpr moved to tools/facts-gen.cx on 2026-10-30.


def _gold(name):
    lines = (ROOT / "weights" / "gold" / (name + ".tsv")).read_text().splitlines()
    return [x.split("\t") for x in lines if x and not x.startswith("#") and "=>" not in x]


def lowercode():
    """Lower code-route target facts: tape-word interning per os/arch (TAPE SHAPE words,
    tape registers, fixed spellings), the machine register / encoding form each word
    carries, and the side-table key bases (was exec/lower/code.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    out = ["=%s\tint\t%d" % (n, (105 + i) << 40)
           for i, n in enumerate(("OP", "KIND", "AC", "ARG", "TXT", "REG", "FORM", "ARGREG", "CCW"))]
    words = list(dict.fromkeys(list(SHAPE) + ["r" + str(i) for i in range(8)] + [
        "0", "1", "2", "3", "4", "8", "-8", "_start:", "write", "exit",
        ".hostcall", ".hostaddr", ".librarycall", ".libraryaddr"]
        # H3d: the cc-interop inbound label definitions `__ccw_N:` (front_parse.c ccw_emit, N < 256) carry a `mark`
        # (the CCW side table, set when they are interned): the scan has no prefix primitive, and an id range
        # is not enough -- source labels are interned between these words at run time
        + ["__ccw_" + str(i) + ":" for i in range(256)]))
    from unisa import lower as L
    for n in ("WIN_HSTD", "WIN_WRITTEN", "WIN_SAVE", "WIN_ARGVA", "SYSA", "SYSFP", "SYSSP"):
        out.append("=%s\tint\t%d" % (n, getattr(L, n)))
    out.append("=WIN_STACK_END\tint\t%d" % (L.WIN_EXTRA + L.WIN_STACK))
    out += ["=SYSA%d\tint\t%d" % (i, L.SYSA + 8 * i) for i in range(6)]
    out += ["=key_argc\tjson\t" + json.dumps("\0process/argc"), "=key_argv\tjson\t" + json.dumps("\0process/argv")]
    out.append("@words\ttarget:str\ti:int\tword:str\trslots:json\treg:json\tform:json\tmark:json")
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for i, w in enumerate(words):
                out.append("\t%s/%s\t%d\t%s\t%s\t%s\t%s\t%s" % (
                    os_, arch, i, _esc(w), json.dumps([j for j, k in enumerate(SHAPE.get(w, ())) if k == "r"]),
                    json.dumps([regmap[w]] if w in regmap else []),
                    json.dumps([" form=" + enc[w]] if w in enc else []),
                    json.dumps([1] if w.startswith("__ccw_") else [])))
    # Per-target env of the code route (was exec/lower/code.py env prep): code-manifest.tsv
    # merges codeenv.<target> with bindmap.  Scratch registers are checked here, at export.
    from unisa.emit_x86 import SCR, SCR2
    from unisa.emit_arm import IP0, IP1
    from exec.facts.load import facts
    banks = {r["name"]: r["value"] for r in facts("top-modelbindings-banks")}
    stride = {r["name"]: r["value"] for r in facts("top-modelbindings-const")}["STRIDE"]
    env = {}
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            reloc = {r[0]: r[2] for r in _gold("reloc") if r[1] == arch}
            ids = {w: "idc" + str(i) for i, w in enumerate(words)}
            scratch = "x16" if arch == "arm64" else "r11"
            assert scratch not in set(regmap.values()), "callm scratch aliases tape register"
            s0, s1 = ("x" + str(IP0), "x" + str(IP1)) if regmap["r0"].startswith("x") else (SCR, SCR2)
            assert not {s0, s1} & set(regmap.values()), "process scratches alias tape values"
            e = dict(target=os_ + "/" + arch, arch=arch, ids=ids, process_flag="true" if os_ == "lnx" else "false",
                     idmapu={"id_" + n: v for n, v in ids.items()}, idmap={"id:" + n: v for n, v in ids.items()},
                     scratch=scratch, form_load64=enc["load64"], form_callr=enc["callr"],
                     s0=s0, s1=s1, hosted=True,
                     IDS=banks["IDS"], ADDRESS=banks["ADDRESS"], DESC=banks["DESC"], BKIND=banks["KIND"],
                     SUPPORTED=banks["SUPPORTED"], WRITABLE=banks["WRITABLE"], EXTENT=banks["EXTENT"],
                     STRIDE=stride)
            e.update({"reloc_" + k: v for k, v in reloc.items()})
            e.update({"reg_" + k: v for k, v in regmap.items()})
            env[os_ + "/" + arch] = e
    out.append("=codeenv\tjson\t" + json.dumps(env, sort_keys=True))
    return out



# lower-armfuse moved to tools/facts-gen.cx (facts: exec/facts/tape-shape.tsv) on 2026-10-30.


def lowerabi():
    """Lower syscall ABI per os/arch (domain data, no labels): gold abi rows (sysno, argument
    registers, return register, gate, number register, argument shape, return convention,
    Windows import), encoding form, Windows API name, and whether the target lowers the op
    (native syscall number, or a Windows API with an import)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import WINAPI
    out = ["@abiops\ttarget:str\top:str",
           ]
    rows = ["@abi\ttarget:str\top:str\tsysno:str\targs:json\tret:str\tgate:str\tnrreg:str\targshape:str\tretconv:str\twinimp:str\twinapi:str\tform:str\tselected:int"]
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            t = os_ + "/" + arch
            abi = {r[0]: r[3:] for r in _gold("abi") if r[1:3] == [os_, arch]}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            out += ["\t%s\t%s" % (t, _esc(o)) for o in dict.fromkeys([*abi, "exit_group"])]
            for o, f in abi.items():
                sel = f[0] != "none" or (os_ == "win" and WINAPI.get(o) is not None and f[12] != "none")
                rows.append("\t" + "\t".join([t, _esc(o), f[0], json.dumps(f[1:7]), f[7], f[8], f[9], f[10], f[11], f[12],
                                                 "none" if WINAPI.get(o) is None else WINAPI[o], enc.get(o, ""), str(int(sel))]))
    # (op, mode) pairs of the selected ops, flattened (decision A): argument sources per
    # exec/lower/code-abi-sources.tsv; mem arguments carry the immediates written before them.
    from unisa import lower as L
    src = [x.split("\t") for x in (ROOT / "exec/lower/code-abi-sources.tsv").read_text().splitlines()
           if x and not x.startswith("#")]
    sysa = {"SYSA%d" % i: L.SYSA + 8 * i for i in range(6)}
    sel = ["@abisel\ttarget:str\top:str"]
    om = ["@opmodes\ttarget:str\top:str\tmode:int\tprev:int\tfirst:json\tlast:json\tsyscall:json\thassysno:json\t"
          "sysnoint:str\tnrreg:str\tret:str\tgate:str\tform:str\tcarry:str\twinapi:str\tsysnoval:str\tretconv:str\t"
          "winimp:str\tmems:json\ttprev:json\ttrail:json"]
    val = lambda v: "'" + v if v == "none" or v.startswith(tuple("0123456789")) else v
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            t = os_ + "/" + arch
            abi = {r[0]: r[3:] for r in _gold("abi") if r[1:3] == [os_, arch]}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for o, f in abi.items():
                if not (f[0] != "none" or (os_ == "win" and WINAPI.get(o) is not None and f[12] != "none")):
                    continue
                sel.append("\t%s\t%s" % (t, _esc(o)))
                for mode in range(5):
                    sm = 5 if o == "syscall" and mode == 1 else mode
                    sources = [(k, sysa[v] if v in sysa else int(v)) for m, sh, k, v in src if int(m) == sm and sh in ("*", f[10])]
                    if not sources:
                        raise ValueError("new Linux " + arch + " argument shape requires migration: " + f[10])
                    mems, pend, prev = [], [], []
                    for i, (k, v) in enumerate(sources):
                        if f[1 + i] == "none":
                            if os_ == "win":
                                break
                            raise ValueError("unsupported stack syscall argument")
                        a = dict(idx=i, kind=k, value=v, reg=f[1 + i])
                        if mode == 1 and os_ != "win":
                            a["stackoffset"] = v - L.SYSA
                        if k == "imm":
                            pend.append(a)
                        else:
                            mems.append(dict(a, imms=pend, prev=prev))
                            pend, prev = [], [dict(idx=i)]
                    om.append("\t" + "\t".join(_esc(x) for x in [
                        t, o, str(mode), str(mode - 1), json.dumps([1] if mode == 0 else []), json.dumps([1] if mode == 4 else []),
                        json.dumps([1] if o == "syscall" else []), json.dumps([1] if f[0] != "none" and o != "syscall" else []),
                        str(int(f[0], 0)) if f[0] != "none" else "", f[9], f[7], f[8], enc.get(o, ""),
                        "true" if os_ == "osx" else "false", "none" if WINAPI.get(o) is None else WINAPI[o],
                        val(f[0]), val(f[11]), val(f[12]), json.dumps(mems),
                        json.dumps(prev), json.dumps(pend)]))
    private_modes = [om[0].replace("@opmodes", "@private_modes", 1)] + [
        row for row in om[1:] if row.split("\t")[3] == "1" and not row.split("\t")[1].startswith("win/")]
    return out + rows + sel + om + private_modes + ["=none\tjson\t[]", "=zero\tint\t0", "=eight\tint\t8"]


# opt-gen (E4 START data and peep indices) moved to tools/facts-gen.cx on 2026-10-30.


# top-modelbindings-template, parse-tokens and k2-tokenlocations-map moved to tools/facts-gen.cx on 2026-10-30.


# nativeabi moved to tools/facts-gen.cx (trie choice order recorded in exec/nativeabi/trie-order.tsv) on 2026-10-30.


# pp-autoinc-gen moved to tools/facts-gen.cx (closure from seed/unisa_libneed.c) on 2026-10-30.


# pp-gen moved to tools/facts-gen.cx (escapes: exec/facts/pp-escapes.tsv) on 2026-10-30.


# (fact stem, inputs whose sha prefixes head the file, producer)
# k2-gen2 moved to tools/facts-gen.cx (literals: exec/facts/gen2-constants.tsv, gen2-literals.tsv) on 2026-10-30.


# k2-gen2-tokens moved to tools/facts-gen.cx on 2026-10-30.


# k2-units-tokens moved to tools/facts-gen.cx on 2026-10-30.


TABLES = [
    ("lower-abi", ["unisa/catalog.py", "unisa/lower.py", "exec/lower/code-abi-sources.tsv", "weights/gold/abi.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowerabi),
    ("lower-code", ["unisa/tape.py", "unisa/lower.py", "weights/gold/regmap.tsv", "weights/gold/enc.tsv", "weights/gold/reloc.tsv", "unisa/emit_x86.py", "unisa/emit_arm.py", "exec/facts/top-modelbindings-banks.tsv", "exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], lowercode),
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
    if argv not in ([], ["--check"]):
        print("usage: python3 exec/facts/export.py [--check]", file=sys.stderr)
        return 2
    check = "--check" in argv
    bad = 0
    for stem, inputs, producer in TABLES:
        path = HERE / (stem + ".tsv")
        text = render(stem, inputs, producer)
        if check:
            try:
                same = path.read_text() == text
            except FileNotFoundError:
                same = False
            if not same:
                bad += 1
                print("facts differ: " + str(path.relative_to(ROOT)))
        else:
            path.write_text(text)
    if check:
        bad += _bank_owners()
    print("facts: %d tables, %d differ" % (len(TABLES), bad) if check else "facts: wrote %d tables" % len(TABLES))
    return 1 if bad else 0


# Bank ownership (was runtime asserts in layoutfacts.py / libraryexports.py): an owner's bank
# list must not be reused by any other stage's constant table (`# source: literal constants
# formerly in ...` name/value tables, the stage constants).
BANK_OWNERS = (("layoutfacts", "RESERVED_BANKS"), ("libraryexports", "RANK_BANKS"))


def _bank_owners():
    import json
    consts = {}
    for path in sorted(HERE.glob("*.tsv")):
        lines = path.read_text().split("\n")
        if len(lines) < 2 or not lines[1].startswith("# source: literal constants formerly in"):
            continue
        consts[path.stem] = {}
        for ln in lines[2:]:
            f = ln.split("\t")
            if len(f) == 2 and not ln.startswith("#"):
                consts[path.stem][f[0]] = json.loads(f[1]) if f[1][:1] in "[-0123456789" else f[1]
    bad = 0
    for owner, name in BANK_OWNERS:
        owned = set(consts[owner][name])
        for stem, d in consts.items():
            hit = sorted(k for k, v in d.items() if stem != owner and type(v) is int and v in owned)
            if hit:
                bad += 1
                print("bank owned by %s.%s reused in %s: %s" % (owner, name, stem, ", ".join(hit)))
    return bad


def _fieldchain():
    """Field-chain recorder for image headers: domain items only.  Each field row is
    (width, value int|register name, lead = the literal-byte / computation item
    names that precede it); `tail` is the lead after the last field."""
    class C:
        def __init__(self):
            self.fields, self.lead = [], []
        def lit(self, bs):
            self.lead += ["byte%d" % b for b in bs]
        def comp(self, name):
            self.lead.append(name)
        def field(self, w, v, endian="little"):
            self.fields.append(dict(width=w, value=v, src="const" if isinstance(v, int) else "reg",
                                  endian=endian, lead=self.lead))
            self.lead = []
    return C()


def _rows(name, rows):
    import json
    out = ["@%s\ti:int\twidth:int\tsrc:str\tvalue:json\tendian:str\tlead:json" % name]
    for i, r in enumerate(rows):
        out.append("\t%d\t%d\t%s\t%s\t%s\t%s" % (i, r["width"], r["src"], json.dumps(r["value"]), r["endian"],
                                                json.dumps(r["lead"], separators=(",", ":"))))
    return out


# enc-pe and enc-pe-forward moved to tools/facts-gen.cx (facts: exec/facts/image-pe.tsv) on 2026-10-30.


# enc-macho and enc-elf moved to tools/facts-gen.cx (facts: exec/facts/image-macho.tsv, image-elf.tsv) on 2026-10-30.


# enc-x86 and enc-arm moved to tools/facts-gen.cx (literals: exec/facts/enc-literals.tsv) on 2026-10-30.

# k2-strings and k2-unitlocations moved to tools/facts-gen.cx on 2026-10-30.


# k2-membercontrol moved to tools/facts-gen.cx on 2026-10-30.


# k2-truth, k2-stack and k2-librarymodule-map moved to tools/facts-gen.cx on 2026-10-30.



# k2-libraryenv moved to tools/facts-gen.cx on 2026-10-30.

# signature-layout moved to tools/facts-gen.cx on 2026-10-30.

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
