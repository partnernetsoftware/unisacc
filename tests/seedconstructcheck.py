#!/usr/bin/env python3
"""Compare the C99 table-to-network constructor with the Python oracle."""
from pathlib import Path
import json
import importlib
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "exec"))
import assemble
import finite_rules
from build import parsebase as native_base
from build import parse2base

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
    # B4 parse2 first slice: the declared token vocabulary and its two E3
    # additions must have the same insertion order and numbers in both seeds.
    token_json = work / "parse2-tokens.c.json"
    run(str(cgen), "inspect-parse2-tokens", str(token_json))
    e3 = parse2base.executor()
    expected_tokens = json.dumps({"WORDS": e3.WORDS, "TK": e3.TK}, separators=(",", ":")).encode()
    assert token_json.read_bytes() == expected_tokens, "seed/gen.c parse2 token prelude differs"
    token_graph = work / "parse2-token-graph.c.json"
    run(str(cgen), "inspect-parse2-token-graph", str(token_graph))
    e3.g.finish()
    expected_graph = {"start": "START",
                      "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                 for name, (mode, row) in e3.g.st.items()},
                      "seqs": [list(map(list, seq)) for seq in e3.g.seqs]}
    assert token_graph.read_bytes() == json.dumps(expected_graph, separators=(",", ":")).encode(), \
        "seed/gen.c parse2 tokenizer graph differs"
    actions_json = work / "parse2-actions.c.json"
    run(str(cgen), "inspect-mapseq", "parse2:gen2-actions", str(actions_json))
    actions_manifest = ROOT / "exec/parse2/gen2-actions-manifest.tsv"
    action_rows = [line.split("\t") for line in actions_manifest.read_text().splitlines()
                   if line and not line.startswith("#")]
    action_names = list(json.loads(action_rows[0][8])["mapseq"])
    action_env = assemble.Run(e3, e3.P, {}, {}).run(actions_manifest)
    expected_actions = {name: action_env[name] for name in action_names}
    assert actions_json.read_bytes() == json.dumps(expected_actions, separators=(",", ":")).encode(), \
        "seed/gen.c parse2 action recipes differ"
    for stage, flags in (("prune", ()), ("opt", ()), ("opt", ("--o2",)), ("lex", ()), ("lex", ("--typed",)), ("pp", ()), ("nativeabi", ())):
        tag = stage + "".join(flags)
        ref = graph if stage == "prune" else work / (tag + ".py.json")
        if stage != "prune": run("python3", "exec/build/gen.py", stage, str(ref), *flags, timeout=50)
        c_json = work / (tag + ".c.json")
        run(str(cgen), stage, str(c_json), *flags, timeout=50)
        assert c_json.read_bytes() == ref.read_bytes(), "seed/gen.c %s delta differs from exec/build/gen.py" % stage
    # Native ABI is the next complete stage. Check its already shared DSL parts
    # against the Python constructor before composing the recursive call graph.
    manifest = ROOT / "exec/nativeabi/gen-manifest.tsv"
    rows = [line.split("\t") for line in manifest.read_text().splitlines()
            if line and not line.startswith("#")]
    let = [row for row in rows if row[0] == "let" and "mapseq" in row[8]]
    assert len(let) == 1, "nativeabi mapseq declaration changed"
    runner = object.__new__(assemble.Run)
    runner.env = {}
    expected = runner.mapseq(json.loads(let[0][8])["mapseq"], assemble.load_facts(let[0][4]), {})
    c_map = work / "nativeabi-mapseq.c.json"
    run(str(cgen), "inspect-mapseq", "nativeabi", str(c_map))
    assert c_map.read_bytes() == json.dumps(expected, separators=(",", ":")).encode(), "nativeabi mapseq differs"
    rules, edits, modes = finite_rules.expand_template(ROOT / "exec/nativeabi/gen-template.tsv",
                                                       assemble.load_facts("nativeabi"),
                                                       lambda kind: (_ for _ in ()).throw(AssertionError(kind)),
                                                       section="reject")
    assert not edits and not modes
    c_reject = work / "nativeabi-reject.c.tsv"
    run(str(cgen), "inspect-template", "nativeabi", "reject", str(c_reject))
    assert c_reject.read_bytes() == ("\n".join(rules) + "\n").encode(), "nativeabi reject template differs"
    def native_graph_bytes():
        native_base.g.finish()
        data = {"start": "START",
                "states": {name: [mode, {str(key): value for key, value in edges.items()}]
                           for name, (mode, edges) in native_base.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in native_base.g.seqs]}
        return json.dumps(data, separators=(",", ":")).encode()
    native_base = importlib.reload(native_base)
    native_base.results = {}
    assemble.Run(native_base, native_base.P, {}, {"fail": "DEAD"}).run(ROOT / "exec/modelgraphequality-manifest.tsv")
    calls_expected = native_graph_bytes()
    calls_c = work / "nativeabi-calls.c.json"
    run(str(cgen), "inspect-nativeabi-calls", str(calls_c))
    assert calls_c.read_bytes() == calls_expected, "nativeabi recursive call graph differs"
    native_base = importlib.reload(native_base)
    native_base.results = {}
    runner = assemble.Run(native_base, native_base.P, {}, native_base.results)
    runner.root = ROOT / "exec/nativeabi"
    for depth, row in assemble.Run.rows(manifest)[:4]:
        runner.one(row, [], depth, {})
    head_expected = native_graph_bytes()
    head_c = work / "nativeabi-head.c.json"
    run(str(cgen), "inspect-nativeabi-head", str(head_c))
    assert head_c.read_bytes() == head_expected, "nativeabi head graph differs"
print("seed net: prune and declared-return table byte-identical to Python; seed/gen.c prune, opt O1, opt --o2, lex, lex --typed, pp and nativeabi deltas byte-identical; nativeabi call graph/head and parse2 token prelude/tokenizer graph/action recipes byte-identical")
