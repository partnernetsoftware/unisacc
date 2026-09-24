#!/bin/bash
# ujs2wasm step suite — path B host inject + run_step fold vs jtape.
#
#   ./tests/ujs2wasm_step.sh
#
# Requires: python3, node, wat2wasm. Bounded per-case timeouts.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

command -v node >/dev/null || { echo "ujs2wasm_step needs node"; exit 1; }
command -v wat2wasm >/dev/null || { echo "ujs2wasm_step needs wat2wasm"; exit 1; }

OUT="${TMPDIR:-/tmp}/ujs2wasm-step.$$"
mkdir -p "$OUT"
trap 'rm -rf "$OUT"' EXIT

# macOS has no timeout(1); perl alarm is the local convention.
run_alarm() {
  local secs="$1"; shift
  perl -e 'alarm shift; exec @ARGV' "$secs" "$@"
}

echo "== ujs2wasm_step ABI surface =="
python3 - <<'PY'
import json, subprocess, sys, tempfile
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.emit_wasm import emit_wasm
from ujs.construct.oracle import Oracle

src = "xs[0] = xs[0] + 1.0; return xs[0];"
o = Oracle(drive="gold")
fn = compile_src(src, o)
need = [
    "clear_slots", "run_step", "host_reset", "host_run",
    "host_set_global", "host_get_global", "host_mk_f64", "host_mk_list",
    "host_list_set", "host_list_get", "host_len",
    "host_dict_key", "host_dict_val", "tag_of_export", "f64_of_export",
    "memory",
]
with tempfile.TemporaryDirectory() as td:
    wasm = Path(td) / "t.wasm"
    emit_wasm(fn, str(wasm), o)
    got = set(json.loads(subprocess.check_output(
        ["node", "--input-type=module", "-e",
         "import fs from 'fs';"
         "const {instance}=await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));"
         "process.stdout.write(JSON.stringify(Object.keys(instance.exports)));",
         str(wasm)], text=True)))
missing = [n for n in need if n not in got]
if missing:
    print("MISSING EXPORTS", missing)
    sys.exit(1)
print("abi ok", len(need), "exports present")
PY

echo "== ujs2wasm_step corpus (inject + run_step) =="
run_alarm 60 python3 - <<PY
import json, os, subprocess, sys
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.emit_wasm import emit_wasm
from ujs.construct.oracle import Oracle
from ujs.construct import api
from ujs.construct import value as V

root = Path("$ROOT")
out = Path("$OUT")
corpus = root / "tests/ujs2wasm/step_corpus"
expect = json.loads((root / "tests/ujs2wasm/step_expect.json").read_text())
runner = root / "tests/ujs2wasm/run_step.mjs"
o = Oracle(drive="gold")

def close(a, b, eps=1e-9):
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(float(a) - float(b)) < eps
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(close(x, y) for x, y in zip(a, b))
    if isinstance(a, dict) and isinstance(b, dict):
        return set(a) == set(b) and all(close(a[k], b[k]) for k in a)
    return a == b

bad = 0
for name, case in sorted(expect.items()):
    src_path = corpus / (name + ".ujs")
    if not src_path.is_file():
        print("MISSING", src_path)
        bad += 1
        continue
    src = src_path.read_text()
    G = case["globals"]
    steps = int(case.get("steps", 1))
    want = case["want"]

    cur = dict(G)
    jv = None
    jfail = False
    for _ in range(steps):
        r = api.run(src, globals_map=cur, backend="jtape", oracle=o, mutate_globals=True)
        if not r.ok:
            print("JTAPE FAIL", name, r.err)
            bad += 1
            jfail = True
            break
        jv = V.to_py(r.value)
        if isinstance(jv, dict):
            for k, v in jv.items():
                if k in ("hit", "n"):
                    continue
                cur[k] = v
    if jfail:
        continue
    if not close(jv, want):
        print("JTAPE WANT", name, jv, want)
        bad += 1
        continue

    fn = compile_src(src, o)
    wasm = out / (name + ".wasm")
    meta = emit_wasm(fn, str(wasm), o)
    meta_path = out / (name + ".meta.json")
    g_path = out / (name + ".g.json")
    meta_path.write_text(json.dumps({
        "globals": meta["globals"], "locals": meta["locals"],
    }))
    g_path.write_text(json.dumps(G))
    got = json.loads(subprocess.check_output(
        ["node", str(runner), str(wasm), str(meta_path), str(g_path), str(steps)],
        text=True, cwd=str(root)))
    if not close(got, want):
        print("STEP MISMATCH", name, "got", got, "want", want)
        bad += 1
        continue
    if not close(got, jv):
        print("FOLD FAIL", name, "jtape", jv, "wasm", got)
        bad += 1
        continue
    print("ok", name, "steps", steps, meta["bytes"], "B")

