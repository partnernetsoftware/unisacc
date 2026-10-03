#!/usr/bin/env python3
"""K2 trace translator (tests side, never shipped; exec/k2-boundary.md strategy 1).

  python3 tests/k2translate.py record MODULE OUT.json GEN [ARGS...]
      run GEN in-process with MODULE.install wrapped; record every finite_rules
      install/install_rows/install_template call made under it and every
      E.P.fresh allocation (holder cur, kind, label) in order.
  python3 tests/k2translate.py emit MODULE TRACE.json [TRACE.json...]
      merge the traces (one per mode) into exec/<dir>/<stem>-manifest.tsv plus
      exec/facts/<stem>.tsv, using only rows/template/fresh/bind/seq/export
      cells of exec/assemble.py.  Refuses ("not covered") on callbacks,
      live-graph reads, mode-variant traces (when-merge), fresh labels whose
      allocation order differs from first use or that are never bound, and
      flattened manifests longer than FOLD_MAX (foreach folding: report).
The caller then runs assemble.run(manifest, E, P, flags, env=<install str/int kwargs>).
"""
import importlib.abc, importlib.util, inspect, json, os, runpy, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FOLD_MAX = 40
OPS = {"install": "rows", "install_rows": "table", "install_template": "template"}


def refuse(why):
    sys.exit("not covered: " + why)


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


def record(module, out, gen, args):
    module = str((ROOT / module).resolve())
    src = Path(module).read_text()
    for bad in ("g.st", ".st[", "g.labels"):
        if bad in src:
            refuse("live graph read %r in %s" % (bad, module))
    genpath = ROOT / gen
    sys.path.insert(0, str(ROOT / "exec"))
    sys.path.insert(0, str(genpath.parent))
    import finite_rules as fr
    trace = {"calls": [], "args": None}
    depth = [0]
    inner = [0]

    def wrap(name, fn):
        def w(*a, **k):
            if not depth[0] or inner[0]:
                return fn(*a, **k)
            inner[0] = 1
            try:
                return rec(*a, **k)
            finally:
                inner[0] = 0
        def rec(*a, **k):
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
                    refuse("template fresh callback invoked (%s)" % used[:3])
                d["fresh"] = None
            else:
                r = fn(*a, **k)
            trace["calls"].append(["call", name, _js(d)])
            return r
        return w
    for n in OPS:
        setattr(fr, n, wrap(n, getattr(fr, n)))
    patched = set()

    def wrap_install(inst):
        def w(*a, **k):
            b = inspect.signature(inst).bind(*a, **k).arguments
            kw = {}
            for n, v in b.items():
                P = getattr(v, "P", None)
                if P is not None and hasattr(P, "fresh") and id(P) not in patched:
                    patched.add(id(P))
                    of = P.fresh
                    def nf(self, h="k", of=of):
                        lab = of(self, h)
                        if depth[0]:
                            trace["calls"].append(["fresh", self.cur, h, lab])
                        return lab
                    P.fresh = nf
                if isinstance(v, (str, int)):
                    kw[n] = v
                elif inspect.isfunction(v):
                    refuse("callback argument %s" % n)
            trace["args"] = kw
            depth[0] += 1
            try:
                return inst(*a, **k)
            finally:
                depth[0] -= 1
        return w
    stem = Path(module).stem

    class Finder(importlib.abc.MetaPathFinder):
        def find_spec(self, name, path, target=None):
            if name != stem:
                return None
            sys.meta_path.remove(self)
            spec = importlib.util.spec_from_file_location(name, module)
            orig = spec.loader.exec_module
            def ex(m):
                orig(m)
                m.install = wrap_install(m.install)
            spec.loader.exec_module = ex
            return spec
    sys.meta_path.insert(0, Finder())
    sys.argv = [str(genpath), out + ".graph"] + args
    os.chdir(ROOT)
    try:
        runpy.run_path(str(genpath), run_name="__main__")
    except SystemExit as e:
        if e.code not in (0, None):
            raise
    if trace["args"] is None:
        refuse("module install never called")
    Path(out).write_text(json.dumps(trace))


def _esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n")


