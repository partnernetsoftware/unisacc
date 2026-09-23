#!/usr/bin/env bash
# Ship asteroid deliverables + GitHub Pages mirror under docs/uxe/asteroid/
# Full Pages (asteroid + drone): npm run ship:pages
# Monopoly is archived — this script deletes docs/uxe/monopoly if present.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WEB="$ROOT/ujs/web"
SHIP="$WEB/engine/ship"
HOST_JS="$SHIP/uxe-host.js"
DOCS="$ROOT/docs"

if [[ ! -f "$WEB/compiler.gen.js" || ! -f "$WEB/ujs_full.wasm" ]]; then
  echo "need web-build first: python3 -m ujs web-build" >&2
  exit 1
fi

cd "$ROOT/ujs"

# Stamp Pages HTML so browsers refetch wasm after ship
STAMP="${UXE_SHIP_STAMP:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || date +%Y%m%d)}"
export UXE_SHIP_STAMP="$STAMP"

# ——— Asteroid (C {game}.wasm) ———
perl -e 'alarm 120; exec @ARGV' node "$SHIP/build-asteroid.mjs"
cp -f "$WEB/ujs_full.wasm" "$SHIP/engine.wasm"

perl -e 'alarm 120; exec @ARGV' npx --yes esbuild@0.23.1 \
  "$SHIP/host-entry.js" \
  --bundle --format=esm --platform=browser --target=es2022 \
  --loader:.json=json \
  --outfile="$HOST_JS"

if grep -q "compiler.gen\|runAsteroidCore\|bootRuntime" "$HOST_JS"; then
  echo "asteroid host glue too fat — abort" >&2
  exit 1
fi

perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../" UXE_SHIP_STAMP="$STAMP" node "$SHIP/bake-html.mjs"

PAGES_AST="$DOCS/uxe/asteroid"
mkdir -p "$PAGES_AST"
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../../" UXE_SHIP_STAMP="$STAMP" node "$SHIP/bake-html.mjs"
cp -f "$SHIP/index.html" "$SHIP/asteroid.wasm" "$SHIP/engine.wasm" "$PAGES_AST/"
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../" UXE_SHIP_STAMP="$STAMP" node "$SHIP/bake-html.mjs"

# ——— Monopoly：归档（见 web/engine/archive/）———
# 源码仍留在 demo/monopoly · ship/monopoly；正式 Pages 不镜像。
rm -rf "$DOCS/uxe/monopoly"

# ——— Game index（手写；确保 .nojekyll） ———
touch "$DOCS/.nojekyll"

echo "ship ok:"
echo "  asteroid  $PAGES_AST"
echo "  index     $DOCS/index.html"
echo "  monopoly  archived — use ship:pages for drone too"
echo "local asteroid: http://127.0.0.1:8765/engine/ship/"
echo "pages root:     docs/ → GitHub Pages"
