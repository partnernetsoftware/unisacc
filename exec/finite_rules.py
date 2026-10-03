"""Expand disjoint finite observation-transition rules; no language-specific decisions.

Parameterised template tables (STEM-template.tsv, installed by install_template):
  columns  section  block  each  over  kind  a  b  c  d
  - consecutive rows with the same (section, block) form a block; the block is
    instantiated once per tuple of `each`, every row once per tuple of `over`
    (both: comma-separated fact names, `-` for none; the product is taken in the
    written order, first name slowest).  A name `x:y.f` iterates the list held in
    field f of the current value of variable y (a dependent fact list).
  - facts are passed by the caller as {name: [value, ...]}; a value is a scalar
    or a dict.  `{name}` substitutes str(value), `{name.field}` str(value[field]),
    in ANY column, including inside the JSON actions.
  - `{fresh:TAG:KIND}` allocates one fresh label per block instance and TAG
    through the caller's fresh(KIND) callback, in order of first appearance.
  - kinds: `rule` a=state b=observation c=target d=JSON actions (same syntax as
    the four-column tables; rows of one state must be contiguous in expansion);
    graph edits applied after the block's rules, in row order:
    `rename` a=OLD b=NEW (state renamed, every target OLD redirected to NEW),
    `alias` a=STATE b=TARGET (STATE goes to TARGET on every observation),
    `prepend` a=STATE d=JSON actions (actions run before every edge of STATE).
No predicate lives here: substitution, product enumeration and graph edits only.
"""
import itertools
import json
import re
from pathlib import Path