print("%d/%d corpus" % (len(expect) - bad, len(expect)))
sys.exit(1 if bad else 0)
PY

echo "== ujs2wasm_step sim.ujs (small N, multi-step fold) =="
run_alarm 120 python3 - <<PY
import json, subprocess, sys
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.emit_wasm import emit_wasm, can_emit_direct
from ujs.construct.oracle import Oracle
from ujs.construct import api
from ujs.construct import value as V

root = Path("$ROOT")
out = Path("$OUT")
src = (root / "ujs/web/game/sim.ujs").read_text()
o = Oracle(drive="gold")
fn = compile_src(src, o)
ok, why = can_emit_direct(fn, o)
if not ok:
    print("NOT READY", why)
    sys.exit(1)

N, STEPS = 16, 3

def fresh():
    xs, ys, zs, vxs, vys, vzs, rs = [], [], [], [], [], [], []
    for i in range(N):
        sx = float(((i * 13) % 37) - 18)
        sy = float(((i * 7) % 25) - 12)
        if -3.5 < sx < 3.5 and -3.5 < sy < 3.5:
            sx += 7.0
        xs.append(sx); ys.append(sy); zs.append(-40.0 - i * 1.55)
        vxs.append(((i % 5) - 2) * 0.55)
        vys.append(((i % 3) - 1) * 0.4)
        vzs.append(8.0 + (i % 11) * 0.35)
        rs.append(0.55 + (i % 5) * 0.22)
    return dict(
        xs=xs, ys=ys, zs=zs, vxs=vxs, vys=vys, vzs=vzs, rs=rs,
        px=0.0, py=0.0, pz=0.0, score=0.0, alive=1,
        ix=0.0, iy=-1.0, dt=0.016,
    )

def close(a, b, eps=1e-9):
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(float(a) - float(b)) < eps
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(close(x, y) for x, y in zip(a, b))
    if isinstance(a, dict) and isinstance(b, dict):
        return set(a) == set(b) and all(close(a[k], b[k]) for k in a)
    return a == b

G0 = fresh()
cur = dict(G0)
jv = None
for _ in range(STEPS):
    r = api.run(src, globals_map=cur, backend="jtape", oracle=o, mutate_globals=True)
    if not r.ok:
        print("JTAPE FAIL", r.err)
        sys.exit(1)
    jv = V.to_py(r.value)
    for k, v in jv.items():
        if k in ("hit", "n"):
            continue
        cur[k] = v

wasm = out / "sim.wasm"
meta = emit_wasm(fn, str(wasm), o)
meta_path = out / "sim.meta.json"
g_path = out / "sim.g.json"
meta_path.write_text(json.dumps({"globals": meta["globals"], "locals": meta["locals"]}))
g_path.write_text(json.dumps(G0))
got = json.loads(subprocess.check_output(
    ["node", str(root / "tests/ujs2wasm/run_step.mjs"),
     str(wasm), str(meta_path), str(g_path), str(STEPS)],
    text=True, cwd=str(root)))
if not close(jv, got):
    print("SIM FOLD FAIL")
    for k in sorted(jv):
        if not close(jv[k], got.get(k)):
            print("DIFF", k, "jtape", jv[k] if not isinstance(jv[k], list) else ("list", len(jv[k])),
                  "wasm", got.get(k) if not isinstance(got.get(k), list) else ("list", len(got.get(k) or [])))
    sys.exit(1)
print("sim fold ok", "N", N, "steps", STEPS, meta["bytes"], "B",
      "score", got["score"], "alive", got["alive"])
PY

echo "== ujs2wasm_step sim.ujs (ship N=480, multi-step fold) =="
run_alarm 180 python3 - <<PY
import json, subprocess, sys
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.emit_wasm import emit_wasm
from ujs.construct.oracle import Oracle
from ujs.construct import api
from ujs.construct import value as V

