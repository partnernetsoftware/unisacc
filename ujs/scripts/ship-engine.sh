#!/usr/bin/env bash
# Ship asteroid + shared docs/uxe/engine.js
# Full Pages: npm run ship:pages
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WEB="$ROOT/ujs/web"
SHIP="$WEB/engine/ship"
DOCS="$ROOT/docs"

if [[ ! -f "$WEB/compiler.gen.js" || ! -f "$WEB/ujs_full.wasm" ]]; then
  echo "need web-build first: python3 -m ujs web-build" >&2
  exit 1
fi

cd "$ROOT/ujs"

STAMP="${UXE_SHIP_STAMP:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || date +%Y%m%d)}"
export UXE_SHIP_STAMP="$STAMP"

perl -e 'alarm 60; exec @ARGV' node "$SHIP/build-engine-js.mjs"

perl -e 'alarm 120; exec @ARGV' node "$SHIP/build-asteroid.mjs"
cp -f "$WEB/ujs_full.wasm" "$SHIP/engine.wasm"

# local ship/: ./engine.js next to game.js
perl -e 'alarm 60; exec @ARGV' env UXE_SHIP_HOME="../" UXE_SHIP_STAMP="$STAMP" node "$SHIP/bake-html.mjs"

PAGES_AST="$DOCS/uxe/asteroid"
mkdir -p "$PAGES_AST"
# Pages HTML: nav up to docs/
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../../" UXE_SHIP_STAMP="$STAMP" UXE_SKIP_GAME_BUNDLE=1 \
  node "$SHIP/bake-html.mjs"
cp -f "$SHIP/index.html" "$SHIP/asteroid.wasm" "$SHIP/engine.wasm" "$PAGES_AST/"
# game.js on Pages imports shared ../engine.js
sed 's|from "./engine.js"|from "../engine.js"|g; s|from '\''./engine.js'\''|from '\''../engine.js'\''|g' \
  "$SHIP/game.js" > "$PAGES_AST/game.js"
# restore local ship HTML (home ../)
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../" UXE_SHIP_STAMP="$STAMP" UXE_SKIP_GAME_BUNDLE=1 \
  node "$SHIP/bake-html.mjs"

rm -rf "$DOCS/uxe/monopoly"
touch "$DOCS/.nojekyll"

echo "ship ok:"
echo "  engine.js $DOCS/uxe/engine.js"
echo "  asteroid  $PAGES_AST"
echo "local: http://127.0.0.1:8765/engine/ship/"
