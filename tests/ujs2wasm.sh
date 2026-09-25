#!/bin/bash
# ujs2wasm suite — construct / full-emit corpus (Paper B 构造脊对照)
#   direct \\0asm via Python tools + fold parity + optional tinyvm.
#   Full expect.json ≠ UJS-1_ship: product core fold is ujs2wasm_compiler.sh §6.2.
#
#   ./tests/ujs2wasm.sh
#
# Requires: python3, node, wat2wasm.  tinyvm optional (module validate).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export UJS2WASM_ROOT="$ROOT"
export UJS2WASM_CORPUS="$ROOT/tests/ujs2wasm/corpus"
export UJS2WASM_NEG="$ROOT/tests/ujs2wasm/neg"
export UJS2WASM_EXPECT="$ROOT/tests/ujs2wasm/expect.json"
export UJS2WASM_RUNNER="$ROOT/tests/ujs2wasm/run_wasm.mjs"
OUT="${TMPDIR:-/tmp}/ujs2wasm-suite.$$"
mkdir -p "$OUT"
export UJS2WASM_OUT="$OUT"
trap 'rm -rf "$OUT"' EXIT

find_tinyvm() {
  if command -v tinyvm >/dev/null 2>&1; then command -v tinyvm; return; fi
  for c in \
    "$ROOT/../tinyvm/target/release/tinyvm" \
    "$ROOT/../tinyvm/target/debug/tinyvm" \
    "$HOME/repos/tinyvm/target/release/tinyvm" \
    "$HOME/repos/tinyvm/target/debug/tinyvm"
  do
    if [[ -x "$c" ]]; then echo "$c"; return; fi
  done
  return 1
}

TINYVM="$(find_tinyvm || true)"
export UJS2WASM_TINYVM="$TINYVM"

command -v node >/dev/null || { echo "ujs2wasm suite needs node"; exit 1; }
command -v wat2wasm >/dev/null || { echo "ujs2wasm suite needs wat2wasm"; exit 1; }

echo "== ujs2wasm corpus (direct) =="
python3 - <<'PY'
import json, os, subprocess, sys
from pathlib import Path

root = Path(os.environ["UJS2WASM_ROOT"])
corpus = Path(os.environ["UJS2WASM_CORPUS"])
expect = json.loads(Path(os.environ["UJS2WASM_EXPECT"]).read_text())
out = Path(os.environ["UJS2WASM_OUT"])
runner = Path(os.environ["UJS2WASM_RUNNER"])
tinyvm = os.environ.get("UJS2WASM_TINYVM") or ""

bad = 0
for name, want in sorted(expect.items()):
    src = corpus / (name + ".ujs")
    if not src.is_file():
        print("MISSING", src)
        bad += 1
        continue
    wasm = out / (name + ".wasm")
    r = subprocess.run(
        [sys.executable, "-m", "ujs", "ujs2wasm", str(src), "-o", str(wasm),
         "--mode", "direct"],
        capture_output=True, text=True, cwd=str(root),
    )
    if r.returncode != 0:
        print("EMIT FAIL", name, (r.stderr or r.stdout).strip())
        bad += 1
        continue
    if "direct" not in (r.stdout + r.stderr):
        print("NOT DIRECT", name, r.stdout.strip())
        bad += 1
        continue
    if wasm.read_bytes()[:4] != b"\0asm":
        print("BAD MAGIC", name)
        bad += 1
        continue
    if tinyvm:
        v = subprocess.run([tinyvm, "module", "validate", str(wasm)],
                           capture_output=True, text=True)
        if v.returncode != 0:
            print("TINYVM FAIL", name, (v.stderr or v.stdout).strip())
            bad += 1
            continue
    got = subprocess.check_output(
        ["node", str(runner), str(wasm)], text=True, cwd=str(root)).strip()
    got_v = json.loads(got)
    if got_v != want:
        print("MISMATCH", name, "got", got_v, "want", want)
        bad += 1
        continue
    print("ok", name, want, wasm.stat().st_size, "B")

