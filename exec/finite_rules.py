"""Expand disjoint finite observation-transition rules; no language-specific decisions."""
import json


def load(path, sequences, domain=range(257), classes=None, bindings=None):
    domain = set(domain)
    explicit, defaults = {}, {}
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
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
    if not explicit:
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
