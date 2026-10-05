"""Generic manifest value, binding and row helpers for assemble.Run.

This module owns the reusable TSV DSL vocabulary.  assemble.py retains the
manifest traversal and graph installation order; importing this module at the
end of assemble.py installs the helpers before any stage constructor runs.
"""
import json
import re
from pathlib import Path

import assemble as _assemble
from assemble import Run, _cell, _path, load_facts

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


# @fmt interpolation (no str.format): {NAME} or {NAME[KEY]...}; an all-digit KEY indexes a list,
# any other KEY is a dict key; {{ and }} are literal braces.  No conversions, no format specs.
_FMT = re.compile(r"\{\{|\}\}|\{([A-Za-z_]\w*)((?:\[[^\[\]{}]+\])*)\}|[{}]")


def _fmt_field(m, facts):
    t = m.group(0)
    if t in ("{{", "}}"):
        return t[0]
    if m.group(1) is None:
        raise ValueError("@fmt: stray brace")
    v = facts[m.group(1)]
    for k in re.findall(r"\[([^\]]+)\]", m.group(2)):
        v = v[int(k)] if k.isdigit() else v[k]
    return str(v)


def _fmt(f, facts):
    return _FMT.sub(lambda m: _fmt_field(m, facts), f)


def _foreach(self, o, body, depth, extra, facts):
    if o["over"].startswith("$"):
        # K2 marked change: over a dict an earlier row stored with result=NAME; rows {key, value}
        # in its order; join {"over": PATH, "on": COL, "default": {col: value | "{key}"}} merges the
        # first fact row whose COL equals key, else the default.
        rows = [{"key": k, "value": v} for k, v in self.env[o["over"][1:]].items()]
        j = o.get("join")
        if j:
            table = _path(facts, j["over"])
            def match(r):
                hit = [t for t in table if t[j["on"]] == r["key"]]
                if hit:
                    return dict(hit[0], **r)
                return dict({c: _fmt(v, r) if isinstance(v, str) else v for c, v in j["default"].items()}, **r)
            rows = [match(r) for r in rows]
    else:
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
    if o.get("cellsfirst"):
        out.update(self.cells(bind, facts) or {})
        bind = "-"
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
                if spec.get("lookup"):   # `$NAME` must be bound; a bare name falls back to itself
                    owner = out[owner[1:]] if owner.startswith("$") else out.get(owner, owner)
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
        if "keep" in o:   # keep NAME: the accumulated bindings also land in env[NAME] (a later manifest's bindmap)
            self.env[o["keep"]] = dict(out)
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
#   msgrows [FILE, FACT, PREFIX] -> SBOUT bytes as PREFIX+name, and facts[FACT] = names.
def _seqrows(self, o, facts):
    if not any(k in o for k in ("textrows", "bufrows", "msgrows")):
        return None
    sq = {}
    def rows(f):
        return [ln.split("\t") for ln in (self.root / f).read_text().splitlines()[1:]]
    for name, value in rows(o["textrows"]) if "textrows" in o else []:
        sq[name] = self.E.O(json.loads(value))
    for name, value in rows(o["bufrows"]) if "bufrows" in o else []:
        sq[name] = [("SBOUT", c) for c in json.loads(value).encode()]
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



# ---- K2 round 3, enc field chains: per-element formatted sequence names ------
# opts mapseq {"NAME{col}": {"over": PATH, "parts": [...]}}: one sequence per element x
#   of PATH, named NAME formatted over x.  A part {"splice": COL} appends the
#   sequences named in x[COL] (a list; built earlier in this mapseq or in env); a part
#   {"where": {col: VALUE}, "acts": [...]} (where optional) appends acts formatted
#   over x (a whole-cell "{col}" keeps the value's type).
_mapseq0 = Run.mapseq


def _mapseq_each(self, spec, facts, bd):
    out = _mapseq0(self, {k: v for k, v in spec.items() if "{" not in k}, facts, bd)
    cell = lambda c, x: (x[c[1:-1]] if re.fullmatch(r"\{\w+\}", c) else _fmt(c, x)) if isinstance(c, str) else c
    for key, d in ((k, v) for k, v in spec.items() if "{" in k):
        for x in _path(facts, d["over"]):
            acts = []
            for part in d["parts"]:
                if "splice" in part:
                    for n in x[part["splice"]]:
                        acts.extend(out[n] if n in out else self.env[n])
                elif all(x.get(c) == v for c, v in part.get("where", {}).items()):
                    if any(isinstance(a, (dict, str)) or a[0] in ("@out", "@bytes") for a in part["acts"]):
                        ctx = dict(facts, **x)
                        acts.extend(_mapseq0(self, {"value": [dict(acts=part["acts"])]}, ctx, bd)["value"])
                    else:
                        acts.extend(tuple(cell(c, x) for c in a) for a in part["acts"])
            out[_fmt(key, x)] = acts
    return out


Run.mapseq = _mapseq_each
# ---- end K2 round 3 enc field chains ------------------------------------------

# The manifest walker and load_facts resolve these names in assemble's module
# after this module has finished installing the shared helpers.
_assemble._when = _when
_assemble._section = _section
_assemble._out = _out
_assemble._headerform = _headerform
_assemble._header_facts = _header_facts
_assemble._fmt = _fmt