print("%d/%d corpus" % (len(expect) - bad, len(expect)))
sys.exit(1 if bad else 0)
PY

echo "== ujs2wasm fold parity (jtape == direct wasm) =="
python3 - <<'PY'
import json, os, subprocess, sys, tempfile
from pathlib import Path
from ujs.construct.oracle import Oracle
from ujs.construct import api
from ujs.construct import value as V

root = Path(os.environ["UJS2WASM_ROOT"])
corpus = Path(os.environ["UJS2WASM_CORPUS"])
expect = json.loads(Path(os.environ["UJS2WASM_EXPECT"]).read_text())
runner = Path(os.environ["UJS2WASM_RUNNER"])
o = Oracle(drive="gold")
# compiler.wasm covers bare global assign; jtape rejects it as readonly.
SKIP_JTAPE = {"globals_fold"}
bad = 0
with tempfile.TemporaryDirectory() as td:
    for name, want in sorted(expect.items()):
        if name in SKIP_JTAPE:
            print("fold skip", name, "(jtape readonly globals)")
            continue
        src_path = corpus / (name + ".ujs")
        src = src_path.read_text()
        r = api.run(src, backend="jtape", oracle=o)
        if not r.ok:
            print("JTAPE FAIL", name, r.err)
            bad += 1
            continue
        jv = V.to_py(r.value)
        if jv != want:
            print("JTAPE MISMATCH", name, jv, want)
            bad += 1
            continue
        wasm = Path(td) / (name + ".wasm")
        subprocess.check_call(
            [sys.executable, "-m", "ujs", "ujs2wasm",
             str(src_path), "-o", str(wasm), "--mode", "direct"],
            cwd=str(root), stdout=subprocess.DEVNULL)
        got = json.loads(subprocess.check_output(
            ["node", str(runner), str(wasm)], text=True))
        if got != jv:
            print("FOLD FAIL", name, "jtape", jv, "wasm", got)
            bad += 1
            continue
        print("fold", name)
n = len(expect) - len(SKIP_JTAPE)
print("%d/%d fold" % (n - bad, n))
sys.exit(1 if bad else 0)
PY

echo "== ujs2wasm neg (must reject) =="
python3 - <<'PY'
import os, subprocess, sys
from pathlib import Path
root = Path(os.environ["UJS2WASM_ROOT"])
neg = Path(os.environ["UJS2WASM_NEG"])
bad = 0
for src in sorted(neg.glob("*.ujs")):
    r = subprocess.run(
        [sys.executable, "-m", "ujs", "ujs2wasm", str(src),
         "-o", "/tmp/ujs2wasm-neg-should-not.wasm", "--mode", "direct"],
        capture_output=True, text=True, cwd=str(root),
    )
    if r.returncode == 0:
        print("SHOULD REJECT", src.name)
        bad += 1
    else:
        print("reject ok", src.name)
sys.exit(1 if bad else 0)
PY

if [[ -n "$TINYVM" ]]; then
  echo "== tinyvm: $TINYVM (validate used on corpus) =="
else
  echo "== tinyvm: skip (not found; Node instantiate still gates) =="
fi

echo "== ujs2wasm game-ready (sim/drone can_emit_direct) =="
python3 - <<'PY'
import sys
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.oracle import Oracle
from ujs.construct.emit_wasm import can_emit_direct

games = [
    Path("ujs/web/game/sim.ujs"),
    Path("ujs/web/game/drone.ujs"),
]
o = Oracle(drive="gold")
bad = 0
for p in games:
    if not p.is_file():
        print("MISSING", p)
        bad += 1
        continue
    fn = compile_src(p.read_text())
    ok, why = can_emit_direct(fn, o)
    if not ok:
        print("NOT READY", p, why)
        bad += 1
    else:
        print("ready", p)
sys.exit(1 if bad else 0)
PY

echo "== ujs2wasm step (sim host_* fold) =="
./tests/ujs2wasm_step.sh

echo "== ujs2wasm compiler (M2) =="
./tests/ujs2wasm_compiler.sh

echo "ujs2wasm suite OK"
