#!/usr/bin/env python3
"""R9 #if syntax and internal-error regression, independent of compiler builds."""
import sys
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from unisa.front import pp


def main():
    failures = []
    valid = {"0": False, "1": True, "(1 + 2) == 3": True,
             "defined(X) && X == 2": True, "UNKNOWN": False,
             "0 && 1 / 0": False, "1 || 1 / 0": True,
             "1 ? 2 : 1 / 0": True, "0 ? 1 / 0 : 3": True}
    for expr, want in valid.items():
        if pp._truth(expr, {"X": "2"}) != want:
            failures.append("valid: " + expr)
    bad = ["", "1 +", "(1", "1)", "1 2", "1 @ 2", "1 ? 2", "1 ? : 2",
           "defined()", "defined(1)", "defined(X", "defined", "0 && (", "08",
           "0x", "'a", "''", "'\\x'", "1uu"]
    for expr in bad:
        try:
            pp._truth(expr, {}, live=False)
        except ValueError:
            pass
        else:
            failures.append("accepted syntax: " + repr(expr))
    for live in (True, False):
        try:
            with patch.object(pp._PPExpr, "cond", side_effect=RuntimeError("sentinel")):
                pp._truth("1", {}, live)
        except RuntimeError as exc:
            assert str(exc) == "sentinel"
        else:
            failures.append("swallowed internal exception")
    try:
        pp._truth("1 / 0", {})
    except ValueError:
        pass
    else:
        failures.append("accepted live division by zero")
    assert pp._truth("1 / 0", {}, live=False) is False
    if failures:
        print("\n".join(failures))
        return 1
    print("pptruth: valid 9, malformed 19, internal 2, division 2 passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
