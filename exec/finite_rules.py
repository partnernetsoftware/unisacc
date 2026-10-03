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
  - call chains: `{fresh@row:TAG:KIND}` allocates a NEW label for every row
    instance (one tuple of a row), and `{prev:TAG}` names the label most
    recently allocated for TAG in this block instance (it is substituted before
    the row's own fresh@row, so `{prev:R}` -> MS.write64 PUSH `{fresh@row:R:r}`
    continues a chain one call further).
  - loop bodies: `over` may start with k dots (nesting depth k).  A row whose
    spec is `=` belongs to the loop opened by the nearest earlier row of the
    same depth; rows of greater depth nest inside it.  Each tuple of the loop
    runs its whole body (that row, its `=` rows, nested loops) in written order,
    so a chain can cross a per-fact variable-length loop.
  - kinds: `rule` (or `rule:MODE`, overriding the caller's mode for that state) a=state b=observation c=target d=JSON actions (same syntax as
    the four-column tables; rows of one state must be contiguous in expansion);
    graph edits applied after the block's rules, in row order:
    `rename` a=OLD b=NEW (state renamed, every target OLD redirected to NEW,
    every PUSH OLD argument in an action sequence rewritten to PUSH NEW),
    `alias` a=STATE b=TARGET (STATE goes to TARGET on every observation),
    `prepend` a=STATE d=JSON actions (actions run before every edge of STATE),
    `redirect` a=STATE b=OBSERVATION c=TARGET d=JSON actions (replace one existing edge),
    `insert-edge` a=STATE b=KEY c=TARGET d=JSON actions (KEY must be absent),
    `fill-edge` a=STATE b=KEY c=TARGET d=JSON actions (existing KEY has precedence;
    an absent STATE is created with the caller's mode, holding only KEY),
    `append` a=STATE d=JSON actions (actions run after every edge of STATE),
    `drop-edge` a=STATE b=KEY (KEY must exist),
    `set-mode` a=STATE b=OLD c=NEW (OLD must match).
    `copy-state` a=SOURCE b=NEW (shallow alias, no target rewriting),
    `move-state` a=OLD b=NEW (move without target rewriting); `move` is its short form;
    `drop-state` a=STATE (STATE must exist; removed with its edges, no target rewriting);
    `copy` a=NEW b=SOURCE is the destination-first hook form of `copy-state`;
    `clone-push` a=SOURCE b=NEW c=EXPECTED_TARGET d=NEW_CONTINUATION.
    `label` a=STATE (STATE joins the graph's return-label set);
    `copy-replace` a=SOURCE b=NEW c=JSON action d=JSON actions (copy SOURCE with every
    occurrence of the action c replaced by the actions d; c must occur in SOURCE).
    `rewrite-tail` a=JSON action patterns c=TARGET (empty: keep) d=JSON actions:
    on every edge of every state whose action sequence ends with actions
    matching the patterns (each pattern a prefix of its action tuple), those
    trailing actions are replaced by d and the target by c.
  - kind `fresh` emits nothing: its columns are only substituted, so a row
    `fresh  {fresh@row:TAG:KIND}` allocates a label at that point of row order
    (for chains that allocate a label before the edge that names it).
  - in graph edits, a column a/b/c of the form `$NAME` names the caller's state
    binding NAME (as in rule rows).
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


def install_rows(g, path, sequences=None, domain=range(257), bindings=None,
                 classes=None, section=None, mode="r"):
    rows = load(Path(path), sequences or {}, domain=domain, bindings=bindings,
                classes=classes, section=section)
    for state, row in rows.items():
        for key, (target, actions) in row.items():
            g.on(state, [key], target, actions, mode)
            g.labels.update(a[1] for a in actions if a[0] == "PUSH")
    return len(rows)


def install(g, root, stem, bindings=None, sequences=None, classes=None, section=None,
            domain=range(257)):
    count = 0
    for suffix, mode in (("byte", "b"), ("result", "r")):
        count += install_rows(g, Path(root) / (stem + "-" + suffix + ".tsv"),
                              sequences, domain=domain, bindings=bindings,
                              classes=classes, section=section, mode=mode)
    if count == 0:
        raise ValueError(f"{stem}: no rules for section {section}")


_VAR = re.compile(r"\{([A-Za-z_][A-Za-z0-9_]*)(?:\.([A-Za-z_][A-Za-z0-9_]*))?\}")
_FRESH = re.compile(r"\{fresh:([A-Za-z0-9_]+):([A-Za-z0-9_]+)\}")
_ROWFRESH = re.compile(r"\{fresh@row:([A-Za-z0-9_]+):([A-Za-z0-9_]+)\}")
_PREV = re.compile(r"\{prev:([A-Za-z0-9_]+)\}")


def _loops(body, where):
    """Group (lineno, fields) rows into nested loops by leading dots of `over`."""
    root, stack = [], []          # stack of (depth, loop) ; loop = [spec, items]
    for lineno, fields in body:
        over = fields[0]
        depth = len(over) - len(over.lstrip("."))
        spec = over[depth:]
        while stack and stack[-1][0] > depth:
            stack.pop()
        if spec == "=":
            if not stack or stack[-1][0] != depth:
                raise ValueError(f"{where}:{lineno}: `=` row without an open loop of depth {depth}")
            stack[-1][1][1].append(("row", lineno, fields[1:]))
            continue
        if stack and stack[-1][0] == depth:
            stack.pop()
        if depth and (not stack or stack[-1][0] != depth - 1):
            raise ValueError(f"{where}:{lineno}: nested loop without an enclosing loop")
        loop = [spec, [("row", lineno, fields[1:])]]
        (stack[-1][1][1] if stack else root).append(("loop", loop))
        stack.append((depth, loop))
    return root


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
    out, edits, modes = [], [], {}
    for (sec, name), each, body in blocks:
        for env in _tuples(each, facts, {}, f"{path}:{sec}:{name}"):
            labels, prev = {}, {}

            def subst(text, scope, where, rowlabels):
                def pv(m):
                    if m.group(1) not in prev:
                        raise ValueError(f"{where}: no previous label for {m.group(0)}")
                    return prev[m.group(1)]

                def rf(m):
                    if m.group(1) not in rowlabels:
                        rowlabels[m.group(1)] = fresh(m.group(2))
                    return rowlabels[m.group(1)]

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
                return _VAR.sub(var, _FRESH.sub(fr, _ROWFRESH.sub(rf, _PREV.sub(pv, text))))

            def run(items, scope):
                for tag, *item in items:
                    if tag == "loop":
                        spec, inner = item[0]
                        for sub in _tuples(spec, facts, scope, f"{path}:{sec}:{name}"):
                            run(inner, sub)
                        continue
                    lineno, (kind, a, b, c, d) = item
                    where = f"{path}:{lineno}"
                    rowlabels = {}
                    a2, b2, c2, d2 = (subst(x, scope, where, rowlabels) for x in (a, b, c, d))
                    prev.update(rowlabels)
                    if kind == "rule" or kind.startswith("rule:"):
                        if kind != "rule":
                            modes[a2] = kind[5:]
                        out.append("\t".join((a2, b2, c2, d2)))
                    elif kind == "fresh":
                        if b2:
                            prev[b2] = a2
                    elif kind in ("append", "rewrite-tail", "rename", "alias", "prepend", "redirect", "insert-edge", "fill-edge", "drop-edge", "set-mode", "copy-state", "move-state", "drop-state", "clone-push", "copy-replace", "label", "move", "copy"):
                        kind = {"move": "move-state"}.get(kind, kind)
                        edits.append((where, kind, a2, b2, c2, d2))
                    else:
                        raise ValueError(f"{where}: unknown template kind {kind}")
            run(_loops(body, f"{path}"), env)
    return out, edits, modes


def install_template(g, root, stem, facts, fresh, bindings=None, sequences=None, classes=None,
                     section=None, mode="r", domain=range(257)):
    path = Path(root) / (stem + "-template.tsv")
    lines, edits, modes = expand_template(path, facts, fresh, section)
    if lines:
        for state, row in load(path, sequences or {}, domain, bindings=bindings, classes=classes,
                               lines=lines).items():
            for key, (target, actions) in row.items():
                g.on(state, [key], target, actions, modes.get(state, mode))
                g.labels.update(a[1] for a in actions if a[0] == "PUSH")
    for where, kind, a, b, c, d in edits:
        if bindings is not None:
            a, b, c = (bindings[x[1:]] if x.startswith("$") and type(bindings.get(x[1:])) is str
                       else x for x in (a, b, c))
        if kind == "alias":
            g.on(a, range(257), b, [], mode)
            continue
        if kind == "label":
            g.labels.add(a)
            continue
        if kind == "rewrite-tail":
            pattern = json.loads(a)
            replacement = [tuple(x) for x in json.loads(d)]
            n = len(pattern)
            for _, row in list(g.st.values()):
                for key, (target, seq) in list(row.items()):
                    acts = list(g.seqs[seq])
                    if n and len(acts) >= n and all(
                            list(x[:len(p)]) == p for x, p in zip(acts[-n:], pattern)):
                        row[key] = (c or target, g.seq(acts[:-n] + replacement))
            continue
        if kind == "copy":
            if a in g.st or b not in g.st:
                raise ValueError(f"{where}: copy precondition failed {b} -> {a}")
            g.st[a] = g.st[b]
            continue
        if a not in g.st and kind != "fill-edge":
            raise ValueError(f"{where}: {kind} of absent state {a}")
        if kind == "drop-state":
            del g.st[a]
            continue
        if kind in ("copy-state", "move-state"):
            if b in g.st:
                raise ValueError(f"{where}: {kind} onto existing state {b}")
            g.st[b] = g.st[a] if kind == "copy-state" else g.st.pop(a)
            continue
        if kind == "copy-replace":
            if b in g.st:
                raise ValueError(f"{where}: copy onto existing state {b}")
            old, new = tuple(json.loads(c)), [tuple(x) for x in json.loads(d)]
            smode, source = g.st[a]
            if not any(old in g.seqs[q] for _, q in source.values()):
                raise ValueError(f"{where}: copy-replace action absent from {a}")
            g.st[b] = (smode, {key: (target, g.seq([y for x in g.seqs[q] for y in (new if x == old else [x])]))
                               for key, (target, q) in source.items()})
            continue
        if kind == "clone-push":
            if b in g.st:
                raise ValueError(f"{where}: clone onto existing state {b}")
            smode, source = g.st[a]
            copied = {}
            for key, (target, seqid) in source.items():
                acts = list(g.seqs[seqid])
                if target != c or sum(x[0] == "PUSH" for x in acts) != 1:
                    raise ValueError(f"{where}: clone-push precondition failed at {a}/{key}")
                copied[key] = (target, g.seq([(x[0], d) if x[0] == "PUSH" else x for x in acts]))
            g.st[b] = [smode, copied]
            g.labels.add(d)
            continue
        if kind in ("insert-edge", "fill-edge", "drop-edge"):
            if not b.isdecimal() or not 0 <= int(b) <= 256:
                raise ValueError(f"{where}: invalid edge key {b}")
            key = int(b)
            present = a in g.st and key in g.st[a][1]
            if kind == "drop-edge":
                if not present:
                    raise ValueError(f"{where}: absent edge {a}/{key}")
                del g.st[a][1][key]
            else:
                if present and kind == "insert-edge":
                    raise ValueError(f"{where}: duplicate edge {a}/{key}")
                if present:
                    continue
                row = load(path, sequences or {}, domain=[key], bindings=bindings,
                           lines=["\t".join((a, b, c, d))])
                target, actions = row[a][key]
                g.on(a, [key], target, actions, g.st[a][0] if a in g.st else mode)
                g.labels.update(x[1] for x in actions if x[0] == "PUSH")
            continue
        if kind == "set-mode":
            if g.st[a][0] != b or c not in ("b", "r"):
                raise ValueError(f"{where}: mode precondition failed for {a}")
            g.st[a] = (c, g.st[a][1])
            continue
        if kind == "rename":
            if b in g.st:
                raise ValueError(f"{where}: rename onto existing state {b}")
            items = [((b if n == a else n), v) for n, v in g.st.items()]
            g.st.clear()
            g.st.update(items)
            if a in g.labels:
                g.labels.discard(a)
                g.labels.add(b)
            pushes = {}
            for _, row in g.st.values():
                for key, (target, seq) in list(row.items()):
                    if seq not in pushes:
                        acts = g.seqs[seq]
                        pushes[seq] = (g.seq([(("PUSH", b) if x[0] == "PUSH" and x[1] == a else x) for x in acts])
                                       if any(x[0] == "PUSH" and x[1] == a for x in acts) else seq)
                    row[key] = ((b if target == a else target), pushes[seq])
        elif kind == "redirect":
            key = int(b) if b.isdecimal() else b
            if key not in g.st[a][1]:
                raise ValueError(f"{where}: redirect of absent edge {a} {key}")
            g.st[a][1][key] = (c, g.seq([tuple(x) for x in json.loads(d)]))
        else:
            extra = [tuple(x) for x in json.loads(d)]
            for key, (target, seq) in list(g.st[a][1].items()):
                acts = list(g.seqs[seq])
                g.st[a][1][key] = (target, g.seq(acts + extra if kind == "append" else extra + acts))
    if not lines and not edits:
        raise ValueError(f"{stem}: no template rows for section {section}")
