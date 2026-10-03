#!/usr/bin/env python3
"""K2 trace translator (tests side, never shipped; exec/k2-boundary.md strategy 1).

  python3 tests/k2translate.py record OUTDIR TAG GEN [ARGS...] -- MODULE...
      run GEN once in-process with each MODULE's install wrapped; for each module
      record every finite_rules install/install_rows/install_template call made
      under it and every E.P.fresh allocation (holder cur, kind, label) in order;
      writes OUTDIR/<stem>.<TAG>.json.
  python3 tests/k2translate.py emit MODULE FLAGS=TRACE.json...
      FLAGS is a comma list of the mode flags set in that run (`-` for none).
      Merges the per-mode traces into exec/<dir>/<stem>-manifest.tsv plus
      exec/facts/<stem>.tsv with ops rows/table/template/foreach and cells
      fresh:/$NAME/fact refs/export only (exec/assemble.py).  Rows present in
      some modes get a `when` conjunction (refused when the mode set is not a
      cube over the flags); runs of rows differing only by data fold into
      `foreach` over a fact table.  Refuses ("not covered") on callbacks,
      live-graph reads, return values, fresh labels bound out of allocation
      order or never bound, and > FOLD_MAX rows after folding.
The caller runs assemble.run(manifest, E, P, flags, env=<install str/int kwargs>).
"""
import difflib, importlib.abc, importlib.util, inspect, itertools, json, os, runpy, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FOLD_MAX = 40
OPS = {"install": "rows", "install_rows": "table", "install_template": "template"}


class Refuse(Exception):
    pass


def _js(v):
    if isinstance(v, (list, tuple)):
        return [_js(x) for x in v]
    if isinstance(v, dict):
        return {str(k): _js(x) for k, x in v.items()}
    if isinstance(v, range):
        return {"__range__": [v.start, v.stop]}
    if isinstance(v, Path):
        return {"__path__": str(v)}
    return v


def record(outdir, tag, gen, args, modules):
    genpath = ROOT / gen
    sys.path.insert(0, str(ROOT / "exec"))
    sys.path.insert(0, str(genpath.parent))
    import finite_rules as fr
    traces, stack, inner = {}, [], [0]
    for m in modules:
        p = (ROOT / m).resolve()
        t = traces[p.stem] = {"calls": [], "args": None, "refused": None, "path": str(p)}
        src = p.read_text()
        for bad in ("g.st", ".st[", "g.labels", ".st.get", "in g.st"):
            if bad in src:
                t["refused"] = "live graph read %r" % bad

    def note(why):
        if stack and not traces[stack[-1]]["refused"]:
            traces[stack[-1]]["refused"] = why

    def wrap(name, fn):
        def w(*a, **k):
            if not stack or inner[0]:
                return fn(*a, **k)
            inner[0] = 1
            try:
                sig = inspect.signature(fn).bind(*a, **k)
                d = dict(sig.arguments)
                d.pop("g")
                f = d.get("fresh")
                if callable(f):
                    used = []
                    def tf(kind, f=f):
                        used.append(kind)
                        return f(kind)
                    sig.arguments["fresh"] = tf
                    r = fn(*sig.args, **sig.kwargs)
                    if used:
                        note("template fresh callback invoked (%s)" % used[:3])
                    d["fresh"] = None
                else:
                    r = fn(*a, **k)
                traces[stack[-1]]["calls"].append(["call", name, _js(d)])
                return r
            finally:
                inner[0] = 0
        return w
    for n in OPS:
        setattr(fr, n, wrap(n, getattr(fr, n)))
    patched = set()

    def wrap_install(stem, inst):
        def w(*a, **k):
            b = inspect.signature(inst).bind(*a, **k).arguments
            kw = {}
            t = traces[stem]
            for n, v in b.items():
                P = getattr(v, "P", None)
                if P is not None and hasattr(P, "fresh") and id(P) not in patched:
                    patched.add(id(P))
                    of = P.fresh
                    def nf(self, h="k", of=of):
                        lab = of(self, h)
                        if stack and not inner[0]:
                            traces[stack[-1]]["calls"].append(["fresh", self.cur, h, lab])
                        return lab
                    P.fresh = nf
                if isinstance(v, (str, int)) and not isinstance(v, bool):
                    kw[n] = v
                elif isinstance(v, bool):
                    kw[n] = v
                elif inspect.isfunction(v) or inspect.ismethod(v):
                    t["refused"] = t["refused"] or "callback argument %s" % n
            if t["args"] is not None:
                t["refused"] = t["refused"] or "install called twice"
            t["args"] = kw
            stack.append(stem)
            try:
                r = inst(*a, **k)
            finally:
                stack.pop()
            if r is not None:
                t["refused"] = t["refused"] or "install returns a value (%s)" % type(r).__name__
            return r
        return w
    paths = {Path(t["path"]).stem: t["path"] for t in traces.values()}

    class Finder(importlib.abc.MetaPathFinder):
        def find_spec(self, name, path, target=None):
            if name not in paths or name in sys.modules:
                return None
            spec = importlib.util.spec_from_file_location(name, paths[name])
            orig = spec.loader.exec_module
            def ex(m):
                orig(m)
                m.install = wrap_install(name, m.install)
            spec.loader.exec_module = ex
            return spec
    sys.meta_path.insert(0, Finder())
    sys.argv = [str(genpath), os.path.join(outdir, tag + ".graph")] + args
    os.chdir(ROOT)
    try:
        runpy.run_path(str(genpath), run_name="__main__")
    except SystemExit as e:
        if e.code not in (0, None):
            raise
    os.unlink(sys.argv[1])
    for stem, t in traces.items():
        if t["args"] is None:
            t["refused"] = t["refused"] or "install never called in this mode"
        Path(outdir, "%s.%s.json" % (stem, tag)).write_text(json.dumps(t))


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


