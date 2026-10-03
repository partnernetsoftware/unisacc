"""Generic manifest assembler (K2, exec/k2-design.md section 2).

A manifest `exec/<stage>/<name>-manifest.tsv` lists installs in row order:

  op  stem  section  when  facts  fresh  seq  bind  opts

op       rows | table | template | call | label | assert-absent | holder | foreach
         (a leading `.` per depth marks the body of the nearest `foreach` above)
stem     the table stem next to the manifest (rows/template), a path (table),
         a sub-manifest stem (call), comma-separated names (label), a state
         (assert-absent), a holder name (holder)
section  literal or format over flags: `fail{errors?.errors}` -> fail.errors / fail
when     `-`, FLAG, `!FLAG`, `A&B`, `fact:NAME` (non-empty fact)
facts    `-` or STEM[+STEM...]: exec/facts/STEM.tsv, merged left to right
fresh    `-` (None) | `P:NAME` (P(NAME).fresh, registered) | `U:NAME` / `S:NAME`
         (unregistered holder with cur=NAME) | `none` (lambda k: None) |
         `=NAME` (holder declared by op holder) | `tape:FILE@PART` (see _tape)
seq,bind comma list of k=V; V is a fact path `a.b.0`, `@rej:TEXT`, `@bytes:TEXT`
         (`@bytes:=FACT` for a fact's text), `$NAME` (env), `fresh:SCOPE:KIND`;
         evaluated left to right
opts     `-` or JSON: mode, domain ([lo,hi)), classes (fact path), overlay,
         result (store the install's return in env under that name),
         over/as (foreach)

Facts files (exec/facts/STEM.tsv, written by exec/facts/export.py):
  # comment / input sha lines
  =NAME<TAB>TYPE<TAB>VALUE                  scalar fact
  @NAME<TAB>col:TYPE<TAB>col:TYPE...        table header; rows follow as
  <TAB>v1<TAB>v2...                         one list element (a dict) per row
TYPE is str | int | json.
"""
import json
import re
from pathlib import Path
from types import SimpleNamespace

from finite_rules import install, install_rows, install_template

FACTS = Path(__file__).resolve().parent / "facts"
_cache = {}


def _cell(t, v):
    if t == "int":
        return int(v)
    if t == "json":
        return json.loads(v)
    if t == "str":
        return re.sub(r"\\(.)", lambda m: {"t": "\t", "n": "\n"}.get(m.group(1), m.group(1)), v)
    raise ValueError("fact type " + t)


def load_facts(stem):
    path = FACTS / (stem + ".tsv")
    if path not in _cache:
        d, cols, cur = {}, None, None
        for ln in path.read_text().split("\n"):
            if not ln or ln.startswith("#"):
                continue
            f = ln.split("\t")
            if f[0].startswith("="):
                d[f[0][1:]] = _cell(f[1], f[2])
            elif f[0].startswith("@"):
                cur = f[0][1:]
                cols = [c.split(":") for c in f[1:]]
                d[cur] = []
            else:
                assert f[0] == "" and cols is not None and len(f) - 1 == len(cols), ln
                d[cur].append({c: _cell(t, v) for (c, t), v in zip(cols, f[1:])})
        _cache[path] = d
    return _cache[path]


def _path(facts, ref):
    v = facts
    for p in ref.split("."):
        v = v[int(p)] if isinstance(v, list) else v[p]
    return v


def _when(w, flags, facts):
    if w in ("", "-"):
        return True
    for t in w.split("&"):
        neg = t.startswith("!")
        t = t[neg:]
        v = bool(facts.get(t[5:])) if t.startswith("fact:") else bool(flags[t])
        if v == neg:
            return False
    return True


def _section(s, flags):
    if s in ("", "-"):
        return None
    return re.sub(r"\{(\w+)\?([^}]*)\}", lambda m: m.group(2) if flags[m.group(1)] else "", s)


