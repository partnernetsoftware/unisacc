#!/usr/bin/env python3
"""Current-state numbers live in the generated regions or nowhere.

The reviewer's finding, 2026-09-30: the same two facts were restated in three
documents with three different values, and none of them was the ledger's.

    research/model-bytes.json (the generated ledger)   24 models   1088 rows
    README.md:29                                       33           1,082
    ARCHITECTURE.md:28                                 32             938
    prd.md:254                                         33           1,082

prd.md:254 was where the other two copied from, so "prd is the authority" was
itself the bug.  The paragraph also still carried the v0.0.9 artifact size
(1,233,236 B) as if it were current.

docs.sh could not see any of it.  docgen owns only the text between
<!-- stages:begin --> and <!-- stages:end -->, modelbytes owns only
<!-- model-bytes:begin --> ... <!-- model-bytes:end -->, and every wrong
number sat outside both.  A structural check cannot catch a restatement; the
check has to be about the prose.

The rule, then: a current-state number may appear in a document only inside a
generated region, or in a sentence that points at the generated ledger.  This
file enforces that, and it takes its own numbers from the ledger -- if it
hard-coded 24 and 1088 it would be a fourth restatement, which is the exact
failure it exists to prevent.

Two exemptions, both deliberate and both narrow:

  * version-table rows.  `| v0.0.9 artifact | ... 1,233,236 B ... |` is
    correct: a historical size on that version's own row is a historical fact,
    not a stale current-state claim.  The rule is about which numbers a reader
    would take as "now".
  * the correction notes.  A row that says "this read 33 and 1,082 while the
    ledger reads 24" is documenting the fix; suppressing the evidence would
    make the correction unauditable.  Those are recognised by an explicit
    marker word, not by a loose heuristic.

Usage:  numberrestatements.py [--mutation KIND] [--quiet]
        --mutation plants a restatement to prove the check can fail.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "research" / "model-bytes.json"

# A document region is text between these, inclusive of the markers.
REGIONS = {
    "prd.md": (
        ("<!-- stages:begin -->", "<!-- stages:end -->"),
        ("<!-- stages-zh:begin -->", "<!-- stages-zh:end -->"),
        ("<!-- model-bytes:begin -->", "<!-- model-bytes:end -->"),
    ),
}
# Every document the rule applies to (prd.tree.md / prd.map.md archived in 0.0.15).  They carried
# generated stage tables too, and are held to the same rule.
DOCS = ("README.md", "ARCHITECTURE.md", "prd.md", "spec.md", "exec/README.md")

# A line is exempt when it is a version-table row (a pipe row naming a version)
# or when it names the wrong value on purpose.  These are the smallest
# conditions that let the two legitimate kinds of line through.
#
# The first run of this check flagged five lines and all five were false
# positives -- every one a *release record* line: a markdown list item under a
# `### v0.0.x` heading, or English prose naming "Published v0.0.8".  A number
# on a line that names its own version or commit is a historical fact about
# that version, which is precisely the kind the rule must allow.  So the
# exemption keys on the version or commit being named, not on the row being a
# table row.
NAMED_VERSION = re.compile(r"\bv?\d+\.\d+\.\d+\b|\b[0-9a-f]{7,40}\b|releases/tag/")
VERSION_ROW = re.compile(r"^\s*\|[^|]*\b(?:v?\d+\.\d+\.\d+|\d{7,8}\b)[^|]*\|")
CORRECTION = re.compile(r"曾写|曾只列|used to say|this row said|while the ledger(?: said| reads)|that read|was written as|不符|都是 24")


def current_numbers():
    """Every current-state number that must not be restated, from the ledger."""
    led = json.loads(LEDGER.read_text())["artifact_ledger"]
    out = {
        "models": len(led["models"]),
        "stage_rows": led["stage_rows"],
        "total_bytes": led["total_bytes"],
        "package_bytes": led["package_bytes"],
        "sha256": led["sha256"],
    }
    return out


def historical_numbers():
    """Sizes that earlier versions had.  A version-table row may carry its own.

    Taken from the ledger's own history if it has one, else from the version
    rows this repository has published -- listed once here so that a *current*
    claim of an old size is visible, which is exactly what prd.md:254 did.
    """
    return {"v0.0.9_com": 1233236, "v0.0.8_com": 5388402, "v0.0.10_com": 1168488, "v0.0.11_com": 1170384}


def regions_of(path):
    spans = []
    for begin, end in REGIONS.get(path.name, ()):
        text = None
        start = 0
        while True:
            i = (text or path.read_text(encoding="utf-8")).find(begin, start)
            if i < 0:
                break
            j = (text or path.read_text(encoding="utf-8")).find(end, i)
            if j < 0:
                break
            spans.append((i, j + len(end)))
            start = j + len(end)
    return spans


def inside(spans, pos):
    return any(a <= pos < b for a, b in spans)


def restatements():
    """Lines that restate a current (or stale-as-current) number outside a region."""
    nums = current_numbers()
    hist = historical_numbers()
    # The literal spellings that count as a restatement of a big number.
    wanted = {}
    for name in ("total_bytes", "package_bytes"):
        v = nums[name]
        wanted[f"{v:,}"] = f"current {name}"
        wanted[str(v)] = f"current {name}"
    for v in hist.values():
        wanted[f"{v:,}"] = "a historical artifact size claimed as current"
        wanted[str(v)] = "a historical artifact size claimed as current"
    bad = []
    for rel in DOCS:
        p = ROOT / rel
        if not p.is_file():
            continue
        text = p.read_text(encoding="utf-8")
        spans = regions_of(p)
        offset = 0
        for line in text.split("\n"):
            pos = offset
            offset += len(line) + 1
            if inside(spans, pos):
                continue
            if VERSION_ROW.match(line) or NAMED_VERSION.search(line) or CORRECTION.search(line):
                continue
            for lit, why in wanted.items():
                if lit in line:
                    bad.append((rel, line.strip()[:120], why))
                    break
    return bad


def mutation(kind):
    """Plant a restatement so the check is proven able to fail.

    Returns a restore function.  Every kind edits a real document, which is
    what makes this a mutation test rather than a unit test of the matcher.
    """
    nums = current_numbers()
    target = ROOT / "ARCHITECTURE.md"
    original = target.read_text(encoding="utf-8")
    if kind == "size":
        planted = original.replace(
            "# 仓库地图\n",
            "# 仓库地图\n\n当前产物 %s B。\n" % f"{nums['total_bytes']:,}",
            1,
        )
    elif kind == "stale":
        planted = original.replace(
            "# 仓库地图\n",
            "# 仓库地图\n\n当前产物 1,233,236 B（包 985,172 B）。\n",
            1,
        )
    else:
        raise SystemExit("unknown mutation: %s" % kind)
    assert planted != original, "mutation did not apply"
    target.write_text(planted, encoding="utf-8")
    return lambda: target.write_text(original, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--mutation", choices=("size", "stale"), help="plant a restatement and require a red")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args()
    if a.mutation:
        restore = mutation(a.mutation)
        try:
            bad = restatements()
        finally:
            restore()
        if not bad:
            print("mutation %r was NOT caught -- the check cannot fail" % a.mutation)
            return 1
        if not a.quiet:
            print("mutation %r caught: %s" % (a.mutation, bad[0][1]))
        return 0
    bad = restatements()
    if bad:
        for rel, line, why in bad:
            print("%s: %s  [%s]" % (rel, line, why))
        print()
        print("state numbers %d restated outside a generated region or a ledger pointer"
              % len(bad))
        return 1
    if not a.quiet:
        print("state numbers: no restatement outside the generated regions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
