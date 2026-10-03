#!/usr/bin/env python3
"""Generic stage entry driver (K2): exec/build/gen.py STAGE OUT.json [--FLAG...]
(OUT.json STAGE is accepted too, for callers that run `SCRIPT OUT ARGS...`.)

Runs exec/STAGE/gen-manifest.tsv (or exec/STAGE-manifest.tsv for STAGE = stage/sub) through exec/assemble.py and writes the graph.
Manifest header lines read here:
  #! base PATH            executor module (relative to exec/), loaded as E
  #! flags A B C          accepted --A/--B/--C; each becomes flags[A]=True/False
  #! start NAME           start state (default START)
  #! graph CLASS          E.g = E.CLASS() (a base without its own g/P, e.g. build/graph.py)
  #! start $NAME          the start state is env[NAME] after the run
  #! domains STEM         E.g.finish(start, {mode: values}) from header-form facts STEM (mode, values)
  #! extra KEY STEM       output KEY = fact KEY of exec/facts/STEM.tsv
Everything else (exclusive flags, env values, sub-manifests) is manifest rows.
"""
import importlib.util, json, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT)); sys.path.insert(0, str(ROOT / "exec"))
import assemble

if len(sys.argv) < 3:
    sys.exit("usage: exec/build/gen.py STAGE OUT.json [--FLAG...]")
def _manifest(a):   # STAGE -> exec/STAGE/gen-manifest.tsv; STAGE/SUB -> exec/STAGE/SUB-manifest.tsv
    return ROOT / "exec" / a / "gen-manifest.tsv" if "/" not in a else ROOT / "exec" / (a + "-manifest.tsv")
def _stage(a):
    return not a.endswith(".json") and a.count("/") <= 1 and _manifest(a).is_file()
stage, out, args = sys.argv[1], sys.argv[2], sys.argv[3:]
if not _stage(stage) and _stage(out):   # OUT STAGE order (script OUT ARGS callers)
    stage, out = out, stage
if not _stage(stage):
    sys.exit("exec/build/gen.py: no stage manifest exec/%s/gen-manifest.tsv or exec/%s-manifest.tsv (args %s %s)" % (stage, stage, sys.argv[1], sys.argv[2]))
man = _manifest(stage)
head = {}
for ln in man.read_text().split("\n"):
    if ln.startswith("#! "):
        k, _, v = ln[3:].partition(" ")
        head[k] = v.split()
names = head.get("flags", [])
if len(args) != len(set(args)) or any(not a.startswith("--") or a[2:] not in names for a in args):
    sys.exit("usage: exec/build/gen.py %s OUT.json %s" % (stage, " ".join("[--%s]" % n for n in names)))
spec = importlib.util.spec_from_file_location(stage.replace("/", "_") + "base", ROOT / "exec" / head["base"][0])
E = importlib.util.module_from_spec(spec)
spec.loader.exec_module(E)
if "graph" in head:
    E.g, E.P = getattr(E, head["graph"][0])(), getattr(E, "P", None)
# Driver data channel: the top-level run environment is E.results, so names a row stores with
# export / result= are readable by executor code later in the same build (E.results[name]).
E.results = {}
env = assemble.Run(E, E.P, {n: "--" + n in args for n in names}, E.results).run(man)
start = head.get('start', ['START'])[0]
start = env[start[1:]] if start.startswith('$') else start
if "domains" in head:
    s = head["domains"][0]
    E.g.finish(start, {r["mode"]: r["values"] for r in assemble.load_facts(s)[s]})
else:
    E.g.finish()
d = {'start': start, 'states': {n: [m, {str(k): v for k, v in row.items()}] for n, (m, row) in E.g.st.items()},
     'seqs': [list(map(list, s)) for s in E.g.seqs]}
for k, s in (head["extra"][i:i + 2] for i in range(0, len(head.get("extra", [])), 2)):
    d[k] = assemble.load_facts(s)[k]
pathlib.Path(out).write_text(json.dumps(d, separators=(',', ':')))
print(stage + ' states', len(d['states']), file=sys.stderr)
