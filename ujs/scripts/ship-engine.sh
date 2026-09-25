#!/usr/bin/env bash
# Ship asteroid (ship-js) + shared docs/uxe/engine.js
# Full Pages: npm run ship:pages
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UJS="$ROOT/ujs"
CORE="$UJS/core"
DOCS="$ROOT/docs"

# Path B ship: compiler_core.wasm (compiler.ujs) — no web-build / ujs_full
if [[ ! -f "$CORE/compiler_core.wasm" || ! -f "$CORE/compiler_core.meta.json" ]]; then
  echo "need ujs/core/compiler_core.wasm (node ujs/compile.mjs ujs/core/compiler.ujs -o ujs/core/compiler_core.wasm)" >&2
  exit 1
fi

cd "$UJS"

STAMP="${UXE_SHIP_STAMP:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || date +%Y%m%d)}"
export UXE_SHIP_STAMP="$STAMP"

perl -e 'alarm 180; exec @ARGV' node "$UJS/uxe/ship/build-asteroid-pages.mjs"

rm -rf "$DOCS/uxe/monopoly"
touch "$DOCS/.nojekyll"

# contract checks (ship-js, no C eng glue; Asteroid path B)
GAME_JS="$UJS/uxe/ship/game.js"
if grep -E 'eng_sim_step|eng_boot' "$GAME_JS" >/dev/null; then
  echo "FAIL: ship/game.js still contains eng_*" >&2
  exit 1
fi
if [[ ! -f "$UJS/uxe/ship/sim.wasm" || ! -f "$DOCS/uxe/asteroid/sim.wasm" ]]; then
  echo "FAIL: Asteroid path B requires sim.wasm next to game" >&2
  exit 1
fi
if [[ -f "$DOCS/uxe/asteroid/asteroid.wasm" ]]; then
  echo "FAIL: pages still shipping asteroid.wasm (C path)" >&2
  exit 1
fi
if [[ -f "$DOCS/uxe/asteroid/engine.wasm" || -f "$UJS/uxe/ship/engine.wasm" ]]; then
  echo "FAIL: path B ship must not ship engine.wasm (A-core / web-build)" >&2
  exit 1
fi

echo "ship ok:"
echo "  engine.js $DOCS/uxe/engine.js"
echo "  asteroid  $DOCS/uxe/asteroid (ship-js path B, no A-core)"
echo "local: http://127.0.0.1:8765/uxe/ship/"
