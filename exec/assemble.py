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
         export (list of bind names also stored in env, for later rows' `$NAME`),
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
        if _headerform(path):
            _cache[path] = _header_facts(stem, path)
            return _cache[path]
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
    return re.sub(r"\{(!?)(\w+)\?([^}]*)\}",
                  lambda m: m.group(3) if bool(flags[m.group(2)]) != bool(m.group(1)) else "", s)


# ---- K2 round 2 slice D/E (lower data): generic additions ------------------
# value `@out:TEXT`: OUT byte sequence of TEXT after str-unescape (\n, \t) and
#   `{NAME}` substitution from the row's facts/env (a literal comma is `\x2c`);
# op `e`: call the executor primitive E.STEM() (no arguments), e.g. prn.
def _out(text, facts):
    t = re.sub(r"\{(\w+)\}", lambda m: str(facts[m.group(1)]), text)
    t = re.sub(r"\\x([0-9a-f]{2})", lambda m: chr(int(m.group(1), 16)), t)
    t = _cell("str", t)
    return [("OUT", c) for c in t.encode()]
# when term `state:NAME` / `!state:NAME`: NAME is (not) already a graph state
#   (idempotent shared sub-constructors: skip when installed);
# value `@str:TEXT` literal string with `{NAME}` substitution; op `set`: store the bind cells in env;
# op `fail`: exit with STEM as the message; op `py`: transitional, call
#   MODULE.install(E, **bind) for a stage constructor not yet migrated.
def _state_when(w, g):
    if "state:" not in w:
        return w
    out = []
    for t in w.split("&"):
        neg = t.startswith("!")
        if t[neg:].startswith("state:"):
            if (t[neg + 6:] in g.st) == neg:
                return "fact:__never__"
            continue
        out.append(t)
    return "&".join(out) or "-"
# ---- end K2 round 2 additions -----------------------------------------------


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
        if v.startswith("@str:"):
            return _cell("str", re.sub(r"\{(\w+)\}", lambda m: str(facts[m.group(1)]), v[5:]))
        if v.startswith("@out:"):
            return _out(v[5:], facts)
        if v.startswith("@fmt:"):
            return _fmt(v[5:], facts)
        if v.startswith("@acts:"):
            return [tuple(a) for a in _path(facts, v[6:])]
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
        when = _state_when(when, self.E.g)
        if not _when(when, self.flags, facts):
            return
        o = {} if opts in ("", "-") else json.loads(opts)
        for k, v in o.get("let", {}).items():
            facts[k] = [self.value(x, facts) for x in v] if isinstance(v, list) else self.value(v, facts)
        if stem.startswith("@"):
            stem = self.value(stem, facts)
        if op == "foreach":
            self.foreach(o, body, depth, extra, facts)
            return
        assert not body, "body under " + op
        g = self.E.g
        sec = self.value(section, facts) if section.startswith("@") else _section(section, self.flags)
        kw = {}
        if "classes" in o:
            kw["classes"] = _path(facts, o["classes"])
        if "domain" in o:
            kw["domain"] = range(*o["domain"])
        res = None
        bd = self.bindings(o, bind, facts)
        for k in o.get("export", []):
            self.env[k] = bd[k]
        if op == "template":
            res = install_template(g, self.root, stem, facts, self.fresh(fresh),
                                   bindings=bd, sequences=self.cells(seq, facts),
                                   section=sec, mode=o.get("mode", "r"),
                                   overlay=o.get("overlay", False), **kw)
        elif op == "rows":
            res = install(g, self.root, stem, bindings=bd,
                          sequences=self.cells(seq, facts), section=sec, **kw)
        elif op == "table":
            res = install_rows(g, self.root / stem, self.cells(seq, facts),
                               bindings=bd, section=sec,
                               mode=o.get("mode", "r"), **kw)
        elif op == "call":
            sub = Run(self.E, self.P, self.flags, dict(self.env, **(bd or {})))
            res = sub.run(self.root / (stem + "-manifest.tsv"))
        elif op == "set":
            self.env.update(bd or {})
        elif op == "fail":
            raise SystemExit(stem)
        elif op == "py":
            import importlib.util, sys
            if str(self.root) not in sys.path:
                sys.path.insert(0, str(self.root))
            sp = importlib.util.spec_from_file_location(self.root.name + "_" + stem, self.root / (stem + ".py"))
            mod = importlib.util.module_from_spec(sp)
            sp.loader.exec_module(mod)
            res = mod.install(self.E, **(bd or {}))
        elif op == "e":
            res = getattr(self.E, stem)()
        elif op == "label":
            g.labels.update(stem.split(","))
        elif op == "assert-absent":
            assert stem not in g.st, stem
        elif op == "assert-present":
            assert stem in g.st, stem
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


# ---- K2 round 2, slice enc/arm: generic additions -------------------------
# Header-form fact tables (exec/facts/load.py format: `# col<TAB>col` then rows)
# load as {STEM: [row dicts]}; a `name value` table also gives {STEM!: {name: value}}.
# foreach opts:  where {col: [section-format...]}  keeps rows whose col is listed;
#   chain {"entry": KEY, "start": VALUE, "next": VALUE, "result": NAME}
#   is the row-driven section loop: each row's body sees KEY bound to the
#   previous row's `next` (first row: `start`) and `next` evaluated once per
#   row; the last `next` (or `start` when no rows) is stored in env[result].
# bind opts:  bindmap PATH  (a {name: value} fact merged first),
#   freshrows [{"over": PATH, "key": FMT, "owner": FMT, "kind": FMT, "where": {col: FMT}}]
#   allocate E.P(owner).fresh(kind) per row in row order ({i} = row index),
#   accumulate NAME  (bindings persist across rows sharing NAME).
# opts let {NAME: VALUE | [VALUE...]} adds facts for this row (e.g. template facts);
# a stem starting with `@` is a value (`@fmt:...`); op assert-present STATE.
# value forms: @fmt:FMT (str.format over facts), @acts:PATH (json list -> tuples).

def _headerform(path):
    for ln in path.read_text().split("\n"):
        if ln and not ln.startswith("#"):
            return not (ln.startswith("=") or ln.startswith("@") or ln.startswith("\t"))
    return False


def _header_facts(stem, path):
    from exec.facts.load import facts as header_facts
    rows = header_facts(stem)
    d = {stem: rows}
    if rows and isinstance(rows[0], dict) and set(rows[0]) == {"name", "value"}:
        d[stem + "!"] = {r["name"]: r["value"] for r in rows}
    return d


def _fmt(f, facts):
    return f.format(**{k: v for k, v in facts.items() if isinstance(k, str) and k.isidentifier()})


def _foreach(self, o, body, depth, extra, facts):
    rows = _path(facts, o["over"])
    for col, allowed in o.get("where", {}).items():
        allowed = [_section(a, self.flags) for a in allowed]
        rows = [r for r in rows if r[col] in allowed]
    ch = o.get("chain")
    cur = ch and self.value(ch["start"], facts)
    for x in rows:
        ex = dict(extra, **{o.get("as", "it"): x})
        if ch:
            nxt = self.value(ch["next"], dict(facts, **ex))
            ex.update({ch["entry"]: cur, "next": nxt})
            cur = nxt
        self.block(body, depth + 1, ex)
    if ch:
        self.env[ch["result"]] = cur


def _bindings(self, o, bind, facts):
    acc = o.get("accumulate")
    out = dict(self.accum.setdefault(acc, {})) if acc else {}
    if "bindmap" in o:
        out.update(_path(facts, o["bindmap"]))
    for spec in o.get("freshrows", []):
        for i, r in enumerate(_path(facts, spec["over"])):
            ctx = dict(facts, i=i, **r)
            if all(str(r[c]) == _fmt(v, ctx) for c, v in spec.get("where", {}).items()):
                out[_fmt(spec["key"], ctx)] = self.E.P(_fmt(spec["owner"], ctx)).fresh(_fmt(spec["kind"], ctx))
    cells = self.cells(bind, facts)
    if cells is None and not out and not acc and "bindmap" not in o:
        return None
    out.update(cells or {})
    if acc:
        self.accum[acc] = out
    return out


_init0 = Run.__init__


def _init(self, *a, **k):
    _init0(self, *a, **k)
    self.accum = {}


Run.__init__ = _init
Run.foreach = _foreach
Run.bindings = _bindings
