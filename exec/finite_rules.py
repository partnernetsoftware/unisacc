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
    `split-at` a=JSON action sequence b=RESUME c=TARGET: on every edge whose action
    sequence contains a (first occurrence), the actions from a on are replaced by
    PUSH RESUME and the target by TARGET; RESUME (a return label, mode r) takes on
    every observation the first such edge's target and the actions after a.
    `rewrite-tail` a=JSON action patterns c=TARGET (empty: keep) d=JSON actions:
    on every edge of every state whose action sequence ends with actions
    matching the patterns (each pattern a prefix of its action tuple), those
    trailing actions are replaced by d and the target by c; b (`-` or empty:
    none) is a comma-separated set of states left untouched.
    `insert-after` a=JSON list of action patterns d=JSON list of action lists
    (parallel): on every edge of every state, after each action matching
    pattern i (a prefix of its tuple; first matching pattern wins) the
    actions d[i] are inserted; b as for rewrite-tail.
    `group-tail` a=JSON [OP, PREFIX] b as for rewrite-tail c=LABEL_PREFIX: on every
    edge whose last action is (OP, S) with S starting with PREFIX, that action
    is dropped and the target becomes LABEL_PREFIX<n>, n numbering the distinct
    S in first-occurrence (graph) order; install_template returns {S: label}.
  - kind `fresh` emits nothing: its columns are only substituted, so a row
    `fresh  {fresh@row:TAG:KIND}` allocates a label at that point of row order
    (for chains that allocate a label before the edge that names it).
  - in graph edits, a column a/b/c of the form `$NAME` names the caller's state
    binding NAME (as in rule rows).
