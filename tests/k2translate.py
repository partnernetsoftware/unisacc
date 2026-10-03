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
    if v is None or isinstance(v, (str, int, float, bool)):
        return v
    if isinstance(v, (set, frozenset)):
        return {"__set__": sorted(map(_js, v), key=repr)}
    return {"__opaque__": repr(v)[:80]}


def record(outdir, tag, gen, args, modules):
    genpath = ROOT / gen
    sys.path.insert(0, str(ROOT / "exec"))
    sys.path.insert(0, str(genpath.parent))
    import finite_rules as fr
    traces, stack, inner, graph, probe = {}, [], [0], [None], []
    for m in modules:
        p = (ROOT / m).resolve()
        t = traces[p.stem] = {"calls": [], "args": None, "refused": None, "path": str(p)}
        src = p.read_text()
        for bad in ("g.st", ".st[", ".st.get", "in g.st", "g.seqs"):
            if bad in src:
                t["refused"] = "live graph read %r" % bad

    def note(why):
        if stack and not traces[stack[-1]]["refused"]:
            traces[stack[-1]]["refused"] = why

    class RecSet(set):
        # graph label set: additions under a module become `label` rows; reads refuse
        def add(self, x):
            if stack and not inner[0]:
                traces[stack[-1]]["calls"].append(["label", [x]])
            return set.add(self, x)

        def update(self, *its):
            items = [x for it in its for x in it]
            if stack and not inner[0]:
                traces[stack[-1]]["calls"].append(["label", items])
            return set.update(self, items)

        def __iter__(self):
            if stack and not inner[0]:
                note("live graph read: iterates g.labels")
            return set.__iter__(self)

        def __contains__(self, x):
            if stack and not inner[0]:
                note("live graph read: membership in g.labels")
            return set.__contains__(self, x)

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
                    used, curs = [], []
                    def tf(kind, f=f):
                        used.append(kind)
                        probe.append([])
                        try:
                            return f(kind)
                        finally:
                            pr = probe.pop()
                            curs.append(pr[0][0] if len(pr) == 1 and pr[0][1] == kind else None)
                    sig.arguments["fresh"] = tf
                    r = fn(*sig.args, **sig.kwargs)
                    own = getattr(f, "__self__", None)
                    if used and own is not None and hasattr(own, "cur") and f.__name__ == "fresh":
                        d["fresh"] = "U:" + own.cur
                    elif used and None not in curs and len(set(curs)) == 1:
                        d["fresh"] = "U:" + curs[0]   # callback == one E.P.fresh on a fixed holder cur
                    elif used:
                        note("template fresh callback invoked (%s; holder curs %s)" % (used[:3], sorted(set(map(str, curs)))[:3]))
                        d["fresh"] = None
                    else:
                        d["fresh"] = None
                else:
                    r = fn(*a, **k)
                graph[0] = a[0] if a else k.get("g")
                traces[stack[-1]]["calls"].append(["call", name, _js(d), r if isinstance(r, (str, int)) else None])
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
                        if probe:
                            probe[-1].append((self.cur, h))
                        if stack and not inner[0]:
                            traces[stack[-1]]["calls"].append(["fresh", self.cur, h, lab])
                        return lab
                    P.fresh = nf
                if isinstance(v, (str, int)) and not isinstance(v, bool):
                    kw[n] = v
                elif isinstance(v, bool):
                    kw[n] = v
                # callback arguments are allowed: their results are recorded as data in the
                # calls; graph writes and fresh allocations made by them trip the guards
            for v in b.values():
                for gr in (v, getattr(v, "g", None)):
                    cls = type(gr)
                    if gr is not None and hasattr(cls, "on") and hasattr(cls, "state") and cls not in patched:
                        patched.add(cls)
                        for meth in ("on", "state", "seq"):
                            if hasattr(cls, meth):
                                def gw(self, *a, _m=getattr(cls, meth), _n=meth, **k):
                                    if stack and not inner[0]:
                                        note("direct graph write %s.%s (not through finite_rules)" % (cls.__name__, _n))
                                    return _m(self, *a, **k)
                                setattr(cls, meth, gw)
            for v in b.values():
                for gr in (v, getattr(v, "g", None)):
                    labs = getattr(gr, "labels", None)
                    if isinstance(labs, set) and not isinstance(labs, RecSet):
                        gr.labels = RecSet(labs)
            if stack:
                note("calls another stage module install (%s)" % stem)
            if t["args"] is not None:
                t["refused"] = t["refused"] or "install called twice"
            t["args"] = kw
            stack.append(stem)
            try:
                r = inst(*a, **k)
            finally:
                stack.pop()
            if r is not None and not isinstance(r, (str, int)):
                t["refused"] = t["refused"] or "install returns a value (%s)" % type(r).__name__
            t["ret"] = r
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
    st = set(getattr(graph[0], "st", None) or ())
    for stem, t in traces.items():
        t["states"] = sorted(x for x in _strings(t["calls"]) if x in st)
        if t["args"] is None:
            t["refused"] = t["refused"] or "install never called in this mode"
        Path(outdir, "%s.%s.json" % (stem, tag)).write_text(json.dumps(t))


