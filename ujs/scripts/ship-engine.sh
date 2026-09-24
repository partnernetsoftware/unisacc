#!/usr/bin/env bash
# Ship asteroid (ship-js) + shared docs/uxe/engine.js
# Full Pages: npm run ship:pages
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UJS="$ROOT/ujs"
CORE="$UJS/core"
DOCS="$ROOT/docs"

if [[ ! -f "$CORE/compiler.gen.js" || ! -f "$CORE/ujs_full.wasm" ]]; then
  echo "need web-build first: python3 -m ujs web-build" >&2
  exit 1
fi

cd "$UJS"

STAMP="${UXE_SHIP_STAMP:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || date +%Y%m%d)}"
export UXE_SHIP_STAMP="$STAMP"

perl -e 'alarm 180; exec @ARGV' node "$UJS/uxe/ship/build-asteroid-pages.mjs"

rm -rf "$DOCS/uxe/monopoly"
touch "$DOCS/.nojekyll"

# contract checks (ship-js, no C eng glue)
GAME_JS="$UJS/uxe/ship/game.js"
if grep -E 'eng_sim_step|eng_boot' "$GAME_JS" >/dev/null; then
  echo "FAIL: ship/game.js still contains eng_*" >&2
  exit 1
fi
if [[ -f "$DOCS/uxe/asteroid/asteroid.wasm" ]]; then
  echo "FAIL: pages still shipping asteroid.wasm (C path)" >&2
  exit 1
fi

echo "ship ok:"
echo "  engine.js $DOCS/uxe/engine.js"
echo "  asteroid  $DOCS/uxe/asteroid (ship-js)"
echo "local: http://127.0.0.1:8765/uxe/ship/"