Overlay tables (opt-in `overlay=True` of load/install_template): rows are applied
in written order and a later row overrides an earlier edge in place (its key keeps
its first position); a key field that is exactly `*` adds the still-absent keys of
the domain in sorted order at that point; `A&B&...` is the intersection of key
sets (`*` = the whole domain, `@class`, numbers, ranges; first operand's order);
an empty intersection adds nothing; states need not be total (the caller checks).
Fact helpers: prefix_facts(words) exposes the prefixes of a word list as trie
facts; rule_facts(path) exposes the rows of a four-column table as facts.
No predicate lives here: substitution, product enumeration and graph edits only.
"""
import itertools
import json
import os
import re
from pathlib import Path


def _keyset(path, lineno, part, domain, classes):
    if part == "*":
        return sorted(domain, key=lambda k: (type(k) is str, k))
    if part.startswith("@"):
        name = part[1:]
        if classes is None or name not in classes or not classes[name]:
            raise ValueError(f"{path}:{lineno}: unknown or empty byte class")
        return list(classes[name])
    keys = []
    for item in part.split(","):
        if item in domain:
            keys.append(item)
            continue
        bounds = item.split("-")
        if len(bounds) not in (1, 2) or not all(x.isdecimal() for x in bounds):
            raise ValueError(f"{path}:{lineno}: invalid byte range")
        lo, hi = int(bounds[0]), int(bounds[-1])
        if lo > hi or not set(range(lo, hi + 1)) <= domain:
            raise ValueError(f"{path}:{lineno}: byte range outside domain")
        keys.extend(range(lo, hi + 1))
    return keys


def load(path, sequences, domain=range(257), classes=None, bindings=None, section=None, lines=None,
         overlay=False, line_numbers=None, provenance=None):
    domain = set(domain)
    explicit, defaults = {}, {}
    row_log = os.environ.get("UNISACC_ROW_LOG")
    row_sources = {} if row_log else None
    default_sources = {} if row_log else None
    source_lines = path.read_text().splitlines() if lines is None else lines
    if line_numbers is not None and len(line_numbers) != len(source_lines):
        raise ValueError(f"{path}: source line count differs from expanded rule count")
    for generated_line, line in enumerate(source_lines, 1):
        lineno = generated_line if line_numbers is None else line_numbers[generated_line - 1]
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
        if overlay:
            if keys == "*":
                chosen = [k for k in _keyset(path, lineno, "*", domain, classes) if k not in row]
            else:
                operands = [_keyset(path, lineno, part, domain, classes) for part in keys.split("&")]
                chosen = [k for k in operands[0] if all(k in other for other in operands[1:])]
            for key in chosen:
                row[key] = answer
                if row_sources is not None:
                    row_sources[state, key] = lineno
            continue
        if keys == "*":
            if state in defaults:
                raise ValueError(f"{path}:{lineno}: repeated default")
            defaults[state] = answer
            if default_sources is not None:
                default_sources[state] = lineno
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
                if row_sources is not None:
                    row_sources[state, part] = lineno
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
                if row_sources is not None:
                    row_sources[state, key] = lineno
    if not explicit and section is None:
        raise ValueError(f"{path}: empty rules")
    for state, row in explicit.items():
        for key in ([] if overlay else sorted(domain - row.keys())):
            if state not in defaults:
                raise ValueError(f"{path}: incomplete state {state}")
            row[key] = defaults[state]
            if row_sources is not None:
                row_sources[state, key] = default_sources[state]
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
                        if value > (1 << 63) - 1:   # table immediates are signed i64: same u64 bits
                            value -= 1 << 64
                    values.append(value)
                expanded.append(tuple(values))
            row[key] = target, expanded
    if provenance is not None and row_sources is not None:
        provenance.update(row_sources)
    elif row_log:
        root = Path(__file__).resolve().parent.parent
        source_path = Path(path).resolve()
        try:
            source_name = str(source_path.relative_to(root))
        except ValueError:
            source_name = str(source_path)
        with open(row_log, "a", encoding="utf-8") as out:
            for state, row in explicit.items():
                for key in row:
                    out.write(f"{state}\t{key}\t{source_name}\t{row_sources[state, key]}\n")
    return explicit


class _TrackedRow(dict):
    """Observe template edits only when provenance logging is enabled."""
    def __init__(self, g, state, row):
        super().__init__(row)
        self.g, self.state = g, state

    def __setitem__(self, key, edge):
        super().__setitem__(key, edge)
        origin = getattr(self.g, "_active_row_origin", None)
        if origin is not None:
            self.g._row_origins[self.state, key] = (edge, *origin)

    def __delitem__(self, key):
        super().__delitem__(key)
        if hasattr(self.g, "_row_origins"):
            self.g._row_origins.pop((self.state, key), None)


def _track_row(g, state):
    mode, row = g.st[state]
    if isinstance(row, _TrackedRow) and row.state == state:
        return
    tracked = _TrackedRow(g, state, row)
    g.st[state] = [mode, tracked] if isinstance(g.st[state], list) else (mode, tracked)
    origin = getattr(g, "_active_row_origin", None)
    if origin is not None:
        for key, edge in tracked.items():
            g._row_origins[state, key] = (edge, *origin)


def _track_all_rows(g):
    for state in list(g.st):
        _track_row(g, state)


def _row_origin(g, state, key, before, after, source, path):
    """Remember the installed edge and its declaration; graph edits are audited at finish."""
    if not os.environ.get("UNISACC_ROW_LOG"):
        return
    if (before is None or before != after or getattr(g, "states", None) is g.st or
            (state, key) not in getattr(g, "_row_origins", {})):
        if not hasattr(g, "_row_origins"):
            g._row_origins = {}
        g._row_origins[state, key] = (after, str(path), source)
    _track_row(g, state)


def write_final_row_log(g):
    """Emit one owner per final live edge; edited edges get an explicit synthetic owner."""
    row_log = os.environ.get("UNISACC_ROW_LOG")
    if not row_log:
        return
    origins = getattr(g, "_row_origins", {})
    with open(row_log, "w", encoding="utf-8") as out:
        for state, (_, row) in g.st.items():
            for key, edge in row.items():
                actions = g.seqs[edge[1]]
                if any(a and a[0] == "REJECT" and len(a) > 1 and a[1] == "unreachable" for a in actions):
                    continue
                remembered = origins.get((state, key))
                path, line = (remembered[1], remembered[2]) if remembered and remembered[0] == edge else ("synthetic", 0)
                out.write(f"{state}\t{key}\t{path}\t{line}\n")


def install_rows(g, path, sequences=None, domain=range(257), bindings=None,
                 classes=None, section=None, mode="r", skip=(), ordered=False):
    """skip: states not installed; ordered: keys written in domain order (else rule order)."""
    origins = {} if os.environ.get("UNISACC_ROW_LOG") else None
    rows = load(Path(path), sequences or {}, domain=domain, bindings=bindings,
                classes=classes, section=section, provenance=origins)
    for state, row in rows.items():
        if state in skip:
            continue
        for key in (domain if ordered else list(row)):
            target, actions = row[key]
            before = g.st.get(state, (None, {}))[1].get(key)
            g.on(state, [key], target, actions, mode)
            if origins is not None:
                _row_origin(g, state, key, before, g.st[state][1][key], origins[state, key], path)
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


def expand_template(path, facts, fresh, section=None, origins=None):
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
                    # K2 marked change: a list value (e.g. a named action sequence) renders as JSON, not repr
                    return json.dumps([list(x) if isinstance(x, tuple) else x for x in value]) if isinstance(value, list) else str(value)
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
                        if origins is not None:
                            origins.append(lineno)
                    elif kind == "fresh":
                        if b2:
                            prev[b2] = a2
                    elif kind in ("append", "split-at", "rewrite-tail", "insert-after", "companion", "group-tail", "rename", "alias", "prepend", "redirect", "insert-edge", "fill-edge", "drop-edge", "set-mode", "copy-state", "move-state", "drop-state", "clone-push", "copy-replace", "label", "move", "copy"):
                        kind = {"move": "move-state"}.get(kind, kind)
                        edits.append((where, kind, a2, b2, c2, d2))
                    else:
                        raise ValueError(f"{where}: unknown template kind {kind}")
            run(_loops(body, f"{path}"), env)
    return out, edits, modes


def prefix_facts(words):
    """Trie facts of an ordered word list (None entries are skipped): returns
    (root, prefixes).  Every non-empty prefix is a dict -- name, last (code of its
    final character), word ('yes' when it is itself a word, else 'no'), longest
    (index in `words` of the longest word that is a prefix of it, -1 if none),
    children (the prefix dicts one character longer, sorted by name); prefixes
    are sorted by name, root is the dict of the empty prefix."""
    names = sorted({w[:n] for w in words if w is not None for n in range(1, len(w) + 1)})
    nodes = {"": {"name": "", "children": []}}
    for name in names:
        best = -1
        for index, w in enumerate(words):
            if w is not None and name.startswith(w) and (best < 0 or len(w) > len(words[best])):
                best = index
        nodes[name] = {"name": name, "last": ord(name[-1]), "word": "yes" if name in words else "no",
                       "longest": best, "children": []}
        nodes[name[:-1]]["children"].append(nodes[name])
    return nodes[""], [nodes[name] for name in names]


def rule_facts(path):
    """Rows of a four-column table as facts {state: [{keys, target, actions}, ...]},
    `*` rows of a state first (so an overlay install keeps explicit keys first)."""
    out = {}
    for line in Path(path).read_text().splitlines():
        if line and not line.startswith("#"):
            state, keys, target, actions = line.split("\t")
            out.setdefault(state, []).append({"keys": keys, "target": target, "actions": actions})
    return {state: sorted(rows, key=lambda r: r["keys"] != "*") for state, rows in out.items()}


def _companions(g, rules):
    """One pass over the graph: every action matching a rule (first match by
    row order, null = any field) is followed by that rule's actions, `$k`
    standing for the action's field k, unless they already follow it. An empty
    pattern prepends to every edge. b = state globs skipped, c = state globs
    required ('-' = all)."""
    from fnmatch import fnmatch
    globs = lambda v: [] if v in ("", "-") else v.split(",")
    index, front = {}, []
    for n, (pattern, b, c, acts) in enumerate(rules):
        rule = (pattern, globs(b), globs(c), acts, n)
        if not pattern:
            front.append(rule)
        else:
            index.setdefault((pattern[0][0], pattern[0][1] if len(pattern[0]) > 1 else None), []).append(rule)
    # The result for one edge depends only on which glob pairs the state name satisfies (its
    # signature) and on the edge's seq, so it is memoised per (signature, seq).  Edges are still
    # visited in the original order and each new combination still calls g.seq in that order, so
    # sequence numbering and bytes are unchanged (0.0.24 T5: ~19 s of parse2 was this loop).
    pairs = sorted({(tuple(r[1]), tuple(r[2])) for rs in [front] + list(index.values()) for r in rs})
    per_sig = {}
    for name, (_, row) in list(g.st.items()):
        sig = tuple(not any(fnmatch(name, x) for x in b) and (not c or any(fnmatch(name, x) for x in c))
                    for b, c in pairs)
        okset = {pc for pc, v in zip(pairs, sig) if v}
        if sig not in per_sig:
            ok = lambda r: (tuple(r[1]), tuple(r[2])) in okset
            per_sig[sig] = ([y for r in front if ok(r) for y in r[3]], {}, {}, ok)
        head, cache, memo, ok = per_sig[sig]
        for key, (target, seq) in list(row.items()):
            hit = memo.get(seq)
            if hit is not None:
                row[key] = (target, hit)
                continue
            source = list(g.seqs[seq])
            out = list(head)
            for i, x in enumerate(source):
                out.append(x)
                cands = cache.get((x[0], x[1] if len(x) > 1 else None))
                if cands is None:
                    cands = [r for r in index.get((x[0], x[1] if len(x) > 1 else None), []) +
                             index.get((x[0], None), []) if ok(r)]
                    cands.sort(key=lambda r: r[4])
                    cache[(x[0], x[1] if len(x) > 1 else None)] = cands
                for pattern, _, _, acts, _ in cands:
                    p = pattern[0]
                    if len(x) >= len(p) and all(q is None or q == v for q, v in zip(p, x)):
                        extra = [tuple(x[int(v[1:])] if type(v) is str and v[:1] == "$" else v for v in y)
                                 for y in acts]
                        if extra and source[i + 1:i + 1 + len(extra)] != extra:
                            out.extend(extra)
                        break
            memo[seq] = new = g.seq(out)
            row[key] = (target, new)


def install_template(g, root, stem, facts, fresh, bindings=None, sequences=None, classes=None,
                     section=None, mode="r", domain=range(257), overlay=False):
    path = Path(root) / (stem + "-template.tsv")
    origins = []
    lines, edits, modes = expand_template(path, facts, fresh, section, origins=origins)
    if lines:
        sources = {} if os.environ.get("UNISACC_ROW_LOG") else None
        for state, row in load(path, sequences or {}, domain, bindings=bindings, classes=classes,
                               lines=lines, overlay=overlay, line_numbers=origins,
                               provenance=sources).items():
            for key, (target, actions) in row.items():
                before = g.st.get(state, (None, {}))[1].get(key)
                g.on(state, [key], target, actions, modes.get(state, mode))
                if sources is not None:
                    _row_origin(g, state, key, before, g.st[state][1][key], sources[state, key], path)
                g.labels.update(a[1] for a in actions if a[0] == "PUSH")
    if os.environ.get("UNISACC_ROW_LOG") and edits:
        if not hasattr(g, "_row_origins"):
            g._row_origins = {}
        _track_all_rows(g)
    groups = {}
    pending = []
    for ix, (where, kind, a, b, c, d) in enumerate(edits):
        if os.environ.get("UNISACC_ROW_LOG"):
            _track_all_rows(g)  # states created by the preceding edit
            edit_path, edit_line = where.rsplit(":", 1)
            g._active_row_origin = (str(Path(edit_path)), int(edit_line))
        if bindings is not None:
            a, b, c = (bindings[x[1:]] if x.startswith("$") and type(bindings.get(x[1:])) is str
                       else x for x in (a, b, c))
        if kind == "alias":
            g.on(a, range(257), b, [], mode)
            continue
        if kind == "label":
            g.labels.add(a)
            continue
        if kind == "split-at":
            cut = [tuple(x) for x in json.loads(a)]
            n = len(cut)
            for _, row in list(g.st.values()):
                for key, (target, seq) in list(row.items()):
                    acts = list(g.seqs[seq])
                    at = next((i for i in range(len(acts) - n + 1) if acts[i:i + n] == cut), None)
                    if n and at is not None:
                        g.labels.add(b)
                        row[key] = (c, g.seq(acts[:at] + [("PUSH", b)]))
                        g.on(b, range(257), target, acts[at + n:], "r")
            continue
        if kind == "rewrite-tail":
            pattern = json.loads(a)
            replacement = [tuple(x) for x in json.loads(d)]
            n = len(pattern)
            skip = set() if b in ("", "-") else set(b.split(","))
            for name, (_, row) in list(g.st.items()):
                if name in skip:
                    continue
                for key, (target, seq) in list(row.items()):
                    acts = list(g.seqs[seq])
                    if n and len(acts) >= n and all(
                            list(x[:len(p)]) == p for x, p in zip(acts[-n:], pattern)):
                        row[key] = (c or target, g.seq(acts[:-n] + replacement))
            continue
        if kind == "group-tail":
            op, prefix = json.loads(a)
            skip = set() if b in ("", "-") else set(b.split(","))
            for name, (_, row) in list(g.st.items()):
                if name in skip:
                    continue
                for key, (target, seq) in list(row.items()):
                    acts = list(g.seqs[seq])
                    if acts and acts[-1][0] == op and acts[-1][1].startswith(prefix):
                        label = groups.setdefault(acts[-1][1], c + str(len(groups)))
                        row[key] = (label, g.seq(acts[:-1]))
            continue
        if kind == "companion":
            pending.append((json.loads(a), b, c, [tuple(x) for x in json.loads(d)]))
            if ix + 1 == len(edits) or edits[ix + 1][1] != "companion":
                _companions(g, pending)
                pending = []
            continue
        if kind == "insert-after":
            patterns = json.loads(a)
            inserts = [[tuple(x) for x in xs] for xs in json.loads(d)]
            if len(patterns) != len(inserts):
                raise ValueError(f"{where}: insert-after needs one insertion per pattern")
            skip = set() if b in ("", "-") else set(b.split(","))
            for name, (_, row) in list(g.st.items()):
                if name in skip:
                    continue
                for key, (target, seq) in list(row.items()):
                    acts = []
                    for x in g.seqs[seq]:
                        acts.append(x)
                        i = next((i for i, p in enumerate(patterns) if list(x[:len(p)]) == p), None)
                        if i is not None:
                            acts.extend(inserts[i])
                    row[key] = (target, g.seq(acts))
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
                           lines=["\t".join((a, b, c, d))],
                           line_numbers=[int(where.rsplit(":", 1)[1])])
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
    if os.environ.get("UNISACC_ROW_LOG"):
        _track_all_rows(g)
        g._active_row_origin = None
    return groups
