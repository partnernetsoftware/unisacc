#!/bin/bash
# ship-js contract — no C game wasm / no eng_* glue on Pages faces.
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

# game-ready emit still required for path-B next step
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
