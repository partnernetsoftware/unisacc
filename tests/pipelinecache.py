#!/usr/bin/env python3
"""Exercise production fresh with K2 stage-data mutations in a private tree.

A tiny graph emitter isolates caching from compiler generation. It reads a real
stage table snapshot; no checkout inputs or shared cache are mutated.
"""
import importlib.util
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "exec/pipeline"))
spec = importlib.util.spec_from_file_location("pipeline_run", ROOT / "exec/pipeline/run.py")
run = importlib.util.module_from_spec(spec)
spec.loader.exec_module(run)

with tempfile.TemporaryDirectory(prefix="t7-cache-") as directory:
    root = Path(directory)
    directories = ("exec/build", "exec/parse2", "exec/parse", "exec/lex", "exec/pp",
                   "unisa", "src", "kernel", "include", "weights/gold",
                   "iterate/kernel", "cache/pipe")
    for name in directories:
        (root / name).mkdir(parents=True, exist_ok=True)
    (root / "exec/stamp.sh").write_bytes((ROOT / "exec/stamp.sh").read_bytes())
    (root / "iterate/kernel/typekw.tsv").write_text("type\n")
    (root / "cache/ua_ref.stamp").write_text("reference\n")
    for name in ("pp", "lex", "lexcls", "lexword", "parse"):
        (root / "weights/gold" / (name + ".tsv")).write_text(name + "\n")
    table = root / "exec/parse2/return-result.tsv"
    table.write_bytes((ROOT / "exec/parse2/return-result.tsv").read_bytes())
    (root / "exec/build/gen.py").write_text(
        "import json,sys\nfrom pathlib import Path\n"
        "p=Path('exec/parse2/return-result.tsv')\n"
        "Path(sys.argv[2]).write_text(json.dumps({'rows':p.read_text()}))\n")
    run.ROOT = str(root)
    run.X = str(root / "cache")
    run.models.ROOT = root
    command = ["python3", "exec/build/gen.py", "parse2", "cache/pipe/graph.json"]
    output = root / "cache/pipe/graph.json"

    def fresh():
        rc, stderr = run.fresh(str(output), command, run.gen_inputs(" ".join(command)))
        assert rc == 0, stderr
        return output.read_bytes(), b"(re)building" in stderr

    original = table.read_text()
    initial, rebuilt = fresh()
    assert rebuilt
    changed = original.replace('["LDI","stl",0]', '["LDI","stl",1]', 1)
    assert changed != original
    table.write_text(changed)
    graph, rebuilt = fresh()
    assert rebuilt and graph != initial
    same, rebuilt = fresh()
    assert not rebuilt and same == graph
    table.write_text(original)
    restored, rebuilt = fresh()
    assert rebuilt and restored == initial
    print("pipeline cache: changed/restored table invalidate; unchanged input hits")

    for relative in ("exec/parse/base-result.tsv", "exec/lex/output.tsv"):
        path = root / relative
        path.write_text("new fact\n")
        assert fresh()[1], relative
        assert not fresh()[1], relative
        path.unlink()
        assert fresh()[1], relative
    print("pipeline cache: parse/lex additions and removals invalidate")

    (root / "exec/build/disposable.json").write_text("generated\n")
    assert not fresh()[1]
    (root / "cache/ua_ref.stamp").write_text("different reference\n")
    assert fresh()[1]
    assert not fresh()[1]
    print("pipeline cache: generated outputs ignored; reference identity retained")
