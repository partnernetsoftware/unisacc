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
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def _module(rel, name):
    spec = importlib.util.spec_from_file_location(name, ROOT / rel)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def _ppsrc():
    """E2 source facts (was the module level of exec/pp/gen.py): layout names, byte classes, targets,
    the gold pp.tsv directive vocabulary, predefines.tsv, operators.tsv, autoinc trigger names."""
    import os, re
    from types import SimpleNamespace
    sys.path.insert(0, str(ROOT))
    from unisa.tsvgold import load_table
    from exec.facts.load import facts
    E = SimpleNamespace(**{r["name"]: r["value"] for r in facts("pp-layout")})
    E.TARGETS = tuple(r["os"] + "/" + r["arch"] for r in facts("pp-targets"))
    by = {r["class"]: {c for a, b in r["ranges"] for c in range(a, b + 1)} for r in facts("pp-bytes")}
    E.ID = by["alpha"] | by["digit"]
    name, fields, heads, rows = load_table(str(ROOT / "weights/gold/pp.tsv"))
    assert name == "pp" and [n for n, _ in fields] == ["dir", "defined"]
    assert fields[1][1] == ("0", "1") and [n for n, _, _ in heads] == ["y"]
    assert list(heads[0][1]) == ["take", "skip", "pop", "macro"]
    E.DIRV = fields[0][1]
    E.PPT = {(d, int(b)): labels["y"] for (d, b), labels in rows.items()}
    path = ROOT / "exec/pp/predefines.tsv"
    pre = {}
    for line, text in enumerate(path.read_text(encoding="utf-8").splitlines(True), 1):
        if text.startswith("#") or not text.strip():
            continue
        f = text.rstrip("\n").split("\t")
        key, names = tuple(f[:2]), f[2:]
        if (len(f) < 3 or key in pre or len(set(names)) != len(names) or
                any(not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", n) for n in names)):
            raise ValueError("%s:%d: invalid or duplicate predefinition row" % (path, line))
        pre[key] = names
    if set(pre) != {("os", x) for x in ("lnx", "osx", "win")} | {("arch", x) for x in ("x86_64", "arm64")} | {("common", "*")}:
        raise ValueError("%s: expected OS, architecture and common declarations" % path)
    E.PREDEF = pre
    E.XOPS = {}
    for line in (ROOT / "exec/pp/operators.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        code, spelling, precedence, arity = line.split("\t")
        code, precedence, arity = int(code), int(precedence), int(arity)
        assert code not in E.XOPS and 0 < code < 257 and precedence >= 0 and arity in (0, 1, 2, 3)
        assert spelling and spelling not in {v[0] for v in E.XOPS.values()}
        E.XOPS[code] = spelling, precedence, arity
    assert E.XOPS, "empty expression operator declarations"
    E.AUTOINC_ORDER = tuple(facts("pp-autoinc"))
    def autoinc_map():   # src/front_pp.c hdrneeded's line rule over include/*.h
        m = {}
        for h in E.AUTOINC_ORDER:
            names = []
            for ln in (ROOT / "include" / h).read_text(encoding="latin-1").split("\n"):
                if len(ln) > 7 and ln.startswith("static") and "(" in ln and "{" in ln[ln.index("("):]:
                    mm = re.search(r"([A-Za-z0-9_]+)\s*$", ln[:ln.index("(")])
                    if mm:
                        names.append(mm.group(1))
            m[h] = names
        return m
    E.autoinc_map = autoinc_map
    E.sbconst = lambda s: [("SBCLR",)] + [("SBOUT", c) for c in s.encode()]
    def xe_init():
        a = []
        for c, (_, p, _) in list(E.XOPS.items()) + [(0, (None, -1, 0))]:
            a += [("LDI", "xc", E.XPRB + c), ("LDI", "xq", p), ("STX", "xc", 0, "xq")]
        return a
    E.xe_init = xe_init
    return E


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


def structreturnexpr():
    E = _module("exec/parse/gen.py", "exec_parse_gen_facts")
    reason = "not covered: struct return expression outside local lvalue"
    return ["=reason\tstr\t" + _esc(reason),
            "@sr\tentry:str\tloc:int\treason:str",
            "\tLI.original.structreturn\t%d\t%s" % (E.LOC, _esc(reason))]



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
           for i, n in enumerate(("OP", "KIND", "AC", "ARG", "TXT", "REG", "FORM", "ARGREG"))]
    words = list(dict.fromkeys(list(SHAPE) + ["r" + str(i) for i in range(8)] + [
        "0", "1", "2", "3", "4", "8", "-8", "_start:", "write", "exit",
        ".hostcall", ".hostaddr", ".librarycall", ".libraryaddr"]))
    from unisa import lower as L
    for n in ("WIN_HSTD", "WIN_WRITTEN", "WIN_SAVE", "WIN_ARGVA", "SYSA", "SYSFP", "SYSSP"):
        out.append("=%s\tint\t%d" % (n, getattr(L, n)))
    out.append("=WIN_STACK_END\tint\t%d" % (L.WIN_EXTRA + L.WIN_STACK))
    out += ["=SYSA%d\tint\t%d" % (i, L.SYSA + 8 * i) for i in range(6)]
    out += ["=key_argc\tjson\t" + json.dumps("\0process/argc"), "=key_argv\tjson\t" + json.dumps("\0process/argv")]
    out.append("@words\ttarget:str\ti:int\tword:str\trslots:json\treg:json\tform:json")
    for os_ in ("lnx", "osx", "win"):
        for arch in ("x86_64", "arm64"):
            regmap = {r[0]: r[2] for r in _gold("regmap") if r[1] == arch}
            enc = {r[0]: r[3] for r in _gold("enc") if r[1:3] == [os_, arch]}
            for i, w in enumerate(words):
                out.append("\t%s/%s\t%d\t%s\t%s\t%s\t%s" % (
                    os_, arch, i, _esc(w), json.dumps([j for j, k in enumerate(SHAPE.get(w, ())) if k == "r"]),
                    json.dumps([regmap[w]] if w in regmap else []),
                    json.dumps([" form=" + enc[w]] if w in enc else [])))
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



def lowerarmfuse():
    """ARM immediate fusion: TAPE SHAPE ops eligible under exec/lower/armfuse-shapes.tsv
    (shape / exclude / read policy), with the operand positions read (was armfuse.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.tape import SHAPE
    pol = [x.split("\t") for x in (ROOT / "exec/lower/armfuse-shapes.tsv").read_text().splitlines() if x and not x.startswith("#")]
    shapes = {tuple(v.split(",")) for k, v in pol if k == "shape"}
    excl = {v for k, v in pol if k == "exclude"}
    reads = {v for k, v in pol if k == "read"}
    out = ["=none\tjson\t[]"]
    out += ["@shapes\ti:int\top:str\treads:json\tfirst:json\trest:json"]
    sel = [(o, sh) for o, sh in SHAPE.items() if sh in shapes and o not in excl]
    for i, (o, sh) in enumerate(sel):
        out.append("\t%d\t%s\t%s\t%s\t%s" % (i, _esc(o), json.dumps([j for j, k in enumerate(sh[1:], 1) if k in reads]),
                                             json.dumps([1] if i == 0 else []), json.dumps([] if i == 0 else [1])))
    return out



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
    return out + rows + sel + om + ["=none\tjson\t[]", "=zero\tint\t0", "=eight\tint\t8"]


def optgen():
    """E4 optimiser START data and peep index facts (was exec/opt/gen.py build/peep_start/peepround):
    start1/start2 = the START action list per level (interned start words, -O2 peep table and
    opinfo classes, opinfo simple set), peepidx = PA_/PB_/PR_ column indices and sizes, Y = one
    answer row per peep head with its target template (answer-targets.tsv)."""
    import json
    sys.path.insert(0, str(ROOT))
    from exec.facts.load import facts
    C = {r["name"]: r["value"] for r in facts("opt-gen-constants")}
    pf = {}
    for ln in (ROOT / "weights/gold/peep.tsv").read_text().splitlines():
        f = ln.split("\t")
        if ln.startswith("#field") or ln.startswith("#head"):
            pf[f[1]] = f[2:]
    PA, PB, PR, PY = pf["a"], pf["b"], pf["rel"], pf["y"]
    def gold(name):   # field-wise `=>` filter, as exec/parse/gen.py gold()
        return [f for f in (ln.split("\t") for ln in (ROOT / "weights/gold" / (name + ".tsv")).read_text(encoding="utf-8").splitlines()
                            if not ln.startswith("#")) if "=>" not in f]
    peep, opinfo = gold("peep"), gold("opinfo")
    def word(w, reg):
        return [["SBCLR"]] + [["SBOUT", c] for c in w.encode()] + [["SBINTERN", reg]]
    out = []
    for level in (1, 2):
        acts = []
        for w in facts("opt-gen-startwords"):
            acts += word(w, "id_" + w.strip("."))
        if level >= 2:
            for f in peep:
                if len(f) == 4 and f[0] in PA and f[1] in PB and f[2] in PR and f[3] in PY:
                    k = (PA.index(f[0]) * len(PB) + PB.index(f[1])) * len(PR) + PR.index(f[2])
                    acts += [["LDI", "q_t", k], ["LDI", "q_u", PY.index(f[3]) + 1], ["STX", "q_t", C["PEEPB"], "q_u"]]
            for f in opinfo:
                if len(f) == 4:
                    acts += word(f[0], "q_t")
                    if f[2] in PA:
                        acts += [["LDI", "q_u", PA.index(f[2]) + 1], ["STX", "q_t", C["ACLSB"], "q_u"]]
                    if f[3] in PB:
                        acts += [["LDI", "q_u", PB.index(f[3]) + 1], ["STX", "q_t", C["BCLSB"], "q_u"]]
        for f in opinfo:
            if len(f) >= 2 and f[1] == "1":
                acts += word(f[0], "t") + [["LDI", "u", 1], ["STX", "t", C["SIMPLE"], "u"]]
        out.append("=start%d\tjson\t%s" % (level, json.dumps(acts)))
    idx = {}
    for prefix, values in (("PA", PA), ("PB", PB), ("PR", PR)):
        idx.update((prefix + "_" + n, i) for i, n in enumerate(values))
    idx.update(PB_size=len(PB), PR_size=len(PR))
    out.append("=peepidx\tjson\t" + json.dumps(idx))
    targets = dict(l.split("\t") for l in (ROOT / "exec/opt/answer-targets.tsv").read_text().splitlines()
                   if l and not l.startswith("#"))
    out.append("=Y\tjson\t" + json.dumps([dict(i=i, target=targets.get(n, targets["*"]).format(name=n)) for i, n in enumerate(PY)]))
    return out


def modelbindingstemplate():
    """USBIND magic chain, identifier byte classes, record kinds (from top-modelbindings-const)."""
    sys.path.insert(0, str(HERE))
    from load import facts
    c = {r["name"]: r["value"] for r in facts("top-modelbindings-const")}
    out = ["@magic\ti:int\tc:int\tnext:int"]
    out += ["\t%d\t%d\t%d" % (i, ch, i + 1) for i, ch in enumerate(c["magic"].encode())]
    L, D = list(c["letters"].encode()), list(c["digits"].encode())
    out += ["@id\tletters:json\tdigits:json\tall:json", "\t%s\t%s\t%s" % tuple(json.dumps(x) for x in (L, D, L + D))]
    out += ["=kind\tjson\t" + json.dumps(c["kind"])]
    return out


_NATIVE_TRIE = r"""
import csv, json, pathlib, sys
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
rows = list(csv.DictReader(lines[:7], delimiter="\t"))
profiles = {r["profile"] for r in rows}
prefixes = {b""}
for profile in profiles:
    raw = profile.encode(); prefixes.update(raw[:i] for i in range(1, len(raw) + 1))
names = {p: "NC.profile" + ("" if not p else "." + p.hex()) for p in prefixes}
T = []
for prefix in sorted(prefixes):
    if prefix.decode() in profiles:
        r = next(r for r in rows if r["profile"] == prefix.decode())
        T.append(dict(name=names[prefix], leaf=[dict(im=int(r["integer_min_width"]), fp=int(r["homogeneous_fp_carrier_kind"]),
                 ld=int(r["long_double_format"]), family=int(r["family"]))], inner=[]))
    else:
        T.append(dict(name=names[prefix], leaf=[], inner=[dict(choices=[dict(byte=p[len(prefix)], target=names[p])
                 for p in prefixes if len(p) == len(prefix) + 1 and p.startswith(prefix)])]))
print(json.dumps(T))
"""


def nativeabi():
    """FFI carrier facts from exec/nativeabi/rules.tsv (was nativeabi/gen.py + ordered.py):
    T = profile byte trie (state order = sorted prefixes; choice order = the original
    PYTHONHASHSEED=0 iteration order of the prefix set, recorded by a seeded child), A = union16
    recipes, extents/alignments classes, ordseq = the ordered `@` field/extra read sequences."""
    import csv, json, os, subprocess
    sys.path.insert(0, str(ROOT))
    from exec.facts.load import facts
    rules = ROOT / "exec/nativeabi/rules.tsv"
    lines = rules.read_text().splitlines()
    rows = list(csv.DictReader(lines[:7], delimiter="\t"))
    decl = [x.split("\t") for x in lines[7:] if not x.startswith("#")]
    leaf = {int(x[1]): "NSI".index(x[2]) for x in decl if x[0] == "leaf"}
    joins = {(x[1], x[2]): x[3] for x in decl if x[0] == "merge"}
    assert all(joins[a, b] == "NSI"[max("NSI".index(a), "NSI".index(b))] for a in "NSI" for b in "NSI")
    recipes = {(int(x[1]), x[2]): x[3] for x in decl if x[0] == "recipe"}
    assert len(recipes) == 8 and leaf == {1: 2, 2: 2, 3: 1}
    assert {r["profile"] for r in rows} == {f"{o}/{a}" for o in ("osx", "lnx", "win") for a in ("arm64", "x86_64")} and len(rows) == 6
    assert all(r["rule"] == "natural_scalar_union_v2" and int(r["mixed_width"]) == 8 for r in rows)
    assert all(int(r["integer_min_width"]) in (1, 8) and int(r["homogeneous_fp_carrier_kind"]) in (1, 3) for r in rows)
    assert {int(r["long_double_format"]) for r in rows} <= {2, 3, 4}
    T = subprocess.run([sys.executable, "-c", _NATIVE_TRIE, str(rules)], env=dict(os.environ, PYTHONHASHSEED="0"),
                       capture_output=True, check=True, text=True).stdout.strip()
    A = [dict(al=al, recipes=[dict(cls=c, code=3 * "NSI".index(c[0]) + "NSI".index(c[1]), n=len(recipes[al, c]),
         elems=[dict(off=i * al, kind=1 if k == "I" else 3, integer=int(k == "I")) for i, k in enumerate(recipes[al, c])])
         for c in ("II", "IS", "SI", "SS")]) for al in (4, 8)]
    ext = [int(x[1]) for x in decl if x[0] == "ordered_extent"]
    ali = [int(x[1]) for x in decl if x[0] == "ordered_alignment"]
    assert ext == list(range(1, 17)) and ali == [1, 2, 4, 8]
    assert [x[1] for x in decl if x[0] == "ordered_anon_policy"] == ["invariant_integer_lanes"]
    geq = {r["name"]: r["value"] for r in facts("top-modelgraphequality-banks")}
    seq = {}
    for ln in (ROOT / "exec/nativeabi/ordered-result.tsv").read_text().splitlines()[1:]:
        for a in json.loads(ln.split("\t")[4]):
            if a[0] == "@" and a[1] not in seq:
                kind, index, reg, node = a[1].split()
                if kind == "field":
                    seq[a[1]] = [["ALUI", "mul", "nc_key", node, 16], ["ALUI", "add", "nc_key", "nc_key", int(index)], ["LDX", reg, "nc_key", geq["FIELDS"]]]
                else:
                    seq[a[1]] = [["ALUI", "mul", "nc_key", node, 8], ["ALUI", "add", "nc_key", "nc_key", int(index)], ["LDX", reg, "nc_key", geq["EXTRA"]]]
    gf = [dict(section=a, name=b, prefix=c, kind=d) for a, b, c, d in (l.split("\t") for l in (ROOT / "exec/nativeabi/gen-fresh.tsv").read_text().splitlines()[1:])]
    of = [dict(section=a, name=b, kind=c) for a, b, c in (l.split("\t") for l in (ROOT / "exec/nativeabi/ordered-fresh.tsv").read_text().splitlines()[1:])]
    return ["=T\tjson\t" + T, "=A\tjson\t" + json.dumps(A), "=extents\tjson\t" + json.dumps(ext),
            "=alignments\tjson\t" + json.dumps(ali), "=ordseq\tjson\t" + json.dumps(seq),
            "=genfresh\tjson\t" + json.dumps(gf), "=ordfresh\tjson\t" + json.dumps(of),
            "=reject\tjson\t" + json.dumps(facts("nativeabi-gen-reject"))]


def ppautoinc():
    """E2 autoinc + -ftrim-libc chains (was exec/pp/gen.py build_autoinc/build_ftrim_libc): the
    libneed closure table (unisa/libneed.py over include/), autoinc header names (hdrneeded's line
    rule), as rows with their chain labels and constant-spelling action lists."""
    import json
    E = _ppsrc()
    from unisa.libneed import table, roots, PREFIX
    inc = str(ROOT / "include")
    keys, closure, bodies = table(inc)
    index = {b: i for i, b in enumerate(bodies)}
    def marks(names):
        out = []
        for b in names:
            out += [("LDI", "t", E.NEEDB + index[b]), ("LDI", "v", 1), ("STX", "t", 0, "v")]
        return out
    K = [dict(entry="LNK%d" % i, test="LNK%dr" % i, next="LNK%d" % (i + 1) if i + 1 < len(keys) else "LNB0",
              name=E.sbconst(k), mark=marks(closure[k])) for i, k in enumerate(keys)]
    B = [dict(entry="LNB%d" % j, test="LNB%dr" % j, next="LNB%d" % (j + 1) if j + 1 < len(bodies) else "LNDEF",
              define="LNB%dd" % j, resume="LNB%ddr" % j, slot=E.NEEDB + j, name=E.sbconst(PREFIX + b)) for j, b in enumerate(bodies)]
    amap, H = E.autoinc_map(), list(E.AUTOINC_ORDER)
    HD = []
    for h, hn in enumerate(H):
        names = [n for n in amap[hn] if n != "printf"]
        nxt = "AH%d_0" % (h + 1) if h + 1 < len(H) else "AEM"
        HD.append(dict(entry="AH%d_0" % h, first="AH%d_n0" % h, last="AH%d_n%d" % (h, len(names)), next=nxt, need="NEED%d" % h,
                       names=[dict(entry="AH%d_n%d" % (h, k), test="AH%d_r%d" % (h, k), found=nxt, next="AH%d_n%d" % (h, k + 1),
                                   need="NEED%d" % h, name=E.sbconst(nm)) for k, nm in enumerate(names)]))
    def line(hn):
        return [("OUT", c) for c in ("#include <%s>\n" % hn).encode()] + [("ALUI", "add", "AI_LINES", "AI_LINES", 1)]
    EM = [dict(entry="AEM", test="AEMR", need="RTP", next="AEM%d" % (len(H) - 1), line=line("stdio.h"))]
    EM += [dict(entry="AEM%d" % h, test="AEM%dr" % h, need="NEED%d" % h, next="AEM%d" % (h - 1) if h else "ACP0", line=line(H[h]))
           for h in range(len(H) - 1, -1, -1)]
    return ["=idclass\tjson\t" + json.dumps(sorted(E.ID)), "=roots\tjson\t" + json.dumps(marks(roots(inc))),
            "=keys\tjson\t" + json.dumps(K), "=bodies\tjson\t" + json.dumps(B),
            "=lndef\tjson\t" + json.dumps(E.sbconst("__UNISA_FTRIM_LIBC")),
            "=headers\tjson\t" + json.dumps(HD), "=emits\tjson\t" + json.dumps(EM)]


def ppgen():
    """E2 delta facts (was exec/pp/gen.py build/build_xe): START init actions (directive ids from
    weights/gold/pp.tsv, pp-init spellings, XE precedences), per-target predefine chains, the DSW
    directive switch, directive-action instances (gold pp.tsv answers), simple escapes, PREC_* layout."""
    import json
    E = _ppsrc()
    from exec.facts.load import facts
    init = []
    for k, w in enumerate(E.DIRV):
        init += E.sbconst(w) + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", E.DIRB), ("LDI", "v", k + 1), ("STX", "a", 0, "v")]
    for r in facts("pp-init"):
        if r["kind"] == "dir":
            init += E.sbconst(r["word"]) + [("SBINTERN", "t"), ("ALUI", "add", "a", "t", E.DIRB), ("LDI", "v", r["arg"]), ("STX", "a", 0, "v")]
        else:
            init += E.sbconst(r["word"]) + [("SBINTERN", r["arg"])]
    init += [("LDI", "RUN", 0), ("LDI", "FP", 0)] + E.xe_init()
    predef = {}
    for t in E.TARGETS:
        o, a = t.split("/")
        names = E.PREDEF["os", o] + E.PREDEF["arch", a] + E.PREDEF["common", "*"]
        assert len(set(names)) == len(names), "overlapping target predefinitions: " + t
        predef[t] = [dict(entry="P3PD%d" % k, resume="P3PDR%d" % k, next="P3PD%d" % (k + 1) if k + 1 < len(names) else "OOBJ.start",
                          name=E.sbconst(nm)) for k, nm in enumerate(names)]
    cases = [dict(key=0, target="P3BLANK", acts=[["JUMP", "LS"]]), dict(key=100, target="PRAG", acts=[["RLD", "LIVE"]]),
             dict(key=101, target="LDIR", acts=[["RLD", "LIVE"]]), dict(key=102, target="D_ERROR", acts=[["RLD", "LIVE"]])]
    cases += [dict(key=k + 1, target="D_" + w, acts=[]) for k, w in enumerate(E.DIRV)]
    for c in cases:
        c["acts"] = json.dumps(c["acts"])   # spliced verbatim into the dsw template's JSON actions
    acts = [dict(name="D_%s_a%d" % (w, fl), section=w + "/" + E.PPT[(w, fl)]) for w in E.DIRV for fl in (0, 1)]
    from unisa.front.lex import ESC
    esc = [{"code": ord(ch), "value": ord(v)} for ch, v in ESC.items() if ch not in "01234567x"]
    prec = {"PREC_" + str(c): p for c, (_, p, _) in E.XOPS.items()}
    prec["XOB_PREV"] = E.XOB - 1
    return ["=init\tjson\t" + json.dumps(init), "=predef\tjson\t" + json.dumps(predef),
            "=cases\tjson\t" + json.dumps(cases), "=dswkeys\tjson\t" + json.dumps(sorted(c["key"] for c in cases)), "=dactions\tjson\t" + json.dumps(acts),
            "=esc\tjson\t" + json.dumps(esc), "=esckeys\tjson\t" + json.dumps([e["code"] for e in esc]),
            "=xelayout\tjson\t" + json.dumps(prec), "=objname\tjson\t" + json.dumps(E.sbconst("__UNISA_OBJECT")),
            "=location_line\tjson\t" + json.dumps([["ALUI", "add", "CLI_PRELINES", "CLI_PRELINES", 1]]),
            "=predefres\tjson\t" + json.dumps({t: "".join(n + "\0" for n in E.PREDEF["os", t.split("/")[0]] + E.PREDEF["arch", t.split("/")[1]] + E.PREDEF["common", "*"])
                                                 for t in sorted(E.TARGETS)})]


# (fact stem, inputs whose sha prefixes head the file, producer)
TABLES = [
    ("pp-gen", ["exec/facts/pp-targets.tsv", "exec/facts/pp-bytes.tsv", "exec/facts/pp-autoinc.tsv", "exec/pp/operators.tsv", "exec/pp/predefines.tsv", "weights/gold/pp.tsv", "exec/facts/pp-init.tsv", "exec/facts/pp-layout.tsv", "unisa/front/lex.py", "exec/facts/export.py"], ppgen),
    ("pp-autoinc-gen", ["exec/facts/pp-bytes.tsv", "unisa/libneed.py", "exec/facts/pp-autoinc.tsv", "exec/facts/pp-layout.tsv", "exec/facts/export.py"], ppautoinc),
    ("nativeabi", ["exec/nativeabi/rules.tsv", "exec/nativeabi/ordered-result.tsv", "exec/nativeabi/gen-fresh.tsv", "exec/nativeabi/ordered-fresh.tsv", "exec/facts/nativeabi-gen-reject.tsv", "exec/facts/top-modelgraphequality-banks.tsv", "exec/facts/export.py"], nativeabi),
    ("opt-gen", ["weights/gold/peep.tsv", "weights/gold/opinfo.tsv", "exec/facts/opt-gen-constants.tsv", "exec/facts/opt-gen-startwords.tsv", "exec/opt/answer-targets.tsv", "exec/facts/export.py"], optgen),
    ("top-modelbindings-template", ["exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], modelbindingstemplate),
    ("structreturnexpr", ["exec/parse/gen.py", "exec/facts/export.py"], structreturnexpr),
    ("lower-armfuse", ["unisa/tape.py", "exec/lower/armfuse-shapes.tsv", "exec/facts/export.py"], lowerarmfuse),
    ("lower-abi", ["unisa/catalog.py", "unisa/lower.py", "exec/lower/code-abi-sources.tsv", "weights/gold/abi.tsv", "weights/gold/enc.tsv", "exec/facts/export.py"], lowerabi),
    ("lower-code", ["unisa/tape.py", "unisa/lower.py", "weights/gold/regmap.tsv", "weights/gold/enc.tsv", "weights/gold/reloc.tsv", "unisa/emit_x86.py", "unisa/emit_arm.py", "exec/facts/top-modelbindings-banks.tsv", "exec/facts/top-modelbindings-const.tsv", "exec/facts/export.py"], lowercode),
]


def render(stem, inputs, producer):
    head = ["# exec/facts/%s.tsv: written by exec/facts/export.py, do not edit" % stem]
    head += ["# input %s sha256:%s" % (p, hashlib.sha256((ROOT / p).read_bytes()).hexdigest()[:16])
             for p in inputs]
    return "\n".join(head + producer()) + "\n"


def main(argv):
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
    print("facts: %d tables, %d differ" % (len(TABLES), bad) if check else "facts: wrote %d tables" % len(TABLES))
    return 1 if bad else 0


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


def pefields():
    """PE32+ header and import-directory field schema (unisa/image/pe.py)."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.image import pe
    n = len(pe.IMPORTS); iat = 40 + 8 * (n + 1); off = iat + 8 * (n + 1)
    names = []
    for name in pe.IMPORTS:
        raw = b"\0\0" + name.encode() + b"\0"; raw += b"\0" * (len(raw) % 2)
        names.append((off, raw)); off += len(raw)
    dlloff = off; off += len(pe.DLL) + 1; cfg = (off + 7) & -8; idlen = cfg + pe.LOADCFG
    consts = dict(DATA=1 << 40, RELOCS=4 << 40, SORTED=5 << 40, TEXT_RVA=pe.TEXT_RVA, IMAGEBASE=pe.IMAGEBASE,
                  HDR_FILE=pe.HDR_FILE, section_minus_one=pe.SECT_ALIGN - 1, section_mask=-pe.SECT_ALIGN,
                  file_minus_one=pe.FILE_ALIGN - 1, file_mask=-pe.FILE_ALIGN,
                  import_vsize=(idlen + pe.SECT_ALIGN - 1) & -pe.SECT_ALIGN,
                  import_fsize=(idlen + pe.FILE_ALIGN - 1) & -pe.FILE_ALIGN,
                  cookie_at=cfg + pe.COOKIE_FIELD, cfg=cfg, iat=iat, dlloff=dlloff)
    out = ["=%s\tint\t%d" % kv for kv in consts.items()]
    used = set()
    for arch in sorted(pe.MACHINE):
        c = _fieldchain()
        c.lit(b"MZ" + bytes(58)); c.field(4, 64); c.lit(b"PE\0\0")
        for w, v in [(2, pe.MACHINE[arch]), (2, 4), (4, 0), (4, 0), (4, 0), (2, 240), (2, 0x22)]: c.field(w, v)
        c.comp("entry-value")
        for w, v in [(2, 0x20B), (1, 14), (1, 0), (4, "pe_tf"), (4, 0), (4, 0), (4, "pe_entry"), (4, pe.TEXT_RVA), (8, pe.IMAGEBASE), (4, pe.SECT_ALIGN), (4, pe.FILE_ALIGN),
                     (2, 4), (2, 0), (2, 0), (2, 0), (2, 4), (2, 0), (4, 0), (4, "pe_img"), (4, pe.HDR_FILE), (4, 0), (2, 3), (2, 0x8160), (8, 0x100000), (8, 0x1000), (8, 0x100000), (8, 0x1000), (4, 0), (4, 16)]:
            c.field(w, v)
        c.comp("directory-values")
        dirs = {1: ("pe_rd", 40), 5: ("pe_rr", "pe_rlsize"), 10: ("pe_cfg", pe.LOADCFG), 12: ("pe_iat", (n + 1) * 8)}
        for i in range(16):
            c.field(4, dirs.get(i, (0, 0))[0]); c.field(4, dirs.get(i, (0, 0))[1])
        for name, rva, vs, fo, fs, flags in [(b".text", pe.TEXT_RVA, "endo", pe.HDR_FILE, "pe_tf", 0x60000020), (b".rdata", "pe_rd", idlen, "pe_rf", (idlen + 511) & -512, 0x40000040),
                                             (b".data", "pe_dt", "pe_dvs", "pe_df", "pe_datafile", 0xC0000040), (b".reloc", "pe_rr", "pe_rlsize", "pe_relf", None, 0x42000040)]:
            if fs is None: c.comp("reloc-align"); fs = "pe_rsize"
            c.lit(name.ljust(8, b"\0"))
            for w, v in [(4, vs), (4, rva), (4, fs), (4, fo), (4, 0), (4, 0), (2, 0), (2, 0), (4, flags)]: c.field(w, v)
        out += _rows("header_" + arch, c.fields) + _rows("header_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain()
    c.comp("import-values")
    for w, v in [(4, "pe_int"), (4, 0), (4, 0), (4, "pe_dll"), (4, "pe_iat")]: c.field(w, v)
    c.lit(bytes(20))
    for _ in range(2):
        for offset, raw in names:
            c.comp("name-value%d" % offset); c.field(8, "pe_name")
        c.field(8, 0)
    for offset, raw in names: c.lit(raw)
    c.lit(pe.DLL + b"\0" + bytes(cfg - (dlloff + len(pe.DLL) + 1)))
    c.field(4, pe.LOADCFG); c.lit(bytes(pe.COOKIE_FIELD - 4)); c.field(8, "pe_cookie"); c.lit(bytes(pe.LOADCFG - pe.COOKIE_FIELD - 8))
    out += _rows("imports", c.fields) + _rows("imports_tail", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
    used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    out += ["@names\toffset:int"] + ["\t%d" % o for o, _ in names]
    return out


TABLES.append(("enc-pe", ["unisa/image/pe.py", "exec/facts/export.py"], pefields))


def machofields():
    """Mach-O load commands and ad-hoc code-signature field schema (unisa/image/macho.py)."""
    sys.path.insert(0, str(ROOT))
    from unisa.image import macho as M
    out = ["=%s\tint\t%d" % kv for kv in dict(DATA=1 << 40, VMADDR=M.VMADDR, STRTAB=M.STRTAB, page_minus_one=M.PAGE - 1,
                                               page_mask=-M.PAGE, page4_minus_one=M.PAGE4 - 1, PAGE4=M.PAGE4,
                                               signature_base=20 + 88 + len(M.IDENT), DLPREFIX=M.DLPREFIX,
                                               DLBINDLEN=len(M.DLBIND)).items()]
    used = set()
    for arch in sorted(M.CPU):
        out.append("=HDRS_%s\tint\t%d" % (arch, M.HDRS(arch)))
        c = _fieldchain()
        def fields(items, big=False):
            for w, v in items: c.field(w, v, "big" if big else "little")
        def name(v): c.lit(v.ljust(16, b"\0"))
        def seg(n, va, vm, fo, fs, prot, ns):
            fields([(4, M.LC_SEGMENT_64), (4, M.SEG + M.SECT * ns)]); name(n)
            fields([(8, va), (8, vm), (8, fo), (8, fs), (4, prot), (4, prot), (4, ns), (4, 0)])
        def sect(n, s, va, sz, off, flags):
            name(n); name(s); fields([(8, va), (8, sz), (4, off), (4, 2), (4, 0), (4, 0), (4, flags), (4, 0), (4, 0), (4, 0)])
        fields([(4, 0xFEEDFACF), (4, M.CPU[arch][0]), (4, M.CPU[arch][1]), (4, 2), (4, M.NCMDS), (4, M._cmdsz(arch)), (4, 0x200085), (4, 0)])
        seg(b"__PAGEZERO", 0, M.VMADDR, 0, 0, 0, 0)
        seg(b"__TEXT", M.VMADDR, "mh_text", 0, "mh_text", 5, 1)
        sect(b"__text", b"__TEXT", M.VMADDR + M.HDRS(arch), "endo", M.HDRS(arch), 0x80000400)
        seg(b"__DATA", "mh_datava", "mh_vm", "mh_text", "mh_data", 3, 2)
        sect(b"__data", b"__DATA", "mh_datava", "mh_stored", "mh_text", 0)
        sect(b"__bss", b"__DATA", "mh_bssva", "mh_bsslen", 0, 1)
        seg(b"__LINKEDIT", "mh_linkva", "mh_linkvm", "mh_link", "mh_linksz", 1, 0)
        fields([(4, M.LC_LOAD_DYLINKER), (4, 32), (4, 12)])
        c.lit(M.DYLD.ljust(20, b"\0"))
        lib = (M.LIBSYS + b"\0"); lib += bytes((-len(lib)) % 8)
        fields([(4, M.LC_LOAD_DYLIB), (4, 24 + len(lib)), (4, 24), (4, 0), (4, 0x10000), (4, 0x10000)])
        c.lit(lib)
        fields([(4, M.LC_MAIN), (4, 24), (8, "mh_entry"), (8, 0), (4, M.LC_BUILD_VERSION), (4, 24), (4, 1), (4, 13 << 16), (4, 13 << 16), (4, 0)])
        fields([(4, M.LC_DYLD_INFO_ONLY), (4, 48), (4, 0), (4, 0), (4, "mh_bindoff"), (4, len(M.DLBIND))] + [(4, 0)] * 6)
        fields([(4, M.LC_SYMTAB), (4, 24), (4, "mh_link"), (4, 0), (4, "mh_link"), (4, M.STRTAB)])
        fields([(4, M.LC_DYSYMTAB), (4, 80)] + [(4, 0)] * 18)
        fields([(4, M.LC_CODE_SIGNATURE), (4, 16), (4, "mh_sigoff"), (4, "mh_siglen")])
        c.lit(bytes(M.SLACK))
        out += _rows("header_" + arch, c.fields) + _rows("header_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
        c = _fieldchain()
        fields([(4, M.CS_MAGIC_EMBEDDED), (4, "mh_siglen"), (4, 1), (4, 0), (4, 20)], True)
        c.comp("signature-length")
        fields([(4, M.CS_MAGIC_CODEDIRECTORY), (4, "mh_cdlen"), (4, 0x20400), (4, M.CS_ADHOC), (4, 88 + len(M.IDENT)), (4, 88), (4, 0), (4, "mh_slots"), (4, "mh_sigoff"), (1, 32), (1, 2), (1, 0), (1, 12), (4, 0),
                (4, 0), (4, 0), (4, 0), (8, 0), (8, 0), (8, "mh_text"), (8, M.CS_EXECSEG_MAIN_BINARY)], True)
        c.lit(M.IDENT)
        out += _rows("signature_" + arch, c.fields) + _rows("signature_tail_" + arch, [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain(); c.lit(bytes(M.DLPREFIX))
    pre = c.lead
    c = _fieldchain(); c.lit(M.DLBIND)
    out += _rows("prefix", [dict(width=0, value=0, src="none", endian="-", lead=pre)])
    out += _rows("binddata", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)])
    used |= set(pre) | set(c.lead)
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    return out


TABLES.append(("enc-macho", ["unisa/image/macho.py", "exec/facts/export.py"], machofields))


def elffields():
    """ELF program-header / dynamic-section field schema and the import/loader slot tables (unisa/image/elf.py, pe.py)."""
    sys.path.insert(0, str(ROOT))
    from unisa.image import elf
    from unisa.image.pe import IMPORTS
    out = ["=%s\tint\t%d" % kv for kv in dict(DATA=1 << 40, RELOCS=4 << 40, iat_size=8 * len(IMPORTS), VADDR=elf.VADDR,
                                               dyn_doff=elf.VADDR + 32, fmt_elf=1, fmt_macho=2, fmt_pe=3).items()]
    used = set()
    def emit(name, c):
        nonlocal used
        out.extend(_rows(name, c.fields) + _rows(name + "_tail", [dict(width=0, value=0, src="none", endian="-", lead=c.lead)]))
        used |= {x for r in c.fields + [dict(lead=c.lead)] for x in r["lead"]}
    c = _fieldchain()
    for i in range(len(IMPORTS)): c.field(8, "ml_imp_%d" % i)
    emit("iat", c)
    c = _fieldchain()
    for i in range(4): c.field(8, "ml_dl_%d" % i)
    emit("dlslots", c)
    c = _fieldchain(); c.lit(bytes(32)); emit("dynslots", c)
    for arch in sorted(elf.MACHINE):
        out.append("=HDRS_%s\tint\t%d" % (arch, elf.HDRS(arch)))
        def preamble(c, phnum):
            c.lit(b"\x7fELF" + bytes([2, 1, 1, 0]) + bytes(8))
            for w, v in [(2, 2), (2, elf.MACHINE[arch]), (4, 1), (8, "entryva"), (8, elf.EHDR), (8, 0),
                         (4, 0), (2, elf.EHDR), (2, elf.PHDR), (2, phnum), (2, 0), (2, 0), (2, 0)]: c.field(w, v)
        def phdr(c, typ, flags, off, va, filesz, memsz, align):
            for w, v in [(4, typ), (4, flags), (8, off), (8, va), (8, va), (8, filesz), (8, memsz), (8, align)]: c.field(w, v)
        c = _fieldchain(); c.comp("header-init")
        preamble(c, 2)
        phdr(c, 1, 5, 0, elf.VADDR, "tend", "tend", elf.PAGE)
        phdr(c, 1, 6, "doff", "data_va", "stored", "memlen", elf.PAGE)
        emit("static_" + arch, c)
        c = _fieldchain(); c.comp("dyn-init")
        preamble(c, 4)
        interp = b"/lib/ld-linux-aarch64.so.1" if arch == "arm64" else b"/lib64/ld-linux-x86-64.so.2"
        phdr(c, 3, 4, 288, elf.VADDR + 288, len(interp) + 1, len(interp) + 1, 1)
        phdr(c, 1, 5, 0, elf.VADDR, "tend", "tend", elf.PAGE)
        phdr(c, 1, 6, "doff", "dyn_slots", "dyn_filesz", "dyn_memsz", elf.PAGE)
        phdr(c, 2, 4, 608, elf.VADDR + 608, 176, 176, 8)
        c.lit(interp + b"\0" + bytes(32 - len(interp) - 1))
        c.lit(b"\0libc.so.6\0dlopen\0dlsym\0dlclose\0dlerror\0")
        c.lit(bytes(24))
        for nameoff in (11, 18, 24, 32):
            c.field(4, nameoff); c.lit(bytes((0x12, 0, 0, 0))); c.field(8, 0); c.field(8, 0)
        c.field(4, 1); c.field(4, 5)
        c.lit(bytes(24))
        for i in range(4):
            c.comp("dynreloc%d" % (8 * i))
            c.field(8, "dyn_reloc"); c.field(8, ((i + 1) << 32) | (1025 if arch == "arm64" else 6)); c.field(8, 0)
        for tag, val in ((1, 1), (4, elf.VADDR + 480), (5, elf.VADDR + 320), (6, elf.VADDR + 360),
                         (10, 40), (11, 24), (7, elf.VADDR + 512), (8, 96), (9, 24), (30, 8), (0, 0)):
            c.field(8, tag); c.field(8, val)
        emit("dyn_" + arch, c)
    out += ["@relocs\toff:int"] + ["\t%d" % (8 * i) for i in range(4)]
    out += ["@bytes\tv:int"] + ["\t%d" % int(x[4:]) for x in sorted(used) if x.startswith("byte")]
    return out


TABLES.append(("enc-elf", ["unisa/image/elf.py", "unisa/image/pe.py", "exec/facts/export.py"], elffields))


def x86entry():
    """x86_64 encoder entry (was exec/enc/gen.py): table bases and class ids (local
    constants of the encoder), ENCSPEC alu2/setcc/shiftext opcodes, emit_x86.NUM
    register numbers, the words START interns, Win32 import names."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import ENCSPEC, REGMAP
    from unisa.emit_x86 import NUM
    from unisa.image.pe import IMPORTS
    from exec.facts.load import facts as F
    X86 = ENCSPEC["x86_64"]
    const = dict(OPC=70, REGN=71, AOPC=72, ACC=73, LABD=74, KND=75, BLB=76, SZ=77, TGT=78, BRG=79, SHT=80,
                 OFF=81, FIT=82, SHX=83, MSN=84)
    const = {k: v * 10 ** 6 for k, v in const.items()}
    cn = ("C_MOV C_IMM C_ALU C_MUL C_LD8 C_ST8 C_LD C_ST C_SET C_RET C_SHF C_CALLR C_PUSH C_POP C_NOP C_FRAME C_ZERO "
          "C_SETREG C_SPINIT C_DIV C_MOD C_UDIV C_UMOD C_GATE C_LEA C_SETMEM C_ARGSAVE C_ARGVGET C_ITOA").split()
    C = {n: i for i, n in enumerate(cn, 1)}
    C.update(C_HOSTCALL=60, C_HOSTADDR=61)
    const.update(C, SCR=11, SCR2=3, SPREG=NUM[REGMAP["x86_64"][7]], RAX=NUM["rax"], RDX=NUM["rdx"])
    out = ["=%s\tint\t%d" % kv for kv in const.items()]
    out.append("=division_registers\tjson\t%s" % json.dumps([NUM[r] for r in REGMAP["x86_64"][:7]]))
    classes = {".div": C["C_DIV"], ".mod": C["C_MOD"], ".udiv": C["C_UDIV"], ".umod": C["C_UMOD"], "setreg": C["C_SETREG"],
               "spinit": C["C_SPINIT"], ".zero": C["C_ZERO"], "push": C["C_PUSH"], "pop": C["C_POP"], "nop": C["C_NOP"],
               ".frame": C["C_FRAME"], "callr": C["C_CALLR"], "mov": C["C_MOV"], "imm": C["C_IMM"], "mul64": C["C_MUL"],
               "load64": C["C_LD8"], "store64": C["C_ST8"], ".ld": C["C_LD"], ".st": C["C_ST"], "ret": C["C_RET"]}
    classes.update({r["op"]: r["id"] for r in F("enc-fp-ops")})
    classes.update(hostcall=60, hostaddr=61)
    classes.update({r["name"]: r["value"] for r in F("enc-x86win-ids")})
    classes.update({"itoa": C["C_ITOA"], "gate": C["C_GATE"], ".lea": C["C_LEA"], "setmem": C["C_SETMEM"],
                    "argsave": C["C_ARGSAVE"], "argvget": C["C_ARGVGET"]})
    for op in X86["alu2"]:
        classes[op] = C["C_ALU"]
    for op in X86["setcc"]:
        classes[op] = C["C_SET"]
    for op in X86["shiftext"]:
        classes[op] = C["C_SHF"]
    out.append("=unknown\tjson\t%s" % json.dumps(sorted(set(range(257)) - set(classes.values()))))
    out.append("@wi\tword:str")
    out += ["\t" + k for k in tuple(F("enc-x86win-ops")) + tuple(F("enc-x86win-rcs"))]
    out.append("@wimp\tword:str\tn:int")
    out += ["\t%s\t%d" % (name, i + 1) for i, name in enumerate(IMPORTS)]
    out.append("@classes\tword:str\tcls:int\taopc:json\tacc:json\tshx:json")
    for op, c in classes.items():
        out.append("\t%s\t%d\t%s\t%s\t%s" % (op, c, *(json.dumps([X86[t][op]] if op in X86[t] else [])
                                                      for t in ("alu2", "setcc", "shiftext"))))
    out.append("@regs\tword:str\tn:int")
    out += ["\t%s\t%d" % (nm, n + 1) for nm, n in NUM.items()]
    meta = tuple(F("enc-tins-meta"))
    ids = [("imm", "tagimm"), ("reg", "tagreg"), ("role", "role"), ("form", "form"), ("reloc", "reloc"), ("rel32", "rel32")]
    ids += [(w, w) for w in ("true", "false", "winapi", "carry",
                             *[k for k in meta if k not in ("role", "form", "reloc", "carry", "winapi")])]
    ids += [("mem", "tagmem"), ("addr", "tagaddr"), ("lnx/x86_64", "target1"), ("osx/x86_64", "target2"),
            ("win/x86_64", "target3")]
    ids += [("@" + k, "h_" + k) for k in ("target", "data", "sym", "src_os", "data_len", "bss", "relocs", "argc", "argv")]
    out.append("@ids\tword:str\tnm:str")
    out += ["\t%s\t%s" % w for w in ids]
    win = tuple(F("enc-x86win-meta"))
    out.append("@metakeys\tkey:str\twin:int")
    out += ["\t%s\t%d" % (k, k in win) for k in meta if k not in ("role", "form", "reloc", "carry")]
    out.append("@winmeta\tk:str")
    out += ["\t" + k for k in win]
    out.append("@puts\ti:int")
    out += ["\t%d" % i for i in range(4)]
    return out


TABLES.append(("enc-x86", ["unisa/catalog.py", "unisa/emit_x86.py", "unisa/image/pe.py", "exec/facts/enc-fp-ops.tsv",
                           "exec/facts/enc-x86win-ids.tsv", "exec/facts/enc-x86win-ops.tsv", "exec/facts/enc-x86win-rcs.tsv",
                           "exec/facts/enc-x86win-meta.tsv", "exec/facts/enc-tins-meta.tsv", "exec/facts/export.py"], x86entry))

def armentry():
    """ARM64 encoder entry (was exec/enc/arm.py): table bases, op specs (shape string ->
    operand checks), ENCSPEC alu3/invcond values, register names, START words."""
    import json
    sys.path.insert(0, str(ROOT))
    from unisa.catalog import ENCSPEC
    from unisa.image.pe import IMPORTS
    from exec.facts.load import facts as F
    A = ENCSPEC["arm64"]
    specs = {'mov': ('rr', 1), 'imm': ('ri', 2), 'mul64': ('rrr', 3), 'ret': ('', 4), 'nop': ('', 5), 'callr': ('r', 6),
             'hostcall': ('rr', 60), 'hostaddr': ('ri', 61), 'load64': ('rri', 9), 'store64': ('rir', 10),
             '.ld': ('rrii', 11), '.st': ('riri', 12), 'jump': ('l', 13), 'jumpz': ('rl', 14), 'call': ('l', 15),
             'setreg': ('rv', 25), 'spinit': ('r', 26), 'gate': ('', 27), '.lea': ('rl', 28), 'setmem': ('ir', 29),
             'argsave': ('iib', 30), 'argvget': ('rri', 31), '.zero': ('rii', 32), 'winsave': ('i', 33),
             'winrest': ('ir', 34), 'winstdh': ('i', 35), 'winargs': ('iii', 36), 'itoa': ('iii', 37)}
    specs.update({r['op']: (r['shape'], r['cls']) for r in F('enc-armint-specs')})
    specs.update({r['op']: (r['shape'], r['cls']) for r in F('enc-armfp-specs')})
    specs.update({k: ('rrr', 7) for k in A['alu3']})
    specs.update({k: ('rrr', 8) for k in A['invcond']})
    out = ["=OP\tint\t70000000", "=REG\tint\t71000000", "=BASE\tint\t72000000", "=LP\tint\t73000000", "=ZERO\tint\t0"]
    out.append("=unknown\tjson\t%s" % json.dumps(sorted(set(range(257)) - set(range(1, len(specs) + 1)))))
    meta = tuple(F("enc-tins-meta"))
    out.append("=meta_classes\tjson\t%s" % json.dumps({"meta_" + k: [i] for i, k in enumerate(meta, 1)}))
    codes = {'r': 1, 'i': 2, 'l': 3, 'b': 4}
    out.append("@specs\ti:int\top:str\tn:int\tcls:int\tlpos:int\tbase:int\tspin:int\tchars:json\tlast:int\tlastsigned:int")
    for i, (op, (shape, cls)) in enumerate(specs.items(), 1):
        chars = []
        for j, k in enumerate(shape):
            chars.append(dict(i=j, p=j - 1, first=int(j == 0), ps=int(j > 0 and chars[-1]['signed']),
                              isv=int(k == 'v'), code=codes.get(k, 0), signed=int(k == 'i' and op != 'imm')))
        out.append("\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%d" % (
            i, op, len(shape), cls, shape.find('l'), A['alu3'].get(op, A['invcond'].get(op, 0)), int(op == 'spinit'),
            json.dumps(chars, separators=(",", ":")), len(shape) - 1, int(bool(chars) and chars[-1]['signed'])))
    out.append("@regs\tword:str\tn:int")
    out += ["\tx%d\t%d" % (i, i + 1) for i in range(31)]
    out.append("@metakeys\tword:str\tn:int")
    out += ["\t%s\t%d" % (k, n) for n, k in enumerate(meta, 1)]
    out.append("@imports\tword:str\tn:int")
    out += ["\t%s\t%d" % (name, i + 1) for i, name in enumerate(IMPORTS)]
    for name, rows in (("inputkeys", F('enc-arminput-keys')), ("headers", F('enc-armlayout-headers')),
                       ("winkeys", F('enc-armwin-keys')), ("winreset", F('enc-armwin-reset'))):
        out.append("@%s\tword:str" % name)
        out += ["\t" + k for k in rows]
    out.append("@targets\tkey:str\tword:str")
    out += ["\t%s\t%s" % (r['key'], r['value']) for r in F('enc-armlayout-targets')]
    out.append("@puts\ti:int\tp:int")
    out += ["\t%d\t%d" % (i, i - 1) for i in range(4)]
    return out


TABLES.append(("enc-arm", ["unisa/catalog.py", "unisa/image/pe.py", "exec/facts/enc-armint-specs.tsv",
                           "exec/facts/enc-armfp-specs.tsv", "exec/facts/enc-tins-meta.tsv", "exec/facts/enc-arminput-keys.tsv",
                           "exec/facts/enc-armlayout-headers.tsv", "exec/facts/enc-armlayout-targets.tsv",
                           "exec/facts/enc-armwin-keys.tsv", "exec/facts/enc-armwin-reset.tsv", "exec/facts/export.py"], armentry))

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
