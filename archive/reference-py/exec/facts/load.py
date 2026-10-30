"""Read committed domain-fact tables exec/facts/STEM.tsv (K2).

Format: first line `# col1<TAB>col2...`; each later line one fact row.  A cell
that parses as JSON (number, list, object, true/false/null, quoted string) is
decoded; any other cell is a plain string.  Returns a list of dicts, or, for a
table whose single column is named `value`, the list of values.
"""
import json
import re
from pathlib import Path

HERE = Path(__file__).parent


def _cell(text):
    try:
        return json.loads(text)
    except ValueError:
        return text


def facts(stem, root=HERE):
    lines = (root / (stem + ".tsv")).read_text(encoding="utf-8").splitlines()
    head = lines[0].lstrip("#").strip().split("\t")
    rows = [dict(zip(head, map(_cell, l.split("\t")))) for l in lines[1:] if l and not l.startswith("#")]
    return [r["value"] for r in rows] if head == ["value"] else rows


def typed_cell(kind, text):
    if kind == "int": return int(text)
    if kind == "json": return json.loads(text)
    if kind == "str": return re.sub(r"\\(.)", lambda m: {"t": "\t", "n": "\n"}.get(m.group(1), m.group(1)), text)
    raise ValueError("fact type " + kind)


def manifest_facts(stem, root=HERE):
    """Read the scalar/table fact form or the older header-row fact form."""
    lines = (root / (stem + ".tsv")).read_text().splitlines()
    first = next((line for line in lines if line and not line.startswith("#")), "")
    if first and first[0] not in "=@\t":
        rows = facts(stem, root)
        result = {stem: rows}
        if rows and isinstance(rows[0], dict) and set(rows[0]) == {"name", "value"}:
            result[stem + "!"] = {r["name"]: r["value"] for r in rows}
            for row in rows: result.setdefault(row["name"], row["value"])
        return result
    result, cols, current = {}, None, None
    for line in lines:
        if not line or line.startswith("#"): continue
        fields = line.split("\t")
        if fields[0].startswith("="): result[fields[0][1:]] = typed_cell(fields[1], fields[2])
        elif fields[0].startswith("@"):
            current, cols = fields[0][1:], [col.split(":") for col in fields[1:]]
            result[current] = []
        else:
            assert fields[0] == "" and cols is not None and len(fields) - 1 == len(cols), line
            result[current].append({col: typed_cell(kind, value) for (col, kind), value in zip(cols, fields[1:])})
    return result
