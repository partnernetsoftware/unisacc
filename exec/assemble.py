"""Generic manifest assembler (K2, exec/k2-design.md section 2).

A manifest `exec/<stage>/<name>-manifest.tsv` lists installs in row order:

  op  stem  section  when  facts  fresh  seq  bind  opts

op       rows | table | template | call | label | assert-absent | holder | foreach | let
         (K2 cap: <= 12 names, checked by tests/decisionledger.py --ops; holder with
         opts.cols = fresh table; assert-absent opts.present; let opts.exit)
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
opts     `-` or JSON: once (NAME: the rest of this manifest runs once per graph), mode, domain ([lo,hi)), classes (fact path), overlay,
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
def _out(text, facts):
    if text.startswith("="):
        return [("OUT", c) for c in str(_path(facts, text[1:])).encode()]
    t = re.sub(r"\{(\w+)\}", lambda m: str(facts[m.group(1)]), text)
    t = re.sub(r"\\x([0-9a-f]{2})", lambda m: chr(int(m.group(1), 16)), t)
    t = _cell("str", t)
    return [("OUT", c) for c in t.encode()]
# value `@str:TEXT` literal string with `{NAME}` substitution; op `set`: store the bind cells in env;
# op `fail`: exit with STEM as the message;
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
        spec = self.interp(spec)   # `U:{entry}`: holder named by an env label
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
        v = self.interp(v)
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
        if v.startswith("@ref:"):
            return _path(facts, _fmt(v[5:], facts))
        if v.startswith("@seqmap:"):
            return self.seqmap(*v[8:].rsplit(":", 1), facts=facts)
        if v.startswith("@acts:"):
            t = lambda x: tuple(map(t, x)) if isinstance(x, list) else x
            return [t(a) for a in _path(facts, v[6:])]
        if v.startswith("@stack:"):
            h, m, slots = v[7:].split(":", 2)
            h = self.holders[h]
            h.acts = []
            return getattr(h, m)(*slots.split("+")).acts
        if v.startswith("$$"):
            d, _, k = v[2:].partition(":")
            return self.env[d][_fmt(k, facts) if re.search(r"\{\w[\w\[\]]*\}", k) else k]
        if v.startswith("$"):
            return self.env[v[1:]]
        if v.startswith("@textf:"):
            return self.E.O(_path(facts, v[7:]))
        if v.startswith("@text:"):
            return self.E.O(json.loads(v[6:]))
        if v.startswith("fresh:"):
            scope, kind = v[6:].rsplit(":", 1)
            scope = _fmt(scope, facts) if "{" in scope else scope
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
        while i < len(rows) and not self.done:
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
            facts.update(load_facts(s[5:] if s.startswith("load:") else s))
        facts.update(extra)
        if not _when(when, self.flags, facts):
            return
        o = {} if opts in ("", "-") else json.loads(opts)
        if "once" in o:   # named result per graph: a manifest that already ran stops here
            seen = self.E.g.__dict__.setdefault("once", set())
            if o["once"] in seen:
                self.done = True
                return
            seen.add(o["once"])
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
        if "domain_keys" in o:   # K2 translator: explicit key list (fact path), not a range
            kw["domain"] = _path(facts, o["domain_keys"])
        if "tokens" in o:
            kw["classes"] = dict(kw.get("classes") or {}, **self.tokens(o["tokens"]))
        for k, v in o.get("classmap", {}).items():
            kw["classes"] = dict(kw.get("classes") or {})
            x = self.value(v, facts)
            kw["classes"][k] = x if isinstance(x, list) else [x]
        facts.update(o.get("with", {}))
        res = None
        if op in ("holder", "fresh") and "cols" in o:   # fresh table (translator spells it `fresh`)
            return self.fresh_table(stem, o)
        bd = self.bindings(o, bind, facts)
        ex = o.get("export", [])
        for k, b in (ex.items() if isinstance(ex, dict) else ((k, k) for k in ex)):
            self.env[k] = bd[b]
        sq = self.cells(seq, facts)
        tr = self.seqrows(o, facts)
        if tr is not None:
            sq = dict(tr, **(sq or {}))
        if isinstance(o.get("seqfact"), str):   # {name: [[act...]...]} fact merged into the row's sequences
            sq = dict({k: [tuple(a) for a in v] for k, v in _path(facts, o["seqfact"]).items()}, **(sq or {}))
        for sf in o.get("seqfact", []) if isinstance(o.get("seqfact"), list) else []:
            # a list: fact dicts {name: acts} merged in order over the sequences, acts deep-tupled
            t = lambda x: tuple(map(t, x)) if isinstance(x, list) else x
            sq = dict(sq or {}, **{k: [t(a) for a in v] for k, v in _path(facts, sf).items()})
        if "mapseq" in o:
            sq = dict(sq or {}, **self.mapseq(o["mapseq"], facts, bd))
        if op == "template":
            res = install_template(g, self.root, stem, facts, self.fresh(fresh),
                                   bindings=bd, sequences=sq,
                                   section=sec, mode=o.get("mode", "r"),
                                   overlay=o.get("overlay", False), **kw)
        elif op == "rows":
            res = install(g, self.root, stem, bindings=bd,
                          sequences=sq, section=sec, **kw)
        elif op == "table":
            res = install_rows(g, self.root / stem, sq,
                               bindings=bd, section=sec,
                               mode=o.get("mode", "r"), **kw)
        elif op == "call":
            sub = Run(self.E, self.P, self.flags, dict(self.env, **(bd or {})))
            res = sub.run(self.root / (stem + "-manifest.tsv"))
            if o.get("merge"):   # opts merge: the sub-manifest's env flows back (segment chains)
                self.env.update(res)
        elif op == "let":
            if "exit" in o:
                raise SystemExit(o["exit"])
            self.env.update(bd or {})
            self.env.update(sq or {})
        elif op == "label":
            g.labels.update(stem.split(","))
        elif op == "assert-absent":
            assert (stem in g.st) == bool(o.get("present")), stem
        elif op == "holder":
            k, _, name = fresh.partition(":")
            self.holders[stem] = self.P(name) if k == "P" else self.holder(name or stem)
            if "cur" in o:
                self.holders[stem].cur = o["cur"]
        else:
            raise ValueError("manifest op " + op)
        if "result" in o:
            self.env[o["result"]] = res


# --- K2 round 2, enc/objectplan slice: generic additions -------------------------
# value  `{NAME}` inside a cell is replaced by str(env[NAME]) (only names in env);
#        `@out:TEXT` is the OUT byte sequence of TEXT.
# op     fresh: stem = TSV next to the manifest; opts {"cols": [key, kind, owner]
#        column indices, "where": [[col, value], ...] (optional), "scope": "P" | "U",
#        "owner": format over {owner} {key} (optional)}; allocates one label per
#        selected row in row order and stores it in env[key].
# opts   with: literal JSON dict merged into the facts of this row.
#        mapseq: {SEQ: [{"over": factpath, "acts": [[...], ...]}, ...]} builds
#        sequence SEQ by instantiating each act per element: a cell "{f}" is the
#        element's field f, "$N" a bind/env/fact value, other text formatted
#        with the element's fields.
def _interp_impl(self, v):
    return re.sub(r"\{(\w+)\}", lambda m: str(self.env[m.group(1)]) if m.group(1) in self.env else m.group(0), v)


def _fresh_table(self, stem, o):
    ki, kd, ko = o["cols"]
    w = o.get("where", [])
    for ln in (self.root / stem).read_text().split("\n"):
        f = ln.split("\t")
        if not ln or ln.startswith("#") or any(f[c] != v for c, v in w):
            continue
        owner = o.get("owner", "{owner}").format(owner=f[ko], key=f[ki])
        if o.get("scope", "U") == "P":
            self.env[f[ki]] = self.P(owner).fresh(f[kd])
        else:
            self.env[f[ki]] = self.E.P.fresh(self.holder(owner), f[kd])


def _mapseq(self, spec, facts, bd):
    # a part without "over" emits its acts once; an act {"over": FIELD, "as": NAME,
    # "acts": [...]} repeats per element of the current element's FIELD (depth 2);
    # an act ["@bytes", TEXT] is one SBOUT per byte of the formatted TEXT;
    # a part's "where" {col: FMT} keeps elements whose col equals FMT over the row facts.
    def cell(c, x):
        m = re.fullmatch(r"\{(\w+)\}", c) if isinstance(c, str) else None
        if m:
            return x[m.group(1)]
        if isinstance(c, str) and c.startswith("$"):
            n = c[1:]
            return (bd or {})[n] if n in (bd or {}) else self.env[n] if n in self.env else facts[n]
        return c.format(**x) if isinstance(c, str) else c

    def emit(acts, a, x):
        if isinstance(a, dict):
            for y in x[a["over"]]:
                for b in a["acts"]:
                    emit(acts, b, dict(x, **{a.get("as", "it"): y}))
        elif isinstance(a, str) and a.startswith("$"):
            acts.extend(self.env[a[1:]])
        elif a[0] == "@out":
            acts.extend(("OUT", c) for c in a[1].format(**x).encode())
        elif a[0] == "@bytes":
            acts.extend(("SBOUT", c) for c in a[1].format(**x).encode())
        else:
            acts.append(tuple(cell(c, x) for c in a))

    out = {}
    for name, parts in spec.items():
        acts = []
        for part in parts:
            for x in (_path(facts, part["over"]) if "over" in part else [facts]):
                if any(str(x[c]) != _fmt(v, facts) for c, v in part.get("where", {}).items()):
                    continue
                ctx = dict(facts, **x) if isinstance(x, dict) and x is not facts else facts
                for a in part["acts"]:
                    emit(acts, a, ctx)
        out[name] = acts
    return out


Run.interp, Run.fresh_table, Run.mapseq = _interp_impl, _fresh_table, _mapseq
# --- end K2 round 2 enc/objectplan slice ---------------------------------------


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
# foreach opts pre [[NAME, VALUE]...] evaluated per row in order (before chain next).
# @ref:FMT  fact path after formatting;  @seqmap:LIST:TEMPLATE  for each x of the
#   (formatted) fact path LIST, the actions of fact TEMPLATE (json list): an
#   action ["OP", "{x}", ...] gets x substituted (whole-cell "{x}" keeps type),
#   a string "$NAME" splices the env sequence NAME.
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
        for r in rows:
            d.setdefault(r["name"], r["value"])
    return d


def _fmt(f, facts):
    return f.format(**{k: v for k, v in facts.items() if isinstance(k, str) and k.isidentifier()})


def _foreach(self, o, body, depth, extra, facts):
    rows = _path(facts, _fmt(o["over"], facts) if "{" in o["over"] else o["over"])
    for col, allowed in o.get("where", {}).items():
        allowed = [_fmt(_section(a, self.flags), facts) for a in allowed]
        rows = [r for r in rows if r[col] in allowed]
    ch = o.get("chain")
    cur = ch and self.value(ch["start"], facts)
    for x in rows:
        ex = dict(extra, **{o.get("as", "it"): x})
        for k, v in o.get("pre", []):
            ex[k] = self.value(v, dict(facts, **ex))
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
    for bm in ([o["bindmap"]] if isinstance(o.get("bindmap"), str) else
               o["bindmap"] if isinstance(o.get("bindmap"), list) else []):   # a list merges several fact dicts
        out.update(_path(facts, self.interp(bm)))
    if isinstance(o.get("freshrows"), str):
        file, _, part = o["freshrows"].partition("@")   # FILE@PART keeps rows whose part/section column is PART
        lines = (self.root / file).read_text().splitlines()
        col = {c: i for i, c in enumerate(lines[0].lstrip("# ").split("\t"))}
        ko, kk = col.get("owner", col.get("prefix")), col.get("key", col.get("name"))
        kp = col.get("part", col.get("section"))
        for ln in lines[1:]:
            f = ln.split("\t")
            if part and f[kp] != part:
                continue
            out[f[kk]] = self.E.P.fresh(self.holder(f[ko]), f[col["kind"]])
    for spec in ([] if isinstance(o.get("freshrows"), str) else o.get("freshrows", [])):
        # "file": a header-line TSV next to the manifest instead of "over" (its "where" formats over
        # the facts only); "lookup": an owner naming a binding so far (or a fact) becomes its value;
        # "holder": allocate through an unregistered holder (no P construction per row)
        if "file" in spec:
            ls = (self.root / spec["file"]).read_text().splitlines()
            hd = ls[0].lstrip("# ").split("\t")
            src = [dict(zip(hd, ln.split("\t"))) for ln in ls[1:] if ln and not ln.startswith("#")]
        else:
            src = _path(facts, spec["over"])
        for i, r in enumerate(src):
            ctx = dict(facts, i=i, **r)
            wctx = dict(facts, i=i) if "file" in spec else ctx
            if all(str(r[c]) == _fmt(v, wctx) for c, v in spec.get("where", {}).items()):
                owner = _fmt(spec["owner"], ctx)
                if spec.get("lookup"):
                    owner = out.get(owner, owner)
                kind = _fmt(spec["kind"], ctx)
                out[_fmt(spec["key"], ctx)] = (self.E.P.fresh(self.holder(owner), kind) if spec.get("holder")
                                               else self.E.P(owner).fresh(kind))
    cells = self.cells(bind, facts)
    if cells is None and not out and not acc and "bindmap" not in o:
        return None
    out.update(cells or {})
    if isinstance(o.get("bindmap"), dict):
        out.update({k: self.value(v, facts) for k, v in o["bindmap"].items()})
    if acc:
        self.accum[acc] = out
    return out


_init0 = Run.__init__


def _init(self, *a, **k):
    _init0(self, *a, **k)
    self.accum = {}
    self.done = False


Run.__init__ = _init
Run.foreach = _foreach
Run.bindings = _bindings


def _seqmap(self, lst, tmpl, facts):
    out = []
    for x in _path(facts, _fmt(lst, facts)):
        for a in _path(facts, tmpl):
            if isinstance(a, str):
                out += self.env[a[1:]]
            else:
                out.append(tuple(x if c == "{x}" else c for c in a))
    return out


Run.seqmap = _seqmap


# seq tables next to the manifest (header line skipped, rows `name<TAB>json`):
#   textrows FILE -> E.O(text); bufrows FILE -> SBOUT bytes;
#   stackrows [FILE, HOLDER] -> P(HOLDER).METHOD(*slots).acts for json [METHOD, slots];
#   msgrows [FILE, FACT, PREFIX] -> SBOUT bytes as PREFIX+name, and facts[FACT] = names.
def _seqrows(self, o, facts):
    if not any(k in o for k in ("textrows", "bufrows", "stackrows", "msgrows")):
        return None
    sq = {}
    def rows(f):
        return [ln.split("\t") for ln in (self.root / f).read_text().splitlines()[1:]]
    for name, value in rows(o["textrows"]) if "textrows" in o else []:
        sq[name] = self.E.O(json.loads(value))
    for name, value in rows(o["bufrows"]) if "bufrows" in o else []:
        sq[name] = [("SBOUT", c) for c in json.loads(value).encode()]
    if "stackrows" in o:
        p = self.P(o["stackrows"][1])
        for name, value in rows(o["stackrows"][0]):
            method, slots = json.loads(value)
            p.acts = []
            sq[name] = getattr(p, method)(*slots).acts
    if "msgrows" in o:
        file, fact, prefix = o["msgrows"]
        names = []
        for name, value in rows(file):
            names.append(name)
            sq[prefix + name] = [("SBOUT", c) for c in json.loads(value).encode()]
        facts[fact] = names
    return sq


Run.seqrows = _seqrows


# opts tokens {class: token} or FILE (rows `class<TAB>token`, header skipped):
#   one-element classes of token codes (identifier/number/string or E.TK[token]).
def _tokens(self, spec):
    if isinstance(spec, str):
        spec = dict(ln.split("\t") for ln in (self.root / spec).read_text().splitlines()[1:])
    lit = {"number": self.E.TK_NUM, "string": self.E.TK_STR, "identifier": self.E.TK_ID}
    return {k: [lit[t] if t in lit else self.E.TK[t]] for k, t in spec.items()}


Run.tokens = _tokens