class Emitter:
    def __init__(self, here, stem):
        self.here, self.stem, self.facts, self.tables = here, stem, {}, {}

    def fact(self, v):
        for k, x in self.facts.items():
            if x == v:
                return k
        k = "v%d" % (len(self.facts) + 1)
        self.facts[k] = v
        return k

    def rows(self, t):
        """One mode's trace -> list of rows (cells are strings or ('lit', value))."""
        args, owner, pending, out = t["args"], {}, [], []

        def cell(k, v):
            if isinstance(v, str):
                if pending and pending[0][2] == v:
                    cur, kind, lab = pending.pop(0)
                    owner[lab] = k
                    return "fresh:U:%s:%s" % (cur, kind), True
                if v in owner:
                    return "$" + owner[v], False
                if any(p[2] == v for p in pending):
                    raise Refuse("fresh allocation order differs from first use (%s)" % v)
            for n, a in args.items():
                if a == v and type(a) == type(v) and not isinstance(v, bool):
                    return "$" + n, False
            return ("lit", json.dumps(v, sort_keys=True)), False

        for c in t["calls"]:
            if c[0] == "fresh":
                pending.append(c[1:])
                continue
            _, name, d = c
            op = OPS[name]
            if op == "table":
                p = Path(d["path"]["__path__"]).resolve()
                if p.parent != self.here:
                    raise Refuse("install_rows path outside " + str(self.here))
                stemcell = p.name
            else:
                root = Path(d["root"]["__path__"] if isinstance(d["root"], dict) else d["root"]).resolve()
                if root != self.here:
                    raise Refuse("install root %s outside %s" % (root, self.here))
                stemcell = d["stem"]
            if op == "template" and d.get("facts"):
                raise Refuse("template with a non-empty facts dict: not exercised yet")
            exp, bind, seq = [], [], []
            for k, v in (d.get("bindings") or {}).items():
                s, new = cell(k, v)
                if new:
                    exp.append(k)
                bind.append((k, s))
            for k, v in (d.get("sequences") or {}).items():
                s, new = cell(k, v)
                if new or (isinstance(v, str) and s[0] == "lit"):
                    raise Refuse("label/string inside sequences (%s)" % k)
                seq.append((k, s))
            o = {}
            if exp:
                o["export"] = exp
            if d.get("classes"):
                o["classes"] = d["classes"]
            dom = d.get("domain")
            if dom and dom != {"__range__": [0, 257]}:
                o["domain"] = dom["__range__"]
            if op in ("template", "table") and d.get("mode", "r") != "r":
                o["mode"] = d["mode"]
            if d.get("overlay"):
                o["overlay"] = True
            out.append((op, stemcell, d.get("section") or "-", tuple(seq), tuple(bind),
                        json.dumps({k: v for k, v in o.items() if k != "classes"}, sort_keys=True),
                        json.dumps(_js(o.get("classes")))))
        if pending:
            raise Refuse("fresh labels allocated but never bound: %s" % pending[:3])
        return out


def _cube(modes, allmodes, flags):
    """Smallest conjunction over flags selecting exactly MODES among ALLMODES."""
    if modes == allmodes:
        return "-"
    for n in range(1, len(flags) + 1):
        for fs in itertools.combinations(flags, n):
            for signs in itertools.product((True, False), repeat=n):
                sel = {m for m in allmodes if all((f in m) == s for f, s in zip(fs, signs))}
                if sel == modes:
                    return "&".join(("" if s else "!") + f for f, s in zip(fs, signs))
    raise Refuse("row set %s is not a flag conjunction" % sorted(map(sorted, modes)))


