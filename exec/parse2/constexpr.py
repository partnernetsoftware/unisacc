"""Constant-expression evaluation as delta procedures (no emitted tape).
Binding strengths come from the same prec rows as ordinary expressions.
The evaluator uses generic integer actions; it adds no executor primitive.
This slice evaluates signed 64-bit integers, not all C constant-expression
conversions. Division by zero is refused even in an unselected arm; sizeof
and casts remain unsupported. These are coverage limits, not C language rules.
"""

import csv
from pathlib import Path
from finite_rules import install as install_rules, load as load_rules


def install(E, P, levels, ops, enum_values, enum_defined):
    root = Path(__file__).parent
    def rows(name):
        with (root / ("constexpr-" + name + ".tsv")).open() as source:
            return list(csv.reader(source, delimiter="\t"))[1:]
    sequences = {name: E.rej(message) for name, message in rows("reject")}
    p = P("constexpr.bindings")
    for name, method, slots in rows("stack"):
        p.acts = []
        sequences[name] = getattr(p, method)(*slots.split(",")).acts
    tokens = dict(E.TK, identifier=E.TK_ID, number=E.TK_NUM, string=E.TK_STR)
    classes = {name: [tokens[token]] for name, token in rows("tokens")}
    # a cast or sizeof(TYPE) inside a constant expression starts with a type word or a tag keyword
    classes["typeword"] = sorted({tokens[w] for w in tokens if w.startswith("type")} | {tokens["struct"], tokens["union"], tokens["enum"]})
    fresh = rows("fresh")
    def emit(section, bindings):
        for part, prefix, kind, key in fresh:
            if part == section:
                p.cur = bindings.get(prefix, prefix)
                bindings[key] = p.fresh(kind)
        install_rules(E.g, root, "constexpr", bindings=bindings, sequences=sequences, classes=classes, section=section)
    emit("entry", dict(first="CE" + str(levels[0])))
    operators = {op: fields for op, *fields in rows("operators")}
    for i, level in enumerate(levels):
        name = "CE" + str(level)
        bindings = dict(owner=name, name=name, loop=name + ".loop",
                        sub="CE" + str(levels[i + 1]) if i + 1 < len(levels) else "CE.atom")
        emit("level", bindings)
        targets = {E.TK[op]: name + "." + op for op in ops[level]}
        for key, target in targets.items(): E.g.on(bindings["dispatch"], [key], target, [], "r")
        for state, row in load_rules(root / "constexpr-default.tsv", {},
                domain=set(range(257)) - targets.keys(), bindings=bindings).items():
            for key, (target, actions) in row.items(): E.g.on(state, [key], target, actions, "r")
        for op in ops[level]:
            category, action, comparison, zero, nonzero = operators[op]
            operator = targets[E.TK[op]]
            bindings.update(operator=operator, action=action, zero=operator + "." + zero, nonzero=operator + "." + nonzero)
            bindings.update((suffix, operator + "." + suffix) for suffix in ("true", "false", "rhs", "hit", "no"))
            classes["comparison"] = list(map(int, comparison.split(","))) if comparison != "-" else []
            emit("prefix", bindings)
            emit(category, bindings)
    emit("atom", dict(enum_values=enum_values, enum_defined=enum_defined, SKIPS=7 * (1 << 26)))   # SKIPS: gen2 POSSPAN region 7 -- a string sizeof() measured is not pooled