root = Path("$ROOT")
out = Path("$OUT")
src = (root / "ujs/web/game/sim.ujs").read_text()
o = Oracle(drive="gold")
fn = compile_src(src, o)
N, STEPS = 480, 5

def fresh():
    xs, ys, zs, vxs, vys, vzs, rs = [], [], [], [], [], [], []
    for i in range(N):
        sx = float(((i * 13) % 37) - 18)
        sy = float(((i * 7) % 25) - 12)
        if -3.5 < sx < 3.5 and -3.5 < sy < 3.5:
            sx += 7.0
        xs.append(sx); ys.append(sy); zs.append(-40.0 - i * 1.55)
        vxs.append(((i % 5) - 2) * 0.55)
        vys.append(((i % 3) - 1) * 0.4)
        vzs.append(8.0 + (i % 11) * 0.35)
        rs.append(0.55 + (i % 5) * 0.22)
    return dict(
        xs=xs, ys=ys, zs=zs, vxs=vxs, vys=vys, vzs=vzs, rs=rs,
        px=0.0, py=0.0, pz=0.0, score=0.0, alive=1,
        ix=1.0, iy=0.0, dt=0.016,
    )

def close(a, b, eps=1e-9):
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(float(a) - float(b)) < eps
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(close(x, y) for x, y in zip(a, b))
    if isinstance(a, dict) and isinstance(b, dict):
        return set(a) == set(b) and all(close(a[k], b[k]) for k in a)
    return a == b

G0 = fresh()
wasm = out / "sim480.wasm"
meta = emit_wasm(fn, str(wasm), o)
meta_path = out / "sim480.meta.json"
g_path = out / "sim480.g.json"
meta_path.write_text(json.dumps({"globals": meta["globals"], "locals": meta["locals"]}))
state = dict(G0)
for step in range(STEPS):
    G = dict(state)
    G["ix"] = 1.0 if step % 2 == 0 else -0.5
    G["iy"] = 0.0
    G["dt"] = 0.016
    g_path.write_text(json.dumps(G))
    got = json.loads(subprocess.check_output(
        ["node", str(root / "tests/ujs2wasm/run_step.mjs"),
         str(wasm), str(meta_path), str(g_path), "1"],
        text=True, cwd=str(root)))
    r = api.run(src, globals_map=dict(G), backend="jtape", oracle=o, mutate_globals=True)
    assert r.ok, r.err
    jt = V.to_py(r.value)
    if not close(got, jt):
        print("SHIP FOLD FAIL step", step)
        sys.exit(1)
    state = {k: got[k] for k in (
        "xs", "ys", "zs", "vxs", "vys", "vzs", "rs",
        "px", "py", "pz", "score", "alive")}
print("sim ship fold ok", "N", N, "steps", STEPS,
      "px", state["px"], "score", state["score"])
PY

echo "== ujs2wasm_step clear_slots preserves inject path =="
run_alarm 30 python3 - <<'PY'
import json, subprocess, tempfile
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.emit_wasm import emit_wasm
from ujs.construct.oracle import Oracle

# host_reset clears; inject; run_step; clear_slots alone must not free heap mid-flight
# — here we only assert clear_slots export zeroes a slot we set without run.
src = "return g;"
o = Oracle(drive="gold")
fn = compile_src(src, o)
js = r'''
import fs from "fs";
const {instance} = await WebAssembly.instantiate(fs.readFileSync(process.argv[1]));
const ex = instance.exports;
ex.host_reset();
const h = ex.host_mk_f64(3.25);
ex.host_set_global(0, h);
if (ex.host_get_global(0) !== h) throw new Error("set");
ex.clear_slots();
if (ex.host_get_global(0) !== 0) throw new Error("clear_slots");
// re-inject without host_reset (heap still live) then run_step
const h2 = ex.host_mk_f64(7.5);
ex.host_set_global(0, h2);
const r = ex.run_step();
const v = ex.f64_of_export(r);
if (Math.abs(v - 7.5) > 1e-9) throw new Error("run_step got " + v);
console.log("clear_slots+run_step ok");
'''
with tempfile.TemporaryDirectory() as td:
    wasm = Path(td) / "c.wasm"
    meta = emit_wasm(fn, str(wasm), o)
    assert meta["globals"] == ["g"], meta
    subprocess.check_call(["node", "--input-type=module", "-e", js, str(wasm)])
PY

echo "ujs2wasm_step suite OK"
