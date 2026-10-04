#!/usr/bin/env python3
"""Compare the C99 table-to-network constructor with the Python oracle."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(*args, timeout=20):
    result = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
    assert result.returncode == 0, (args, result.returncode, result.stderr[:1500])
    return result

with tempfile.TemporaryDirectory(prefix="unisacc-seed-net-") as d:
    work = Path(d)
    cnet = work / "seed-net"
    run("sh", str(ROOT / "unisacc.com"), "seed/net.c", "-o", str(cnet))   # 0.0.25 B1: our compiler, not cc
    graph = work / "prune.json"
    table = work / "prune.tbl"
    run("python3", "exec/build/gen.py", "prune", str(graph))
    run("python3", "exec/c/tbl.py", str(graph), str(table))
    cases = [table]
    stack = work / "stack.tbl"
    stack.write_text("T 3 2 0 0 0\nQ 0\nQ 1 29 4\n"
                     "R 1 3 -1 0 -1 -1 0 0 0 1 1 1 1\n"
                     "R 0 0 -1 0\nR 0 0 -1 0\n")
    cases.append(stack)
    for source in cases:
        py = work / (source.stem + ".py.net")
        c = work / (source.stem + ".c.net")
        run("python3", "exec/c/net.py", str(source), str(py))
        run(str(cnet), str(source), str(c))
        assert py.read_bytes() == c.read_bytes(), source
    # 0.0.25 B4: the manifest -> delta step in C, one stage at a time (seed/gen.c); prune first
    cgen = work / "seed-gen"
    run("sh", str(ROOT / "unisacc.com"), "seed/gen.c", "-o", str(cgen))
    for stage in ("prune",):
        c_json = work / (stage + ".c.json")
        run(str(cgen), stage, str(c_json), timeout=50)
        assert c_json.read_bytes() == graph.read_bytes(), "seed/gen.c %s delta differs from exec/build/gen.py" % stage
print("seed net: prune and declared-return table byte-identical to Python; seed/gen.c prune delta byte-identical")
