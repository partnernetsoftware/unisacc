"""Read committed domain-fact tables exec/facts/STEM.tsv (K2).

Format: first line `# col1<TAB>col2...`; each later line one fact row.  A cell
that parses as JSON (number, list, object, true/false/null, quoted string) is
decoded; any other cell is a plain string.  Returns a list of dicts, or, for a
table whose single column is named `value`, the list of values.
"""
import json
from pathlib import Path

HERE = Path(__file__).parent


def _cell(text):
    try:
        return json.loads(text)
    except ValueError:
        return text


def facts(stem):
    lines = (HERE / (stem + ".tsv")).read_text(encoding="utf-8").splitlines()
    head = lines[0].lstrip("#").strip().split("\t")
    rows = [dict(zip(head, map(_cell, l.split("\t")))) for l in lines[1:] if l and not l.startswith("#")]
    return [r["value"] for r in rows] if head == ["value"] else rows
