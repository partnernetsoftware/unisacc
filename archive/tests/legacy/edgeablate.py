#!/usr/bin/env python3
"""Offline produce for tests/edgeablate.cx --produce (not on gate hot path)."""
from __future__ import annotations

import hashlib
import os
import subprocess
import sys
from pathlib import Path

_LEGACY = Path(__file__).resolve().parent
if str(_LEGACY) not in sys.path:
    sys.path.insert(0, str(_LEGACY))
from edgeseal import esc, read_manifest


def _ask_keys(leaf: Path) -> list[str]:
    out: list[str] = []
    for line in (leaf / "asks.tsv").read_text().splitlines():
        if line and not line.startswith("#"):
            out.append(line.split("\t", 1)[0])
    return out


def _gold_keys(root: Path, net: str) -> list[str]:
    from unisa.gold import STAGES

    st = STAGES[net]
    keys: list[str] = []
    for line in (root / "weights/gold" / ("%s.tsv" % net)).read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < len(st.fields) + 1:
            continue
        key = tuple(parts[: len(st.fields)])
        keys.append(esc("\x1f".join(key)))
    return keys


def _compile(root: Path, probe: str, target: str, ablate: str | None, out: Path) -> int:
    env = os.environ.copy()
    if ablate:
        env["UNISA_ABLATE"] = ablate
    else:
        env.pop("UNISA_ABLATE", None)
    return subprocess.run(
        [sys.executable, "-m", "unisa", "compile", probe, "-o", str(out), "--target", target, "--drive", "built"],
        cwd=root,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    ).returncode


def _how(root: Path, m: dict[str, str]) -> str:
    import tempfile

    with tempfile.TemporaryDirectory() as td:
        base = Path(td) / "b.img"
        abl = Path(td) / "a.img"
        rc0 = _compile(root, m["probe"], m["target"], None, base)
        rc1 = _compile(root, m["probe"], m["target"], m["net"], abl)
        if rc0 != 0:
            raise SystemExit("edgeablate: baseline compile failed")
        if rc1 != 0:
            return "raise"
        if hashlib.sha256(base.read_bytes()).digest() != hashlib.sha256(abl.read_bytes()).digest():
            return "artifact"
    raise SystemExit("edgeablate: refuse to freeze (UNISA_ABLATE=%s had no effect)" % m["net"])


def produce(leaf: Path, root: Path) -> int:
    os.chdir(root)
    if str(root) not in sys.path:
        sys.path.insert(0, str(root))
    m = read_manifest(leaf)
    in_map = _ask_keys(leaf)
    if len(in_map) < 2:
        print("edgeablate: need at least two asks.tsv keys for in-map samples")
        return 1
    on_edge = set(in_map)
    gold = _gold_keys(root, m["net"])
    gold_set = set(gold)
    in_pref = {
        "pp": ("define\\x1f1", "include\\x1f1"),
        "parse": ("after_name\\x1f(", "unary\\x1f~"),
    }.get(m["net"], ())
    if len(in_pref) >= 2 and in_pref[0] in on_edge and in_pref[1] in on_edge:
        first, second = in_pref[0], in_pref[1]
    else:
        first = in_map[0]
        second = (
            next((k for k in in_map if k.startswith("include")), None)
            or next((k for k in in_map if k == "unary\\x1f~"), None)
            or in_map[1]
        )
    out_pref = {
        "pp": ("elif\\x1f1",),
        "parse": ("top\\x1fid",),
    }.get(m["net"], ())
    out_key = None
    for prefer in out_pref:
        if prefer not in on_edge and prefer in gold_set:
            out_key = prefer
            break
    if not out_key:
        for gk in gold:
            if gk not in on_edge:
                out_key = gk
                break
    if not out_key:
        print("edgeablate: no out-of-map gold key found")
        return 1
    how = _how(root, m)
    hdr = (
        "# sampled ablation verdicts %s->%s probe %s -- written by edgeablate.cx --produce; do not edit\n"
        "# key\tsample\tverdict\thow (in-map keys must be changed, out-of-map unchanged)\n"
        % (m["flow"], m["net"], m["probe"])
    )
    rows = [
        "%s\tin-map\tchanged\t%s\n" % (first, how),
        "%s\tin-map\tchanged\t%s\n" % (second, how),
        "%s\tout-of-map\tunchanged\tartifact\n" % out_key,
    ]
    (leaf / "ablate.tsv").write_text(hdr + "".join(rows))
    print("edgeablate: wrote %s (%d rows, in-map how=%s)" % (leaf / "ablate.tsv", len(rows), how))
    return 0


def main() -> int:
    if len(sys.argv) != 4 or sys.argv[1] != "--produce":
        print(__doc__)
        return 2
    return produce(Path(sys.argv[2]), Path(sys.argv[3]).resolve())


if __name__ == "__main__":
    sys.exit(main())