class Run:
    def __init__(self, E, P, flags, env):
        self.E, self.P, self.flags, self.env = E, P, flags, env
        self.holders = {}
        self.root = None

    @staticmethod
    def holder(cur):
        return SimpleNamespace(cur=cur)

    def fresh(self, spec):
        if spec in ("", "-"):
            return None
        if spec == "none":
            return lambda k: None
        kind, _, name = spec.partition(":")
        if spec.startswith("="):
            h = self.holders[spec[1:]]
            if isinstance(h, self.P):
                return h.fresh
        elif kind == "P":
            return self.P(name).fresh
        elif kind in ("U", "S"):
            h = self.holder(name)
        elif kind == "tape":
            return self._tape(name)
        else:
            raise ValueError("fresh " + spec)
        return lambda k, h=h: self.E.P.fresh(h, k)

    def _tape(self, spec):
        # FILE@PART: rows `part owner kind key` of a *-fresh.tsv; one label per
        # row of PART in row order through an owner holder; fresh(key) looks it up.
        file, _, part = spec.partition("@")
        labels = {}
        for ln in (self.root / file).read_text().split("\n"):
            f = ln.split("\t")
            if ln and not ln.startswith("#") and f[0] == part:
                labels[f[3]] = self.E.P.fresh(self.holder(f[1]), f[2])
        return labels.__getitem__

    def value(self, v, facts):
        if v.startswith("@rej:"):
            return self.E.rej(v[5:])
        if v.startswith("@bytes:"):
            t = v[7:]
            t = str(_path(facts, t[1:])) if t.startswith("=") else t
            return [("SBOUT", c) for c in t.encode()]
        if v.startswith("$"):
            return self.env[v[1:]]
        if v.startswith("fresh:"):
            scope, kind = v[6:].rsplit(":", 1)
            return self.fresh(scope)(kind)
        return _path(facts, v)

    def cells(self, s, facts):
        if s in ("", "-"):
            return None
        out = {}
        for item in s.split(","):
            k, _, v = item.partition("=")
            out[k] = self.value(v, facts)
        return out

    @staticmethod
    def rows(manifest):
        out = []
        for ln in Path(manifest).read_text().split("\n"):
            if ln and not ln.startswith("#"):
                f = (ln.split("\t") + ["-"] * 9)[:9]
                depth = len(f[0]) - len(f[0].lstrip("."))
                out.append((depth, [f[0].lstrip(".")] + f[1:]))
        return out

    def run(self, manifest):
        self.root = Path(manifest).parent
        self.block(self.rows(manifest), 0, {})
        return self.env

    def block(self, rows, depth, extra):
        i = 0
        while i < len(rows):
            d, row = rows[i]
            assert d == depth, rows[i]
            j = i + 1
            while j < len(rows) and rows[j][0] > depth:
                j += 1
            self.one(row, rows[i + 1:j], depth, extra)
            i = j

    def one(self, row, body, depth, extra):
        op, stem, section, when, factn, fresh, seq, bind, opts = row
        facts = dict(self.env)
        for s in ([] if factn in ("", "-") else factn.split("+")):
            facts.update(load_facts(s))
        facts.update(extra)
        if not _when(when, self.flags, facts):
            return
        o = {} if opts in ("", "-") else json.loads(opts)
        if op == "foreach":
            for x in _path(facts, o["over"]):
                self.block(body, depth + 1, dict(extra, **{o.get("as", "it"): x}))
            return
        assert not body, "body under " + op
        g = self.E.g
        sec = _section(section, self.flags)
        kw = {}
        if "classes" in o:
            kw["classes"] = _path(facts, o["classes"])
        if "domain" in o:
            kw["domain"] = range(*o["domain"])
        res = None
        if op == "template":
            res = install_template(g, self.root, stem, facts, self.fresh(fresh),
                                   bindings=self.cells(bind, facts), sequences=self.cells(seq, facts),
                                   section=sec, mode=o.get("mode", "r"),
                                   overlay=o.get("overlay", False), **kw)
        elif op == "rows":
            res = install(g, self.root, stem, bindings=self.cells(bind, facts),
                          sequences=self.cells(seq, facts), section=sec, **kw)
        elif op == "table":
            res = install_rows(g, self.root / stem, self.cells(seq, facts),
                               bindings=self.cells(bind, facts), section=sec,
                               mode=o.get("mode", "r"), **kw)
        elif op == "call":
            sub = Run(self.E, self.P, self.flags, dict(self.env, **(self.cells(bind, facts) or {})))
            res = sub.run(self.root / (stem + "-manifest.tsv"))
        elif op == "label":
            g.labels.update(stem.split(","))
        elif op == "assert-absent":
            assert stem not in g.st, stem
        elif op == "holder":
            k, _, name = fresh.partition(":")
            self.holders[stem] = self.P(name) if k == "P" else self.holder(name or stem)
        else:
            raise ValueError("manifest op " + op)
        if "result" in o:
            self.env[o["result"]] = res


def run(manifest, E, P, flags, env=None):
    """Assemble MANIFEST onto E.g; returns the (updated) environment."""
    return Run(E, P, dict(flags), dict(env or {})).run(manifest)