def _norm(t):
    # labels differ across modes (global counter): compare with labels renamed by order
    m, out = {}, []
    for c in t["calls"]:
        if c[0] == "fresh":
            m[c[3]] = "F%d" % len(m)
            out.append(["fresh", c[1], c[2]])
        else:
            s = json.dumps(c)
            for lab, r in sorted(m.items(), key=lambda x: -len(x[0])):
                s = s.replace(json.dumps(lab), json.dumps(r))
            out.append(s)
    return out, t["args"]


def emit(module, traces):
    ts = [json.loads(Path(t).read_text()) for t in traces]
    base = ts[0]
    for t in ts[1:]:
        if _norm(t) != _norm(base):
            refuse("trace differs across modes (when-merge needed)")
    mod = (ROOT / module).resolve()
    here, stem = mod.parent, mod.stem
    args = base["args"]
    facts, rows, owner, exported, pending = {}, [], {}, set(), []

    def fact(v):
        for k, x in facts.items():
            if x == v:
                return k
        k = "v%d" % (len(facts) + 1)
        facts[k] = v
        return k

    def cell(k, v):
        if isinstance(v, str):
            if pending and pending[0][2] == v:
                cur, kind, lab = pending.pop(0)
                owner[lab] = k
                return "fresh:U:%s:%s" % (cur, kind), True
            if v in owner:
                return "$" + owner[v], False
            if any(p[2] == v for p in pending):
                refuse("fresh allocation order differs from first use (%s)" % v)
            for n, a in args.items():
                if a == v:
                    return "$" + n, False
        return fact(v), False

    for c in base["calls"]:
        if c[0] == "fresh":
            pending.append(c[1:])
            continue
        _, name, d = c
        op = OPS[name]
        if op == "table":
            refuse("install_rows not exercised yet")
        root = Path(d["root"]["__path__"] if isinstance(d["root"], dict) else d["root"]).resolve()
        if root != here:
            refuse("install root %s outside %s" % (root, here))
        if op == "template" and d.get("facts"):
            refuse("template with a non-empty facts dict: not exercised yet")
        exp, bind, seq = [], [], []
        for k, v in (d.get("bindings") or {}).items():
            s, new = cell(k, v)
            if new:
                if k in exported:
                    refuse("export name reused: " + k)
                exp.append(k)
                exported.add(k)
            bind.append("%s=%s" % (k, s))
        for k, v in (d.get("sequences") or {}).items():
            s, new = cell(k, v)
            if new or isinstance(v, str):
                refuse("label/string inside sequences (%s)" % k)
            seq.append("%s=%s" % (k, s))
        o = {}
        if exp:
            o["export"] = exp
        if d.get("classes"):
            o["classes"] = fact(d["classes"])
        dom = d.get("domain")
        if dom and dom != {"__range__": [0, 257]}:
            o["domain"] = dom["__range__"]
        if op == "template":
            if d.get("mode", "r") != "r":
                o["mode"] = d["mode"]
            if d.get("overlay"):
                o["overlay"] = True
        rows.append([op, d["stem"], d.get("section") or "-", "-", "", "-",
                     ",".join(seq) or "-", ",".join(bind) or "-", json.dumps(o) if o else "-"])
    if pending:
        refuse("fresh labels allocated but never bound: %s" % pending[:3])
    if len(rows) > FOLD_MAX:
        refuse("%d flattened rows; foreach folding not implemented" % len(rows))
    for r in rows:
        r[4] = stem if facts else "-"
    man = here / (stem + "-manifest.tsv")
    with open(man, "w") as f:
        f.write("# op\tstem\tsection\twhen\tfacts\tfresh\tseq\tbind\topts\n")
        f.write("# translated from %s.py by tests/k2translate.py (assembled by exec/assemble.py)\n" % stem)
        for r in rows:
            f.write("\t".join(r) + "\n")
    if facts:
        with open(ROOT / "exec" / "facts" / (stem + ".tsv"), "w") as f:
            f.write("# written by tests/k2translate.py from %s\n" % mod.relative_to(ROOT))
            for k, v in facts.items():
                f.write("=%s\tjson\t%s\n" % (k, _esc(json.dumps(v))))
    print(man, len(rows), "rows", len(facts), "facts", "env", sorted(args))


if __name__ == "__main__":
    if sys.argv[1] == "record":
        record(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
    else:
        emit(sys.argv[2], sys.argv[3:])