def load(path, sequences, domain=range(257), classes=None, bindings=None, section=None, lines=None):
    domain = set(domain)
    explicit, defaults = {}, {}
    for lineno, line in enumerate(path.read_text().splitlines() if lines is None else lines, 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if section is not None:
            if len(fields) != 5:
                raise ValueError(f"{path}:{lineno}: expected section plus four columns")
            if fields[0] != section:
                continue
            fields = fields[1:]
        if len(fields) != 4:
            raise ValueError(f"{path}:{lineno}: expected four columns")
        for index in (0, 2):
            value = fields[index]
            if value.startswith("$"):
                if bindings is None or type(bindings.get(value[1:])) is not str:
                    raise ValueError(f"{path}:{lineno}: missing state binding {value}")
                fields[index] = bindings[value[1:]]
        state, keys, target, encoded = fields
        if not state or not target:
            raise ValueError(f"{path}:{lineno}: empty state or target")
        actions = []
        for action in json.loads(encoded):
            if not isinstance(action, list) or not action:
                raise ValueError(f"{path}:{lineno}: invalid action")
            if action[0] == "@":
                if len(action) != 2 or action[1] not in sequences:
                    raise ValueError(f"{path}:{lineno}: unknown sequence")
                actions.extend(sequences[action[1]])
            else:
                actions.append(tuple(action))
        answer = target, actions
        row = explicit.setdefault(state, {})
        if keys == "*":
            if state in defaults:
                raise ValueError(f"{path}:{lineno}: repeated default")
            defaults[state] = answer
            continue
        if keys.startswith("@"):
            name = keys[1:]
            if classes is None or name not in classes or not classes[name]:
                raise ValueError(f"{path}:{lineno}: unknown or empty byte class")
            keys = ",".join(str(k) for k in classes[name])
        for part in keys.split(","):
            if part in domain:
                if part in row:
                    raise ValueError(f"{path}:{lineno}: overlapping symbol rules")
                row[part] = answer
                continue
            bounds = part.split("-")
            if len(bounds) not in (1, 2) or not all(x.isdecimal() for x in bounds):
                raise ValueError(f"{path}:{lineno}: invalid byte range")
            lo, hi = int(bounds[0]), int(bounds[-1])
            if lo > hi or not set(range(lo, hi + 1)) <= domain:
                raise ValueError(f"{path}:{lineno}: byte range outside domain")
            for key in range(lo, hi + 1):
                if key in row:
                    raise ValueError(f"{path}:{lineno}: overlapping byte rules")
                row[key] = answer
    if not explicit and section is None:
        raise ValueError(f"{path}: empty rules")
    for state, row in explicit.items():
        for key in sorted(domain - row.keys()):
            if state not in defaults:
                raise ValueError(f"{path}: incomplete state {state}")
            row[key] = defaults[state]
        for key, (target, actions) in row.items():
            expanded = []
            for action in actions:
                values = []
                for value in action:
                    if isinstance(value, list) and len(value) == 2 and value[0] == "constant":
                        if bindings is None or value[1] not in bindings:
                            raise ValueError(f"{path}: unknown constant binding {value[1]}")
                        value = bindings[value[1]]
                        if type(value) not in (int, str):
                            raise ValueError(f"{path}: constant binding must be an integer or symbol")
                    elif isinstance(value, list):
                        if (len(value) != 2 or value[0] != "observation" or
                            type(key) is not int or type(value[1]) is not int or
                            not 0 <= value[1] <= 63):
                            raise ValueError(f"{path}: invalid observation substitution")
                        value = key << value[1]
                    values.append(value)
                expanded.append(tuple(values))
            row[key] = target, expanded
    return explicit


def install(g, root, stem, bindings=None, sequences=None, classes=None, section=None):
    count = 0
    for suffix, mode in (("byte", "b"), ("result", "r")):
        for state, row in load(Path(root) / (stem + "-" + suffix + ".tsv"),
                                     sequences or {}, bindings=bindings, classes=classes, section=section).items():
            count += 1
            for key, (target, actions) in row.items():
                g.on(state, [key], target, actions, mode)
                g.labels.update(a[1] for a in actions if a[0] == "PUSH")
    if count == 0:
        raise ValueError(f"{stem}: no rules for section {section}")


_VAR = re.compile(r"\{([A-Za-z_][A-Za-z0-9_]*)(?:\.([A-Za-z_][A-Za-z0-9_]*))?\}")
_FRESH = re.compile(r"\{fresh:([A-Za-z0-9_]+):([A-Za-z0-9_]+)\}")


def _tuples(spec, facts, env, where):
    if spec == "-":
        yield dict(env)
        return
    names = spec.split(",")

    def walk(i, scope):
        if i == len(names):
            yield dict(scope)
            return
        name, _, source = names[i].partition(":")
        if source:
            var, _, fld = source.partition(".")
            if var not in scope or not isinstance(scope[var], dict) or fld not in scope[var]:
                raise ValueError(f"{where}: unknown dependent fact {names[i]}")
            values = scope[var][fld]
        else:
            if name not in facts:
                raise ValueError(f"{where}: unknown fact list {name}")
            values = facts[name]
        for value in values:
            yield from walk(i + 1, {**scope, name: value})
    yield from walk(0, dict(env))


def expand_template(path, facts, fresh, section=None):
    """Return (four-column rule lines, graph edits) for one section of a template."""
    rows, blocks = [], []
    for lineno, line in enumerate(Path(path).read_text().splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) != 9:
            raise ValueError(f"{path}:{lineno}: expected nine template columns")
        if section is not None and fields[0] != section:
            continue
        if blocks and blocks[-1][0] == tuple(fields[:2]):
            if blocks[-1][1] != fields[2]:
                raise ValueError(f"{path}:{lineno}: block with two `each` lists")
            blocks[-1][2].append((lineno, fields[3:]))
        else:
            blocks.append((tuple(fields[:2]), fields[2], [(lineno, fields[3:])]))
    out, edits = [], []
    for (sec, name), each, body in blocks:
        for env in _tuples(each, facts, {}, f"{path}:{sec}:{name}"):
            labels = {}

            def subst(text, scope, where):
                def fr(m):
                    if m.group(1) not in labels:
                        labels[m.group(1)] = fresh(m.group(2))
                    return labels[m.group(1)]

                def var(m):
                    if m.group(1) not in scope:
                        raise ValueError(f"{where}: unbound variable {m.group(0)}")
                    value = scope[m.group(1)]
                    if m.group(2) is not None:
                        if not isinstance(value, dict) or m.group(2) not in value:
                            raise ValueError(f"{where}: no field {m.group(0)}")
                        value = value[m.group(2)]
                    return str(value)
                return _VAR.sub(var, _FRESH.sub(fr, text))
            for lineno, (over, kind, a, b, c, d) in body:
                where = f"{path}:{lineno}"
                for scope in _tuples(over, facts, env, where):
                    a2, b2, c2, d2 = (subst(x, scope, where) for x in (a, b, c, d))
                    if kind == "rule":
                        out.append("\t".join((a2, b2, c2, d2)))
                    elif kind in ("rename", "alias", "prepend"):
                        edits.append((where, kind, a2, b2, d2))
                    else:
                        raise ValueError(f"{where}: unknown template kind {kind}")
    return out, edits


def install_template(g, root, stem, facts, fresh, bindings=None, sequences=None, classes=None,
                     section=None, mode="r"):
    path = Path(root) / (stem + "-template.tsv")
    lines, edits = expand_template(path, facts, fresh, section)
    if lines:
        for state, row in load(path, sequences or {}, bindings=bindings, classes=classes,
                               lines=lines).items():
            for key, (target, actions) in row.items():
                g.on(state, [key], target, actions, mode)
                g.labels.update(a[1] for a in actions if a[0] == "PUSH")
    for where, kind, a, b, d in edits:
        if kind == "alias":
            g.on(a, range(257), b, [], mode)
            continue
        if a not in g.st:
            raise ValueError(f"{where}: {kind} of absent state {a}")
        if kind == "rename":
            if b in g.st:
                raise ValueError(f"{where}: rename onto existing state {b}")
            items = [((b if n == a else n), v) for n, v in g.st.items()]
            g.st.clear()
            g.st.update(items)
            if a in g.labels:
                g.labels.discard(a)
                g.labels.add(b)
            for _, row in g.st.values():
                for key, (target, seq) in list(row.items()):
                    if target == a:
                        row[key] = (b, seq)
        else:
            extra = [tuple(x) for x in json.loads(d)]
            for key, (target, seq) in list(g.st[a][1].items()):
                g.st[a][1][key] = (target, g.seq(extra + list(g.seqs[seq])))
    if not lines and not edits:
        raise ValueError(f"{stem}: no template rows for section {section}")
