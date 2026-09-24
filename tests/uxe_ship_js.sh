#!/bin/bash
# ship-js contract — no C game wasm / no eng_* glue on Pages faces.
# Asteroid + drone Pages default path B: sim.wasm + direct_step in game.js.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
bad=0

check_game() {
  local js="$1" label="$2"
  if [[ ! -f "$js" ]]; then
    echo "MISSING $label $js"; bad=$((bad+1)); return
  fi
  if grep -E 'eng_sim_step|eng_boot' "$js" >/dev/null; then
    echo "FAIL $label has eng_*"; bad=$((bad+1))
  else
    echo "ok $label no eng_*"
  fi
  if grep -E 'compiler\.gen\.js' "$js" >/dev/null; then
    echo "FAIL $label glued compiler.gen"; bad=$((bad+1))
  else
    echo "ok $label no compiler.gen"
  fi
}

check_game "$ROOT/ujs/uxe/ship/game.js" "asteroid-local"
check_game "$ROOT/docs/uxe/asteroid/game.js" "asteroid-pages"
check_game "$ROOT/ujs/uxe/ship/drone/game.js" "drone-local"
check_game "$ROOT/docs/uxe/drone/game.js" "drone-pages"

for wasm in \
  "$ROOT/ujs/uxe/ship/asteroid.wasm" \
  "$ROOT/docs/uxe/asteroid/asteroid.wasm"
do
  if [[ -f "$wasm" ]]; then
    echo "FAIL legacy C artifact still present: $wasm"
    bad=$((bad+1))
  fi
done
echo "no legacy asteroid.wasm ok"

# Path B: sim.wasm next to game + entry wires directSim
check_sim_wasm() {
  local wasm="$1" label="$2"
  if [[ ! -f "$wasm" ]]; then
    echo "FAIL $label missing sim.wasm ($wasm)"; bad=$((bad+1)); return
  fi
  if ! python3 -c 'import sys; d=open(sys.argv[1],"rb").read(4); sys.exit(0 if d==b"\0asm" else 1)' "$wasm"; then
    echo "FAIL $label sim.wasm not \\0asm"; bad=$((bad+1))
  else
    echo "ok $label sim.wasm \\0asm ($(wc -c < "$wasm" | tr -d ' ') B)"
  fi
}
check_sim_wasm "$ROOT/ujs/uxe/ship/sim.wasm" "asteroid-local"
check_sim_wasm "$ROOT/docs/uxe/asteroid/sim.wasm" "asteroid-pages"
check_sim_wasm "$ROOT/ujs/uxe/ship/drone/sim.wasm" "drone-local"
check_sim_wasm "$ROOT/docs/uxe/drone/sim.wasm" "drone-pages"

check_path_b() {
  local js="$1" label="$2"
  if [[ ! -f "$js" ]]; then
    echo "FAIL $label missing game.js"; bad=$((bad+1)); return
  fi
  if ! grep -E 'bootDirectStep|host_set_global' "$js" >/dev/null; then
    echo "FAIL $label missing path-B direct_step surface"; bad=$((bad+1))
  else
    echo "ok $label path-B direct_step"
  fi
  # must not step solely via bytecode image embed (path A hot path)
  if grep -E 'precompiled:\s*\{\s*image' "$js" >/dev/null; then
    echo "FAIL $label still wires precompiled image (path A)"; bad=$((bad+1))
  else
    echo "ok $label no precompiled image wire"
  fi
  if ! grep -E 'directSim' "$js" >/dev/null; then
    echo "FAIL $label missing directSim pass"; bad=$((bad+1))
  else
    echo "ok $label passes directSim"
  fi
}

check_path_b "$ROOT/ujs/uxe/ship/game.js" "asteroid-local"
check_path_b "$ROOT/docs/uxe/asteroid/game.js" "asteroid-pages"
check_path_b "$ROOT/ujs/uxe/ship/drone/game.js" "drone-local"
check_path_b "$ROOT/docs/uxe/drone/game.js" "drone-pages"

# legacy path-A drone embed must be gone
if [[ -f "$ROOT/ujs/uxe/ship/drone.embed.json" ]]; then
  echo "FAIL legacy drone.embed.json still present"; bad=$((bad+1))
else
  echo "ok no drone.embed.json"
fi

# game-ready emit still required for path-B
python3 - <<'PY'
import sys
from pathlib import Path
from ujs.construct.front.compile import compile_src
from ujs.construct.oracle import Oracle
from ujs.construct.emit_wasm import can_emit_direct
o = Oracle(drive="gold")
bad = 0
for p in [Path("ujs/web/game/sim.ujs"), Path("ujs/web/game/drone.ujs")]:
    ok, why = can_emit_direct(compile_src(p.read_text()), o)
    print(("ready" if ok else "NOT READY"), p, "" if ok else why)
    bad += 0 if ok else 1
sys.exit(1 if bad else 0)
PY

if [[ $bad -ne 0 ]]; then
  echo "ship-js contract FAIL ($bad)"
  exit 1
fi
echo "ship-js contract OK"