def _strings(v):
    if isinstance(v, str):
        yield v
    elif isinstance(v, (list, tuple)):
        for x in v:
            yield from _strings(x)
    elif isinstance(v, dict):
        for k, x in v.items():
            yield from _strings(k)
            yield from _strings(x)


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


class Emitter:
    def __init__(self, here, stem):
        self.here, self.stem, self.facts, self.tables = here, stem, {}, {}
        self.tfacts, self.states = [], set()

    def p4(self, v, where):
        hit = [x for x in _strings(v) if x in self.states]
        if hit:
            raise Refuse("P4: state label(s) %s inside %s data; must stay template rows" % (hit[:3], where))

    def tfact(self, d):
        if d not in self.tfacts:
            self.tfacts.append(d)
        return "%s-f%d" % (self.stem, self.tfacts.index(d) + 1)

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
        self.states |= set(t.get("states", ()))
        ret, retrow = t.get("ret"), None

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
            if isinstance(v, str):   # P4: names in bindings stay in the manifest, never in facts
                if any(ch in v for ch in ",\t\n{}\\"):
                    raise Refuse("state label %r not expressible as @str" % v)
                return "@str:" + v, False
            self.p4(v, "binding/sequence " + k)
            return ("lit", json.dumps(v, sort_keys=True)), False

        for c in t["calls"]:
            if c[0] == "fresh":
                pending.append(c[1:])
                continue
            if c[0] == "label":
                if any(set(x) & set(",\t\n") for x in c[1]):
                    raise Refuse("label name not expressible")
                out.append(("label", ",".join(c[1]), "-", (), (), "{}", "null"))
                continue
            _, name, d, r = c
            if "__opaque__" in json.dumps(d) or "__set__" in json.dumps(d):
                raise Refuse("non-data argument in %s (%s)" % (name, json.dumps(d)[json.dumps(d).find("__"):][:60]))
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
            tf = None
            if op == "template" and d.get("facts"):
                self.p4(d["facts"], "template facts")
                tf = self.tfact(d["facts"])
            if op == "template" and d.get("fresh"):
                tf_fresh = d["fresh"]
            else:
                tf_fresh = "-"
            exp, bind, seq = [], [], []
            for k, v in (d.get("bindings") or {}).items():
                s, new = cell(k, v)
                if new:
                    exp.append(k)
                bind.append((k, s))
            for k, v in (d.get("sequences") or {}).items():
                s, new = cell(k, v)
                if new or isinstance(v, str):
                    raise Refuse("label/string inside sequences (%s)" % k)
                seq.append((k, s))
            o = {}
            if exp:
                o["export"] = exp
            if d.get("classes"):
                self.p4(d["classes"], "classes")
                o["classes"] = d["classes"]
            if tf:
                o["_tf"] = tf
            if tf_fresh != "-":
                o["_fresh"] = tf_fresh
            if ret is not None and r == ret and retrow is None:
                o["result"] = "ret"
                retrow = len(out)
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
        if ret is not None and retrow is None:
            if isinstance(ret, str) and not any(ch in ret for ch in ",\t\n{}\\"):
                out.append(("let", "-", "-", (), (("ret", "@str:" + ret),), "{}", "null"))
            else:
                raise Refuse("install return value %r not traceable" % (ret,))
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
    fstem = stem
    fp = ROOT / "exec" / "facts" / (stem + ".tsv")
    try:
        if "tests/k2translate.py" not in fp.read_text().split("\n", 1)[0]:
            fstem = "k2-" + stem   # never overwrite a hand/export-written facts file
    except FileNotFoundError:
        pass
    em = Emitter(here, fstem)
    runs = []
    for s in specs:
        fl, _, path = s.partition("=")
        t = json.loads(Path(path).read_text())
        if t["refused"] == "install never called in this mode":
            continue   # the caller guards the call; modes come from the runs that reach it
        if t["refused"]:
            raise Refuse(t["refused"])
        runs.append((frozenset(x for x in fl.split(",") if x not in ("", "-")), t))
    if not runs:
        raise Refuse("install never called by the recorded generator runs")
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
        tfc = oo.pop("_tf", None)
        frc = oo.pop("_fresh", "-")
        if cl != "null":
            oo["classes"] = em.fact(json.loads(cl))
        if len(grp) >= 3 and not oo.get("export"):
            cols = [k for k, v in seq + bind if not isinstance(v, str)]
            tab = [{k: json.loads(v[1]) for k, v in r[0][3] + r[0][4] if not isinstance(v, str)} for r in grp]
            tn = "t%d" % (len(em.tables) + 1)
            em.tables[tn] = (cols, tab)
            lines.append(["foreach", "-", "-", w, stem, "-", "-", "-", json.dumps({"over": tn, "as": "it"})])
            c = lambda kv: "%s=%s" % (kv[0], kv[1] if isinstance(kv[1], str) else "it." + kv[0])
            lines.append(["." + op, st, sec, "-", fstem + ("+" + tfc if tfc else ""), frc, ",".join(map(c, seq)) or "-",
                          ",".join(map(c, bind)) or "-", json.dumps(oo) if oo else "-"])
            continue
        for r, w in grp:
            c = lambda kv: "%s=%s" % (kv[0], kv[1] if isinstance(kv[1], str) else em.fact(json.loads(kv[1][1])))
            lines.append([op, st, sec, w, fstem + ("+" + tfc if tfc else ""), frc, ",".join(map(c, r[3])) or "-",
                          ",".join(map(c, r[4])) or "-", json.dumps(oo) if oo else "-"])
    if len(lines) > FOLD_MAX:
        raise Refuse("%d rows after folding (> %d)" % (len(lines), FOLD_MAX))
    if len(em.tfacts) > 16:
        raise Refuse("%d distinct template facts dicts (> 16)" % len(em.tfacts))
    for ln in lines:
        parts = [x for x in ln[4].split("+") if x != fstem or em.facts or em.tables]
        ln[4] = "+".join(parts) or "-"
    for i, d in enumerate(em.tfacts, 1):
        with open(ROOT / "exec" / "facts" / ("%s-f%d.tsv" % (fstem, i)), "w") as f:
            f.write("# written by tests/k2translate.py from %s (template facts)\n" % mod.relative_to(ROOT))
            for k, v in d.items():
                f.write("=%s\tjson\t%s\n" % (k, _esc(json.dumps(v))))
    man = here / (stem + "-manifest.tsv")
    with open(man, "w") as f:
        f.write("# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts\n")
        f.write("# translated from %s.py by tests/k2translate.py (assembled by exec/assemble.py)\n" % stem)
        for ln in lines:
            f.write("\t".join(ln) + "\n")
    if em.facts or em.tables:
        with open(ROOT / "exec" / "facts" / (fstem + ".tsv"), "w") as f:
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
