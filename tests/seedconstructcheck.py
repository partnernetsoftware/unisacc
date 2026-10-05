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

if len(sys.argv) == 3 and sys.argv[1] == "--part" and sys.argv[2] in ("base", "parse2", "parse2-2", "parse2-3", "parse2-4", "parse2-5", "parse2-6", "parse2-7", "parse2-8", "parse2-9", "parse2-10", "parse2-11", "parse2-12"):
    part = sys.argv[2]
else:
    raise SystemExit("usage: seedconstructcheck.py --part base|parse2|parse2-2|parse2-3|parse2-4|parse2-5|parse2-6|parse2-7|parse2-8|parse2-9|parse2-10|parse2-11|parse2-12")

with tempfile.TemporaryDirectory(prefix="unisacc-seed-net-") as d:
    work = Path(d)
    if part in ("base", "all"):
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
    if part in ("parse2", "all"):
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
        # First nested E3 body call: gen2parts startup edits copy the token reader,
        # remove its old marker edge, then install the declared marker rule.
        startup_graph = work / "parse2-startup-graph.c.json"
        run(str(cgen), "inspect-parse2-startup-graph", str(startup_graph))
        e3_startup = parse2base.executor()
        assemble.Run(e3_startup, e3_startup.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        e3_startup.g.finish()
        expected_startup = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_startup.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_startup.g.seqs]}
        assert startup_graph.read_bytes() == json.dumps(expected_startup, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 startup body call differs"
        control_graph = work / "parse2-startup-control.c.json"
        run(str(cgen), "inspect-parse2-startup-control-graph", str(control_graph))
        e3_control = parse2base.executor()
        assemble.Run(e3_control, e3_control.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_control, e3_control.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        e3_control.g.finish()
        expected_control = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_control.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_control.g.seqs]}
        assert control_graph.read_bytes() == json.dumps(expected_control, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 startup control body call differs"
        string_graph = work / "parse2-startup-strings.c.json"
        run(str(cgen), "inspect-parse2-startup-strings-graph", str(string_graph))
        e3_strings = parse2base.executor()
        assemble.Run(e3_strings, e3_strings.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_strings, e3_strings.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        e3_strings.g.finish()
        expected_strings = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_strings.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_strings.g.seqs]}
        assert string_graph.read_bytes() == json.dumps(expected_strings, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 string span body call differs"
        prefix_graph = work / "parse2-startup-prefix.c.json"
        run(str(cgen), "inspect-parse2-startup-prefix-graph", str(prefix_graph))
        e3_prefix = parse2base.executor()
        assemble.Run(e3_prefix, e3_prefix.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_prefix, e3_prefix.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_prefix, e3_prefix.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        e3_prefix.g.finish()
        expected_prefix = {"start": "START",
                           "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                      for name, (mode, row) in e3_prefix.g.st.items()},
                           "seqs": [list(map(list, seq)) for seq in e3_prefix.g.seqs]}
        assert prefix_graph.read_bytes() == json.dumps(expected_prefix, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 startup call order differs"
        head_graph = work / "parse2-initializer-head.c.json"
        run(str(cgen), "inspect-parse2-initializer-head-graph", str(head_graph))
        e3_head = parse2base.executor()
        assemble.Run(e3_head, e3_head.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_head, e3_head.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_head, e3_head.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        initializer_manifest = ROOT / "exec/parse2/strings-initializer-manifest.tsv"
        initializer = assemble.Run(e3_head, e3_head.P, {}, {})
        initializer.root = initializer_manifest.parent
        for depth, row in initializer.rows(initializer_manifest)[:2]:
            assert depth == 0
            initializer.one(row, [], depth, {})
        e3_head.g.finish()
        expected_head = {"start": "START",
                         "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                    for name, (mode, row) in e3_head.g.st.items()},
                         "seqs": [list(map(list, seq)) for seq in e3_head.g.seqs]}
        assert head_graph.read_bytes() == json.dumps(expected_head, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 string initializer head differs"
        walk_graph = work / "parse2-strwalk-head.c.json"
        run(str(cgen), "inspect-parse2-strwalk-head-graph", str(walk_graph))
        e3_walk = parse2base.executor()
        assemble.Run(e3_walk, e3_walk.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_walk, e3_walk.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_walk, e3_walk.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        initializer = assemble.Run(e3_walk, e3_walk.P, {}, {})
        initializer.root = initializer_manifest.parent
        for depth, row in initializer.rows(initializer_manifest)[:2]:
            initializer.one(row, [], depth, {})
        walker_manifest = ROOT / "exec/parse2/strwalk-manifest.tsv"
        walker = assemble.Run(e3_walk, e3_walk.P, {},
                              {"pre": "SI.walk", "body": "SI.byte", "done": "SI.end"})
        walker.root = walker_manifest.parent
        for depth, row in walker.rows(walker_manifest)[:2]:
            walker.one(row, [], depth, {})
        e3_walk.g.finish()
        expected_walk = {"start": "START",
                         "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                    for name, (mode, row) in e3_walk.g.st.items()},
                         "seqs": [list(map(list, seq)) for seq in e3_walk.g.seqs]}
        assert walk_graph.read_bytes() == json.dumps(expected_walk, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 string walker head differs"
        escape_graph = work / "parse2-strwalk-escape.c.json"
        run(str(cgen), "inspect-parse2-strwalk-escape-graph", str(escape_graph))
        e3_escape = parse2base.executor()
        assemble.Run(e3_escape, e3_escape.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_escape, e3_escape.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_escape, e3_escape.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        initializer = assemble.Run(e3_escape, e3_escape.P, {}, {})
        initializer.root = initializer_manifest.parent
        for depth, row in initializer.rows(initializer_manifest)[:2]:
            initializer.one(row, [], depth, {})
        walker = assemble.Run(e3_escape, e3_escape.P, {},
                              {"pre": "SI.walk", "body": "SI.byte", "done": "SI.end"})
        walker.root = walker_manifest.parent
        for depth, row in walker.rows(walker_manifest)[:3]:
            walker.one(row, [], depth, {})
        e3_escape.g.finish()
        expected_escape = {"start": "START",
                           "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                      for name, (mode, row) in e3_escape.g.st.items()},
                           "seqs": [list(map(list, seq)) for seq in e3_escape.g.seqs]}
        assert escape_graph.read_bytes() == json.dumps(expected_escape, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 string walker escape template differs"
        tail_graph = work / "parse2-strwalk.c.json"
        run(str(cgen), "inspect-parse2-strwalk-graph", str(tail_graph))
        e3_tail = parse2base.executor()
        assemble.Run(e3_tail, e3_tail.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_tail, e3_tail.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_tail, e3_tail.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        initializer = assemble.Run(e3_tail, e3_tail.P, {}, {})
        initializer.root = initializer_manifest.parent
        for depth, row in initializer.rows(initializer_manifest)[:2]:
            initializer.one(row, [], depth, {})
        walker = assemble.Run(e3_tail, e3_tail.P, {},
                              {"pre": "SI.walk", "body": "SI.byte", "done": "SI.end"})
        walker.root = walker_manifest.parent
        for depth, row in walker.rows(walker_manifest):
            walker.one(row, [], depth, {})
        e3_tail.g.finish()
        expected_tail = {"start": "START",
                         "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                    for name, (mode, row) in e3_tail.g.st.items()},
                         "seqs": [list(map(list, seq)) for seq in e3_tail.g.seqs]}
        assert tail_graph.read_bytes() == json.dumps(expected_tail, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 string walker tail differs"
        branch_graph = work / "parse2-startup-branch.c.json"
        run(str(cgen), "inspect-parse2-startup-branch-graph", str(branch_graph))
        e3_branch = parse2base.executor()
        assemble.Run(e3_branch, e3_branch.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_branch, e3_branch.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_branch, e3_branch.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_branch, e3_branch.P, {}, {}).run(initializer_manifest)
        e3_branch.g.finish()
        expected_branch = {"start": "START",
                           "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                      for name, (mode, row) in e3_branch.g.st.items()},
                           "seqs": [list(map(list, seq)) for seq in e3_branch.g.seqs]}
        assert branch_graph.read_bytes() == json.dumps(expected_branch, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 full startup branch differs"
        numeric_graph = work / "parse2-numeric.c.json"
        run(str(cgen), "inspect-parse2-numeric-graph", str(numeric_graph))
        e3_numeric = parse2base.executor()
        assemble.Run(e3_numeric, e3_numeric.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_numeric, e3_numeric.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_numeric, e3_numeric.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_numeric, e3_numeric.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_numeric, e3_numeric.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        e3_numeric.g.finish()
        expected_numeric = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_numeric.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_numeric.g.seqs]}
        assert numeric_graph.read_bytes() == json.dumps(expected_numeric, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 numeric child differs"
        float_boundary_graph = work / "parse2-float-boundary.c.json"
        run(str(cgen), "inspect-parse2-float-boundary-graph", str(float_boundary_graph))
        e3_float = parse2base.executor()
        assemble.Run(e3_float, e3_float.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_float, e3_float.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_float, e3_float.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_float, e3_float.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_float, e3_float.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        float_manifest = ROOT / "exec/parse2/floatconst-manifest.tsv"
        float_run = assemble.Run(e3_float, e3_float.P, {}, {})
        float_run.root = float_manifest.parent
        for depth, row in float_run.rows(float_manifest)[:2]:
            float_run.one(row, [], depth, {})
        e3_float.g.finish()
        expected_float = {"start": "START",
                          "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                     for name, (mode, row) in e3_float.g.st.items()},
                          "seqs": [list(map(list, seq)) for seq in e3_float.g.seqs]}
        assert float_boundary_graph.read_bytes() == json.dumps(expected_float, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 float boundary differs"
        float_entry_graph = work / "parse2-float-entry.c.json"
        run(str(cgen), "inspect-parse2-float-entry-graph", str(float_entry_graph))
        e3_entry = parse2base.executor()
        assemble.Run(e3_entry, e3_entry.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_entry, e3_entry.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_entry, e3_entry.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_entry, e3_entry.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_entry, e3_entry.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        entry_run = assemble.Run(e3_entry, e3_entry.P, {}, {})
        entry_run.root = float_manifest.parent
        for depth, row in entry_run.rows(float_manifest)[:3]:
            entry_run.one(row, [], depth, {})
        e3_entry.g.finish()
        expected_entry = {"start": "START",
                          "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                     for name, (mode, row) in e3_entry.g.st.items()},
                          "seqs": [list(map(list, seq)) for seq in e3_entry.g.seqs]}
        assert float_entry_graph.read_bytes() == json.dumps(expected_entry, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 float entry differs"
        float_digits_graph = work / "parse2-float-digits.c.json"
        run(str(cgen), "inspect-parse2-float-digits-graph", str(float_digits_graph))
        e3_digits = parse2base.executor()
        assemble.Run(e3_digits, e3_digits.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_digits, e3_digits.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_digits, e3_digits.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_digits, e3_digits.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_digits, e3_digits.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        digits_run = assemble.Run(e3_digits, e3_digits.P, {}, {})
        digits_run.root = float_manifest.parent
        for depth, row in digits_run.rows(float_manifest)[:6]:
            digits_run.one(row, [], depth, {})
        e3_digits.g.finish()
        expected_digits = {"start": "START",
                           "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                      for name, (mode, row) in e3_digits.g.st.items()},
                           "seqs": [list(map(list, seq)) for seq in e3_digits.g.seqs]}
        assert float_digits_graph.read_bytes() == json.dumps(expected_digits, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 float digits differ"
        float_rows_graph = work / "parse2-float-rows.c.json"
        run(str(cgen), "inspect-parse2-float-rows-graph", str(float_rows_graph))
        e3_float_rows = parse2base.executor()
        assemble.Run(e3_float_rows, e3_float_rows.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_float_rows, e3_float_rows.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_float_rows, e3_float_rows.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_float_rows, e3_float_rows.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_float_rows, e3_float_rows.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_float_rows, e3_float_rows.P, {}, {}).run(float_manifest)
        e3_float_rows.g.finish()
        expected_float_rows = {"start": "START",
                               "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                          for name, (mode, row) in e3_float_rows.g.st.items()},
                               "seqs": [list(map(list, seq)) for seq in e3_float_rows.g.seqs]}
        assert float_rows_graph.read_bytes() == json.dumps(expected_float_rows, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 float rows differ"
        autoscan_graph = work / "parse2-autoscan.c.json"
        run(str(cgen), "inspect-parse2-autoscan-graph", str(autoscan_graph))
        e3_autoscan = parse2base.executor()
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_autoscan, e3_autoscan.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {}).run(float_manifest)
        assemble.Run(e3_autoscan, e3_autoscan.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        e3_autoscan.g.finish()
        expected_autoscan = {"start": "START",
                             "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                        for name, (mode, row) in e3_autoscan.g.st.items()},
                             "seqs": [list(map(list, seq)) for seq in e3_autoscan.g.seqs]}
        assert autoscan_graph.read_bytes() == json.dumps(expected_autoscan, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 autoscan child differs"
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
    if part in ("parse2-2", "all"):
        initializer_manifest = ROOT / "exec/parse2/strings-initializer-manifest.tsv"
        float_manifest = ROOT / "exec/parse2/floatconst-manifest.tsv"
        unary_head_graph = work / "parse2-unary-head.c.json"
        run(str(cgen), "inspect-parse2-unary-head-graph", str(unary_head_graph))
        e3_unary = parse2base.executor()
        assemble.Run(e3_unary, e3_unary.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_unary, e3_unary.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_unary, e3_unary.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_unary, e3_unary.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_unary, e3_unary.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_unary, e3_unary.P, {}, {}).run(float_manifest)
        assemble.Run(e3_unary, e3_unary.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        unary_manifest = ROOT / "exec/parse2/unarycontrol-manifest.tsv"
        unaryenv = assemble.load_facts("k2-gen2")["unaryenv"]
        unary_run = assemble.Run(e3_unary, e3_unary.P, {},
                                 {"warnings": False, "ucx": unaryenv["ucx"],
                                  "ufacts": unaryenv["ufacts"]})
        unary_run.root = unary_manifest.parent
        for depth, row in unary_run.rows(unary_manifest)[:2]:
            unary_run.one(row, [], depth, {})
        e3_unary.g.finish()
        expected_unary = {"start": "START",
                          "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                     for name, (mode, row) in e3_unary.g.st.items()},
                          "seqs": [list(map(list, seq)) for seq in e3_unary.g.seqs]}
        assert unary_head_graph.read_bytes() == json.dumps(expected_unary, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary head differs"
        unary_compound_graph = work / "parse2-unary-compound.c.json"
        run(str(cgen), "inspect-parse2-unary-compound-graph", str(unary_compound_graph))
        e3_compound = parse2base.executor()
        assemble.Run(e3_compound, e3_compound.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_compound, e3_compound.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_compound, e3_compound.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_compound, e3_compound.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_compound, e3_compound.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_compound, e3_compound.P, {}, {}).run(float_manifest)
        assemble.Run(e3_compound, e3_compound.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        compound_run = assemble.Run(e3_compound, e3_compound.P, {},
                                    {"warnings": False, "ucx": unaryenv["ucx"],
                                     "ufacts": unaryenv["ufacts"]})
        compound_run.root = unary_manifest.parent
        for depth, row in compound_run.rows(unary_manifest)[:3]:
            compound_run.one(row, [], depth, {})
        e3_compound.g.finish()
        expected_compound = {"start": "START",
                             "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                        for name, (mode, row) in e3_compound.g.st.items()},
                             "seqs": [list(map(list, seq)) for seq in e3_compound.g.seqs]}
        assert unary_compound_graph.read_bytes() == json.dumps(expected_compound, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary compound differs"
        cast_void_graph = work / "parse2-unary-cast-void.c.json"
        run(str(cgen), "inspect-parse2-unary-cast-void-graph", str(cast_void_graph))
        e3_cast = parse2base.executor()
        assemble.Run(e3_cast, e3_cast.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_cast, e3_cast.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_cast, e3_cast.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_cast, e3_cast.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_cast, e3_cast.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_cast, e3_cast.P, {}, {}).run(float_manifest)
        assemble.Run(e3_cast, e3_cast.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        cast_run = assemble.Run(e3_cast, e3_cast.P, {},
                                {"warnings": False, "ucx": unaryenv["ucx"],
                                 "ufacts": unaryenv["ufacts"]})
        cast_run.root = unary_manifest.parent
        for depth, row in cast_run.rows(unary_manifest)[:4]:
            cast_run.one(row, [], depth, {})
        e3_cast.g.finish()
        expected_cast = {"start": "START",
                         "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                    for name, (mode, row) in e3_cast.g.st.items()},
                         "seqs": [list(map(list, seq)) for seq in e3_cast.g.seqs]}
        assert cast_void_graph.read_bytes() == json.dumps(expected_cast, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary cast-void differs"
        part4_graph = work / "parse2-unary-part4.c.json"
        run(str(cgen), "inspect-parse2-unary-part4-graph", str(part4_graph))
        e3_part4 = parse2base.executor()
        assemble.Run(e3_part4, e3_part4.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_part4, e3_part4.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_part4, e3_part4.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_part4, e3_part4.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_part4, e3_part4.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_part4, e3_part4.P, {}, {}).run(float_manifest)
        assemble.Run(e3_part4, e3_part4.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        part4_run = assemble.Run(e3_part4, e3_part4.P, {},
                                 {"warnings": False, "ucx": unaryenv["ucx"],
                                  "ufacts": unaryenv["ufacts"]})
        part4_run.root = unary_manifest.parent
        for depth, row in part4_run.rows(unary_manifest)[:5]:
            part4_run.one(row, [], depth, {})
        e3_part4.g.finish()
        expected_part4 = {"start": "START",
                          "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                     for name, (mode, row) in e3_part4.g.st.items()},
                          "seqs": [list(map(list, seq)) for seq in e3_part4.g.seqs]}
        assert part4_graph.read_bytes() == json.dumps(expected_part4, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary part4 differs"
        float_d_graph = work / "parse2-unary-float-d.c.json"
        run(str(cgen), "inspect-parse2-unary-float-d-graph", str(float_d_graph))
        e3_float_d = parse2base.executor()
        assemble.Run(e3_float_d, e3_float_d.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_float_d, e3_float_d.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_float_d, e3_float_d.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_float_d, e3_float_d.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_float_d, e3_float_d.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_float_d, e3_float_d.P, {}, {}).run(float_manifest)
        assemble.Run(e3_float_d, e3_float_d.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        float_d_run = assemble.Run(e3_float_d, e3_float_d.P, {},
                                   {"warnings": False, "ucx": unaryenv["ucx"],
                                    "ufacts": unaryenv["ufacts"]})
        float_d_run.root = unary_manifest.parent
        for depth, row in float_d_run.rows(unary_manifest)[:6]:
            float_d_run.one(row, [], depth, {})
        e3_float_d.g.finish()
        expected_float_d = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_float_d.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_float_d.g.seqs]}
        assert float_d_graph.read_bytes() == json.dumps(expected_float_d, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary float-d differs"
        float_s_graph = work / "parse2-unary-float-s.c.json"
        run(str(cgen), "inspect-parse2-unary-float-s-graph", str(float_s_graph))
        e3_float_s = parse2base.executor()
        assemble.Run(e3_float_s, e3_float_s.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_float_s, e3_float_s.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_float_s, e3_float_s.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_float_s, e3_float_s.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_float_s, e3_float_s.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_float_s, e3_float_s.P, {}, {}).run(float_manifest)
        assemble.Run(e3_float_s, e3_float_s.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        float_s_run = assemble.Run(e3_float_s, e3_float_s.P, {},
                                   {"warnings": False, "ucx": unaryenv["ucx"],
                                    "ufacts": unaryenv["ufacts"]})
        float_s_run.root = unary_manifest.parent
        for depth, row in float_s_run.rows(unary_manifest)[:7]:
            float_s_run.one(row, [], depth, {})
        e3_float_s.g.finish()
        expected_float_s = {"start": "START",
                            "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                       for name, (mode, row) in e3_float_s.g.st.items()},
                            "seqs": [list(map(list, seq)) for seq in e3_float_s.g.seqs]}
        assert float_s_graph.read_bytes() == json.dumps(expected_float_s, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary float-s differs"
        int_graph = work / "parse2-unary-int.c.json"
        run(str(cgen), "inspect-parse2-unary-int-graph", str(int_graph))
        e3_int = parse2base.executor()
        assemble.Run(e3_int, e3_int.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_int, e3_int.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_int, e3_int.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_int, e3_int.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_int, e3_int.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_int, e3_int.P, {}, {}).run(float_manifest)
        assemble.Run(e3_int, e3_int.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        int_run = assemble.Run(e3_int, e3_int.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        int_run.root = unary_manifest.parent
        for depth, row in int_run.rows(unary_manifest)[:9]:
            int_run.one(row, [], depth, {})
        e3_int.g.finish()
        expected_int = {"start": "START",
                        "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                   for name, (mode, row) in e3_int.g.st.items()},
                        "seqs": [list(map(list, seq)) for seq in e3_int.g.seqs]}
        assert int_graph.read_bytes() == json.dumps(expected_int, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary integer conversions differ"
        part6_graph = work / "parse2-unary-part6.c.json"
        run(str(cgen), "inspect-parse2-unary-part6-graph", str(part6_graph))
        e3_part6 = parse2base.executor()
        assemble.Run(e3_part6, e3_part6.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_part6, e3_part6.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_part6, e3_part6.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_part6, e3_part6.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_part6, e3_part6.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_part6, e3_part6.P, {}, {}).run(float_manifest)
        assemble.Run(e3_part6, e3_part6.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        part6_run = assemble.Run(e3_part6, e3_part6.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        part6_run.root = unary_manifest.parent
        for depth, row in part6_run.rows(unary_manifest)[:10]:
            part6_run.one(row, [], depth, {})
        e3_part6.g.finish()
        expected_part6 = {"start": "START",
                          "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                     for name, (mode, row) in e3_part6.g.st.items()},
                          "seqs": [list(map(list, seq)) for seq in e3_part6.g.seqs]}
        assert part6_graph.read_bytes() == json.dumps(expected_part6, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 unary part6 differs"
        printf_head_graph = work / "parse2-printfallback-head.c.json"
        run(str(cgen), "inspect-parse2-printfallback-head-graph", str(printf_head_graph))
        e3_printf_head = parse2base.executor()
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_printf_head, e3_printf_head.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {}).run(float_manifest)
        assemble.Run(e3_printf_head, e3_printf_head.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        printf_head_unary_run = assemble.Run(e3_printf_head, e3_printf_head.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        printf_head_unary_run.root = unary_manifest.parent
        for depth, row in printf_head_unary_run.rows(unary_manifest)[:10]:
            printf_head_unary_run.one(row, [], depth, {})
        printf_manifest = ROOT / "exec/parse2/printfallback-manifest.tsv"
        printf_head_run = assemble.Run(e3_printf_head, e3_printf_head.P, {}, {})
        printf_head_run.root = printf_manifest.parent
        for depth, row in printf_head_run.rows(printf_manifest)[:2]:
            printf_head_run.one(row, [], depth, {})
        e3_printf_head.g.finish()
        expected_printf_head = {"start": "START",
                                "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                           for name, (mode, row) in e3_printf_head.g.st.items()},
                                "seqs": [list(map(list, seq)) for seq in e3_printf_head.g.seqs]}
        assert printf_head_graph.read_bytes() == json.dumps(expected_printf_head, separators=(",", ":")).encode(), \
            "seed/gen.c parse2 printfallback head differs"
        printf_dispatch_graph = work / "parse2-printfallback-dispatch.c.json"
        run(str(cgen), "inspect-parse2-printfallback-dispatch-graph", str(printf_dispatch_graph))
        e3_printf_dispatch = parse2base.executor()
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {}).run(float_manifest)
        assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        printf_dispatch_unary_run = assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        printf_dispatch_unary_run.root = unary_manifest.parent
        for depth, row in printf_dispatch_unary_run.rows(unary_manifest)[:10]:
            printf_dispatch_unary_run.one(row, [], depth, {})
        printf_manifest = ROOT / "exec/parse2/printfallback-manifest.tsv"
        printf_dispatch_run = assemble.Run(e3_printf_dispatch, e3_printf_dispatch.P, {}, {})
        printf_dispatch_run.root = printf_manifest.parent
        for depth, row in printf_dispatch_run.rows(printf_manifest)[:3]:
            printf_dispatch_run.one(row, [], depth, {})
        e3_printf_dispatch.g.finish()
        expected_printf_dispatch = {"start": "START",
                                    "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                               for name, (mode, row) in e3_printf_dispatch.g.st.items()},
                                    "seqs": [list(map(list, seq)) for seq in e3_printf_dispatch.g.seqs]}
        printf_dispatch_actual = printf_dispatch_graph.read_bytes()
        printf_dispatch_expected = json.dumps(expected_printf_dispatch, separators=(",", ":")).encode()
        if printf_dispatch_actual != printf_dispatch_expected:
            first = next((i for i, (a, b) in enumerate(zip(printf_dispatch_actual, printf_dispatch_expected))
                          if a != b), min(len(printf_dispatch_actual), len(printf_dispatch_expected)))
            raise AssertionError(f"seed/gen.c parse2 printfallback dispatch differs at {first}: "
                                 f"C={printf_dispatch_actual[first:first+100]!r} "
                                 f"Python={printf_dispatch_expected[first:first+100]!r}")
        printf_body_graph = work / "parse2-printfallback-body.c.json"
        run(str(cgen), "inspect-parse2-printfallback-body-graph", str(printf_body_graph))
        e3_printf_body = parse2base.executor()
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_printf_body, e3_printf_body.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {}).run(float_manifest)
        assemble.Run(e3_printf_body, e3_printf_body.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        printf_body_unary_run = assemble.Run(e3_printf_body, e3_printf_body.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        printf_body_unary_run.root = unary_manifest.parent
        for depth, row in printf_body_unary_run.rows(unary_manifest)[:10]:
            printf_body_unary_run.one(row, [], depth, {})
        printf_manifest = ROOT / "exec/parse2/printfallback-manifest.tsv"
        printf_body_run = assemble.Run(e3_printf_body, e3_printf_body.P, {}, {})
        printf_body_run.root = printf_manifest.parent
        for depth, row in printf_body_run.rows(printf_manifest)[:4]:
            printf_body_run.one(row, [], depth, {})
        e3_printf_body.g.finish()
        expected_printf_body = {"start": "START",
                                "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                           for name, (mode, row) in e3_printf_body.g.st.items()},
                                "seqs": [list(map(list, seq)) for seq in e3_printf_body.g.seqs]}
        printf_body_actual = printf_body_graph.read_bytes()
        printf_body_expected = json.dumps(expected_printf_body, separators=(",", ":")).encode()
        if printf_body_actual != printf_body_expected:
            first = next((i for i, (a, b) in enumerate(zip(printf_body_actual, printf_body_expected))
                          if a != b), min(len(printf_body_actual), len(printf_body_expected)))
            raise AssertionError(f"seed/gen.c parse2 printfallback body differs at {first}: "
                                 f"C={printf_body_actual[first:first+100]!r} "
                                 f"Python={printf_body_expected[first:first+100]!r}")
    if part == "parse2-4":
        for name, command, flags in (
            ("dimensions", "inspect-parse2-gen2-dimensions-graph",
             {"seg_types-dimensions": 1}),
            ("dimensions-tail", "inspect-parse2-gen2-dimensions-tail-graph",
             {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1}),
            ("type-prefix", "inspect-parse2-gen2-type-prefix-graph",
             {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1,
              "seg_types-prefix": 1}),
            ("type-typedef", "inspect-parse2-gen2-type-typedef-graph",
             {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1,
              "seg_types-prefix": 1, "seg_types-typedef": 1}),
            ("type-tail", "inspect-parse2-gen2-type-tail-graph",
             {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1,
              "seg_types-prefix": 1, "seg_types-typedef": 1,
              "seg_types-tail": 1}),
            ("type-word", "inspect-parse2-gen2-type-word-graph",
             {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1,
              "seg_types-prefix": 1, "seg_types-typedef": 1,
              "seg_types-tail": 1, "seg_types-word": 1}),
        ):
            actual_path = work / f"parse2-gen2-{name}.c.json"
            run(str(cgen), command, str(actual_path))
            expected_graph = parse2base.executor()
            assemble.Run(expected_graph, expected_graph.P, {}, flags).run(
                ROOT / "exec/parse2/gen2-manifest.tsv")
            expected_graph.g.finish()
            expected = json.dumps({
                "start": "START",
                "states": {state: [mode, {str(k): v for k, v in row.items()}]
                           for state, (mode, row) in expected_graph.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
            }, separators=(",", ":")).encode()
            actual = actual_path.read_bytes()
            if actual != expected:
                first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                              if a != b), min(len(actual), len(expected)))
                raise AssertionError(f"seed/gen.c gen2 {name} differs at {first}: "
                                     f"C={actual[first:first+100]!r} "
                                     f"Python={expected[first:first+100]!r}")
    if part == "parse2-5":
        actual_path = work / "parse2-gen2-type-entry.c.json"
        run(str(cgen), "inspect-parse2-gen2-type-entry-graph", str(actual_path))
        expected_graph = parse2base.executor()
        flags = {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1,
                 "seg_types-prefix": 1, "seg_types-typedef": 1,
                 "seg_types-tail": 1, "seg_types-word": 1,
                 "seg_types-entry": 1}
        assemble.Run(expected_graph, expected_graph.P, {}, flags).run(
            ROOT / "exec/parse2/gen2-manifest.tsv")
        expected_graph.g.finish()
        expected = json.dumps({
            "start": "START",
            "states": {state: [mode, {str(k): v for k, v in row.items()}]
                       for state, (mode, row) in expected_graph.g.st.items()},
            "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
        }, separators=(",", ":")).encode()
        actual = actual_path.read_bytes()
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                          if a != b), min(len(actual), len(expected)))
            raise AssertionError(f"seed/gen.c gen2 type-entry differs at {first}: "
                                 f"C={actual[first:first+100]!r} "
                                 f"Python={expected[first:first+100]!r}")
        tail_path = work / "parse2-gen2-tytail.c.json"
        run(str(cgen), "inspect-parse2-gen2-tytail-graph", str(tail_path))
        tail_graph = parse2base.executor()
        tail_flags = dict(flags, seg_tytail=1)
        assemble.Run(tail_graph, tail_graph.P, {}, tail_flags).run(
            ROOT / "exec/parse2/gen2-manifest.tsv")
        tail_graph.g.finish()
        tail_expected = json.dumps({
            "start": "START",
            "states": {state: [mode, {str(k): v for k, v in row.items()}]
                       for state, (mode, row) in tail_graph.g.st.items()},
            "seqs": [list(map(list, seq)) for seq in tail_graph.g.seqs],
        }, separators=(",", ":")).encode()
        tail_actual = tail_path.read_bytes()
        if tail_actual != tail_expected:
            first = next((i for i, (a, b) in enumerate(zip(tail_actual, tail_expected))
                          if a != b), min(len(tail_actual), len(tail_expected)))
            raise AssertionError(f"seed/gen.c gen2 tytail differs at {first}: "
                                 f"C={tail_actual[first:first+100]!r} "
                                 f"Python={tail_expected[first:first+100]!r}")
    if part == "parse2-6":
        for name, command, flags in (
            ("ladder-reject-e", "inspect-parse2-gen2-ladder-reject-e-graph",
             {"seg_ladder-E-reject": 1}),
            ("ladder-reject-ec", "inspect-parse2-gen2-ladder-reject-ec-graph",
             {"seg_ladder-E-reject": 1, "seg_ladder-C-reject": 1}),
            ("ladder-e", "inspect-parse2-gen2-ladder-e-graph",
             {"seg_ladder-E": 1}),
            ("ladder-c", "inspect-parse2-gen2-ladder-c-graph",
             {"seg_ladder-C": 1}),
        ):
            actual_path = work / f"parse2-gen2-{name}.c.json"
            run(str(cgen), command, str(actual_path))
            expected_graph = parse2base.executor()
            assemble.Run(expected_graph, expected_graph.P, {}, flags).run(
                ROOT / "exec/parse2/gen2-manifest.tsv")
            expected_graph.g.finish()
            expected = json.dumps({
                "start": "START",
                "states": {state: [mode, {str(k): v for k, v in row.items()}]
                           for state, (mode, row) in expected_graph.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
            }, separators=(",", ":")).encode()
            actual = actual_path.read_bytes()
            if actual != expected:
                first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                              if a != b), min(len(actual), len(expected)))
                raise AssertionError(f"seed/gen.c gen2 {name} differs at {first}: "
                                     f"C={actual[first:first+100]!r} "
                                     f"Python={expected[first:first+100]!r}")
    if part == "parse2-7":
        manifest = ROOT / "exec/parse2/gen2-manifest.tsv"
        rows = assemble.Run.rows(manifest)
        for name, command, end in (
            ("prefix", "inspect-parse2-gen2-operator-prefix-graph", 54),
            ("select", "inspect-parse2-gen2-operator-select-graph", 56),
            ("body", "inspect-parse2-gen2-operator-body-graph", 59),
            ("reject", "inspect-parse2-gen2-operator-reject-graph", 61),
        ):
            actual_path = work / f"parse2-gen2-operator-{name}.c.json"
            run(str(cgen), command, str(actual_path))
            expected_graph = parse2base.executor()
            runner = assemble.Run(expected_graph, expected_graph.P, {}, {})
            runner.root = manifest.parent
            runner.one(rows[47][1], rows[48:end], 0, {"seg_optail": 1})
            expected_graph.g.finish()
            expected = json.dumps({
                "start": "START",
                "states": {state: [mode, {str(k): v for k, v in row.items()}]
                           for state, (mode, row) in expected_graph.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
            }, separators=(",", ":")).encode()
            actual = actual_path.read_bytes()
            if actual != expected:
                first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                              if a != b), min(len(actual), len(expected)))
                raise AssertionError(f"seed/gen.c gen2 operator-{name} differs at {first}: "
                                     f"C={actual[first:first+100]!r} "
                                     f"Python={expected[first:first+100]!r}")
    if part == "parse2-8":
        manifest = ROOT / "exec/parse2/gen2-manifest.tsv"
        rows = assemble.Run.rows(manifest)
        for name, command, end in (
            ("pointer", "inspect-parse2-gen2-operator-pointer-graph", 62),
            ("full", "inspect-parse2-gen2-operator-full-graph", 63),
        ):
            actual_path = work / f"parse2-gen2-operator-{name}.c.json"
            run(str(cgen), command, str(actual_path))
            expected_graph = parse2base.executor()
            runner = assemble.Run(expected_graph, expected_graph.P, {}, {})
            runner.root = manifest.parent
            runner.one(rows[47][1], rows[48:end], 0, {"seg_optail": 1})
            expected_graph.g.finish()
            expected = json.dumps({
                "start": "START",
                "states": {state: [mode, {str(k): v for k, v in row.items()}]
                           for state, (mode, row) in expected_graph.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
            }, separators=(",", ":")).encode()
            actual = actual_path.read_bytes()
            if actual != expected:
                first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                              if a != b), min(len(actual), len(expected)))
                raise AssertionError(f"seed/gen.c gen2 operator-{name} differs at {first}: "
                                     f"C={actual[first:first+100]!r} "
                                     f"Python={expected[first:first+100]!r}")
    if part == "parse2-9":
        actual_path = work / "parse2-gen2-ladders.c.json"
        run(str(cgen), "inspect-parse2-gen2-ladders-graph", str(actual_path), timeout=50)
        expected_graph = parse2base.executor()
        assemble.Run(expected_graph, expected_graph.P, {}, {"seg_ladders": 1}).run(
            ROOT / "exec/parse2/gen2-manifest.tsv")
        expected_graph.g.finish()
        expected = json.dumps({
            "start": "START",
            "states": {state: [mode, {str(k): v for k, v in row.items()}]
                       for state, (mode, row) in expected_graph.g.st.items()},
            "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
        }, separators=(",", ":")).encode()
        actual = actual_path.read_bytes()
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                          if a != b), min(len(actual), len(expected)))
            raise AssertionError(f"seed/gen.c gen2 ladders differs at {first}: "
                                 f"C={actual[first:first+100]!r} "
                                 f"Python={expected[first:first+100]!r}")
    if part == "parse2-10":
        for name, command, flags in (
            ("staticauto", "inspect-parse2-gen2-staticauto-graph",
             {"seg_staticauto": 1}),
            ("statics-guard", "inspect-parse2-gen2-statics-guard-graph",
             {"seg_staticauto": 1, "seg_startup-guard": 1}),
        ):
            actual_path = work / f"parse2-gen2-{name}.c.json"
            run(str(cgen), command, str(actual_path))
            expected_graph = parse2base.executor()
            assemble.Run(expected_graph, expected_graph.P, {}, flags).run(
                ROOT / "exec/parse2/gen2-manifest.tsv")
            expected_graph.g.finish()
            expected = json.dumps({
                "start": "START",
                "states": {state: [mode, {str(k): v for k, v in row.items()}]
                           for state, (mode, row) in expected_graph.g.st.items()},
                "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
            }, separators=(",", ":")).encode()
            actual = actual_path.read_bytes()
            if actual != expected:
                first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                              if a != b), min(len(actual), len(expected)))
                raise AssertionError(f"seed/gen.c gen2 {name} differs at {first}: "
                                     f"C={actual[first:first+100]!r} "
                                     f"Python={expected[first:first+100]!r}")
    if part == "parse2-12":
        actual_path = work / "parse2-gen2-initializers-hook.c.json"
        run(str(cgen), "inspect-parse2-gen2-initializers-hook-graph", str(actual_path))
        expected_graph = parse2base.executor()
        assemble.Run(expected_graph, expected_graph.P, {},
                     {"seg_types-dimensions": 1, "seg_types-dimensions-tail": 1}).run(
                         ROOT / "exec/parse2/gen2-manifest.tsv")
        assemble.Run(expected_graph, expected_graph.P, {},
                     assemble.load_facts("k2-gen2")["staticsenv"]).run(
                         ROOT / "exec/parse2/statics-manifest.tsv")
        runner = assemble.Run(expected_graph, expected_graph.P, {}, {})
        manifest = ROOT / "exec/parse2/initializers-manifest.tsv"
        runner.root = manifest.parent
        for depth, row in runner.rows(manifest)[:2]:
            assert depth == 0
            runner.one(row, [], depth, {})
        expected_graph.g.finish()
        expected = json.dumps({
            "start": "START",
            "states": {state: [mode, {str(k): v for k, v in row.items()}]
                       for state, (mode, row) in expected_graph.g.st.items()},
            "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
        }, separators=(",", ":")).encode()
        actual = actual_path.read_bytes()
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                          if a != b), min(len(actual), len(expected)))
            raise AssertionError(f"seed/gen.c initializers hook differs at {first}: "
                                 f"C={actual[first:first+100]!r} "
                                 f"Python={expected[first:first+100]!r}")
    if part == "parse2-11":
        actual_path = work / "parse2-gen2-statics.c.json"
        run(str(cgen), "inspect-parse2-gen2-statics-graph", str(actual_path))
        expected_graph = parse2base.executor()
        env = assemble.load_facts("k2-gen2")["staticsenv"]
        assemble.Run(expected_graph, expected_graph.P, {}, env).run(
            ROOT / "exec/parse2/statics-manifest.tsv")
        expected_graph.g.finish()
        expected = json.dumps({
            "start": "START",
            "states": {state: [mode, {str(k): v for k, v in row.items()}]
                       for state, (mode, row) in expected_graph.g.st.items()},
            "seqs": [list(map(list, seq)) for seq in expected_graph.g.seqs],
        }, separators=(",", ":")).encode()
        actual = actual_path.read_bytes()
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected))
                          if a != b), min(len(actual), len(expected)))
            raise AssertionError(f"seed/gen.c gen2 statics differs at {first}: "
                                 f"C={actual[first:first+100]!r} "
                                 f"Python={expected[first:first+100]!r}")
    if part == "parse2-3":
        initializer_manifest = ROOT / "exec/parse2/strings-initializer-manifest.tsv"
        float_manifest = ROOT / "exec/parse2/floatconst-manifest.tsv"
        unary_manifest = ROOT / "exec/parse2/unarycontrol-manifest.tsv"
        unaryenv = assemble.load_facts("k2-gen2")["unaryenv"]
        printf_all_graph = work / "parse2-unary-part10.c.json"
        run(str(cgen), "inspect-parse2-unary-part10-graph", str(printf_all_graph))
        e3_printf_all = parse2base.executor()
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {"part_startup": 1}).run(
            ROOT / "exec/parse2/gen2parts-manifest.tsv")
        assemble.Run(e3_printf_all, e3_printf_all.P, {},
                     {"control_section": "startup-marker", "statement": "STMT",
                      "extra": {}, "seqb": {}}).run(ROOT / "exec/parse2/control-manifest.tsv")
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {"TK_STR": assemble.load_facts("parse-constants")["TK_STR"]}).run(
            ROOT / "exec/parse2/strings-token-span-manifest.tsv")
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {}).run(initializer_manifest)
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {}).run(ROOT / "exec/parse/numeric-manifest.tsv")
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {}).run(float_manifest)
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {}).run(ROOT / "exec/parse/autoscan-manifest.tsv")
        printf_all_unary_run = assemble.Run(e3_printf_all, e3_printf_all.P, {},
                               {"warnings": False, "ucx": unaryenv["ucx"],
                                "ufacts": unaryenv["ufacts"]})
        printf_all_unary_run.root = unary_manifest.parent
        for depth, row in printf_all_unary_run.rows(unary_manifest)[:10]:
            printf_all_unary_run.one(row, [], depth, {})
        printf_manifest = ROOT / "exec/parse2/printf-manifest.tsv"
        printf_all_run = assemble.Run(e3_printf_all, e3_printf_all.P, {}, {"warnings": False})
        printf_all_run.root = printf_manifest.parent
        for depth, row in printf_all_run.rows(printf_manifest)[:4]:
            printf_all_run.one(row, [], depth, {})
        append_manifest = ROOT / "exec/parse2/printfcontrol-1-manifest.tsv"
        append_run = assemble.Run(e3_printf_all, e3_printf_all.P, {"warnings": False}, {})
        append_run.root = append_manifest.parent
        for depth, row in append_run.rows(append_manifest)[:3]:
            append_run.one(row, [], depth, {})
        fmt_manifest = ROOT / "exec/parse2/fmtwalk-manifest.tsv"
        fmt_run = assemble.Run(e3_printf_all, e3_printf_all.P, {},
                               {"pre": "PF", "on_byte": "PF.b", "on_d": "PF.d", "on_end": "PF.end"})
        fmt_run.root = fmt_manifest.parent
        fmt_run.block(fmt_run.rows(fmt_manifest), 0, {})
        assemble.Run(e3_printf_all, e3_printf_all.P, {}, {}).run(
            ROOT / "exec/parse2/printfcontrol-2-manifest.tsv")
        assemble.Run(e3_printf_all, e3_printf_all.P, {},
                     {"pre": "PL", "body": "PL.cp", "done": "PL.end"}).run(
            ROOT / "exec/parse2/strwalk-manifest.tsv")
        wide_manifest = ROOT / "exec/parse2/printf-manifest.tsv"
        wide_run = assemble.Run(e3_printf_all, e3_printf_all.P, {},
                                assemble.load_facts("k2-strings")["rej"])
        wide_run.root = wide_manifest.parent
        for depth, row in wide_run.rows(wide_manifest)[8:9]:
            wide_run.one(row, [], depth, {})
        escape_manifest = ROOT / "exec/parse2/printfcontrol-3-manifest.tsv"
        escape_run = assemble.Run(e3_printf_all, e3_printf_all.P, {}, {})
        escape_run.root = escape_manifest.parent
        for depth, row in escape_run.rows(escape_manifest)[:2]:
            escape_run.one(row, [], depth, {})
        for depth, row in escape_run.rows(escape_manifest)[2:3]:
            escape_run.one(row, [], depth, {})
        po_run = assemble.Run(e3_printf_all, e3_printf_all.P, {},
                              {"pre": "PO", "on_byte": "PO.b", "on_d": "PO.d", "on_end": "PO.end"})
        po_run.root = fmt_manifest.parent
        po_run.block(po_run.rows(fmt_manifest), 0, {})
        control4_manifest = ROOT / "exec/parse2/printfcontrol-4-manifest.tsv"
        control4_run = assemble.Run(e3_printf_all, e3_printf_all.P, {}, {})
        control4_run.root = control4_manifest.parent
        for depth, row in control4_run.rows(control4_manifest)[:2]:
            control4_run.one(row, [], depth, {})
        depth, row = printf_all_unary_run.rows(unary_manifest)[11]
        printf_all_unary_run.one(row, [], depth, {})
        addr_entry = printf_all_unary_run.env["f_part8_1216_U_r_1"]
        addr_manifest = ROOT / "exec/parse2/addr-manifest.tsv"
        addr_run = assemble.Run(e3_printf_all, e3_printf_all.P, {},
                                {"entry": addr_entry, "pending": []})
        addr_run.root = addr_manifest.parent
        for depth, row in addr_run.rows(addr_manifest)[:5]:
            addr_run.one(row, [], depth, {})
        printf_all_unary_run.env["addrenv"] = addr_run.env
        for depth, row in printf_all_unary_run.rows(unary_manifest)[13:15]:
            printf_all_unary_run.one(row, [], depth, {})
        e3_printf_all.g.finish()
        expected_printf_all = {"start": "START",
                               "states": {name: [mode, {str(k): v for k, v in row.items()}]
                                          for name, (mode, row) in e3_printf_all.g.st.items()},
                               "seqs": [list(map(list, seq)) for seq in e3_printf_all.g.seqs]}
        actual = printf_all_graph.read_bytes()
        expected = json.dumps(expected_printf_all, separators=(",", ":")).encode()
        if actual != expected:
            first = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b),
                         min(len(actual), len(expected)))
            raise AssertionError(f"seed/gen.c parse2 unary part10 differs at {first}: "
                                 f"C={actual[first:first+100]!r} Python={expected[first:first+100]!r}")
    if part in ("base", "all"):
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
if part in ("base", "all"):
    print("seed construct base: net/tbl and prune, opt, lex, pp, nativeabi byte-identical")
if part in ("parse2", "all"):
    print("seed construct parse2: token prelude through autoscan graph byte-identical")
if part in ("parse2-2", "all"):
    print("seed construct parse2-2: unary control through printfallback body graph byte-identical")
if part == "parse2-3":
    print("seed construct parse2-3: unary part10 graph byte-identical")
if part == "parse2-4":
    print("seed construct parse2-4: gen2 dimensions through type-word graphs byte-identical")
if part == "parse2-5":
    print("seed construct parse2-5: gen2 type-entry and tytail graphs byte-identical")
if part == "parse2-6":
    print("seed construct parse2-6: gen2 E/C ladders and rejection graphs byte-identical")
if part == "parse2-7":
    print("seed construct parse2-7: gen2 operator prefix through float-reject graphs byte-identical")
if part == "parse2-8":
    print("seed construct parse2-8: gen2 operator pointer and full graphs byte-identical")
if part == "parse2-9":
    print("seed construct parse2-9: gen2 ladder family graph byte-identical")
if part == "parse2-10":
    print("seed construct parse2-10: gen2 staticauto and startup-guard graphs byte-identical")
if part == "parse2-11":
    print("seed construct parse2-11: gen2 statics graph byte-identical")
if part == "parse2-12":
    print("seed construct parse2-12: gen2 initializers hook graph byte-identical")