def emit(module, specs):
    mod = (ROOT / module).resolve()
    here, stem = mod.parent, mod.stem
    em = Emitter(here, stem)
    runs = []
    for s in specs:
        fl, _, path = s.partition("=")
        t = json.loads(Path(path).read_text())
        if t["refused"] == "install never called in this mode":
            continue   # the caller guards the call; modes come from the runs that reach it
        if t["refused"]:
            raise Refuse(t["refused"])
        runs.append((frozenset(x for x in fl.split(",") if x not in ("", "-")), t))
    args = runs[0][1]["args"]
    if any(t["args"] != args for _, t in runs):
        raise Refuse("install arguments differ across modes")
    flags = sorted(set().union(*[m for m, _ in runs]))
    allmodes = {m for m, _ in runs}
    merged = []   # [row, set(modes)]
    seen = {}
    for m, t in runs:
        rs = em.rows(t)
        if m in seen:
            if seen[m] != rs:
                raise Refuse("runs with equal flags %s differ (positional mode argument)" % sorted(m))
            continue
        seen[m] = rs
        keys = [r for r, _ in merged]
        sm = difflib.SequenceMatcher(a=keys, b=rs, autojunk=False)
        new = []
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                for i in range(i1, i2):
                    merged[i][1].add(m)
                    new.append(merged[i])
            else:
                new += merged[i1:i2]
                new += [[r, {m}] for r in rs[j1:j2]]
        merged = new
    rows = [(r, _cube(frozenset(ms), allmodes, flags)) for r, ms in merged]
    # foreach folding: adjacent rows equal except literal cells
    out, i = [], 0
    while i < len(rows):
        j = i + 1
        def shape(r):
            (op, st, sec, seq, bind, o, cl), w = r
            return (op, st, sec, w, o, cl, tuple((k, v if isinstance(v, str) else "L") for k, v in seq),
                    tuple((k, v if isinstance(v, str) else "L") for k, v in bind))
        while j < len(rows) and shape(rows[j]) == shape(rows[i]):
            j += 1
        out.append(rows[i:j])
        i = j
    lines = []
    for grp in out:
        (op, st, sec, seq, bind, o, cl), w = grp[0]
        oo = json.loads(o)
        if cl != "null":
            oo["classes"] = em.fact(json.loads(cl))
        if len(grp) >= 3 and not oo.get("export"):
            cols = [k for k, v in seq + bind if not isinstance(v, str)]
            tab = [{k: json.loads(v[1]) for k, v in r[0][3] + r[0][4] if not isinstance(v, str)} for r in grp]
            tn = "t%d" % (len(em.tables) + 1)
            em.tables[tn] = (cols, tab)
            lines.append(["foreach", "-", "-", w, stem, "-", "-", "-", json.dumps({"over": tn, "as": "it"})])
            c = lambda kv: "%s=%s" % (kv[0], kv[1] if isinstance(kv[1], str) else "it." + kv[0])
            lines.append(["." + op, st, sec, "-", stem, "-", ",".join(map(c, seq)) or "-",
                          ",".join(map(c, bind)) or "-", json.dumps(oo) if oo else "-"])
            continue
        for r, w in grp:
            c = lambda kv: "%s=%s" % (kv[0], kv[1] if isinstance(kv[1], str) else em.fact(json.loads(kv[1][1])))
            lines.append([op, st, sec, w, stem, "-", ",".join(map(c, r[3])) or "-",
                          ",".join(map(c, r[4])) or "-", json.dumps(oo) if oo else "-"])
    if len(lines) > FOLD_MAX:
        raise Refuse("%d rows after folding (> %d)" % (len(lines), FOLD_MAX))
    if not em.facts and not em.tables:
        for ln in lines:
            ln[4] = "-"
    man = here / (stem + "-manifest.tsv")
    with open(man, "w") as f:
        f.write("# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts\n")
        f.write("# translated from %s.py by tests/k2translate.py (assembled by exec/assemble.py)\n" % stem)
        for ln in lines:
            f.write("\t".join(ln) + "\n")
    if em.facts or em.tables:
        with open(ROOT / "exec" / "facts" / (stem + ".tsv"), "w") as f:
            f.write("# written by tests/k2translate.py from %s\n" % mod.relative_to(ROOT))
            for k, v in em.facts.items():
                f.write("=%s\tjson\t%s\n" % (k, _esc(json.dumps(v))))
            for k, (cols, tab) in em.tables.items():
                f.write("@%s\t%s\n" % (k, "\t".join(c + ":json" for c in cols)))
                for r in tab:
                    f.write("\t" + "\t".join(_esc(json.dumps(r[c])) for c in cols) + "\n")
    print(man, len(lines), "rows; env", sorted(args))


if __name__ == "__main__":
    try:
        if sys.argv[1] == "record":
            i = sys.argv.index("--")
            record(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:i], sys.argv[i + 1:])
        else:
            emit(sys.argv[2], sys.argv[3:])
    except Refuse as e:
        sys.exit("not covered: %s" % e)
