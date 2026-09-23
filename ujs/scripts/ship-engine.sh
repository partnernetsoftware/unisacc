#!/usr/bin/env bash
# Ship deliverables + GitHub Pages mirror under docs/
#   asteroid: index.html + asteroid.wasm + gameEngine.wasm
#   monopoly: index.html + gameEngine.wasm（JS 核 + 预编译 sim；C monopoly.wasm 后补）
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

# ——— Asteroid (C {game}.wasm) ———
perl -e 'alarm 120; exec @ARGV' node "$SHIP/build-asteroid.mjs"
cp -f "$WEB/ujs_full.wasm" "$SHIP/gameEngine.wasm"

perl -e 'alarm 120; exec @ARGV' npx --yes esbuild@0.23.1 \
  "$SHIP/host-entry.js" \
  --bundle --format=esm --platform=browser --target=es2022 \
  --loader:.json=json \
  --outfile="$HOST_JS"

if grep -q "compiler.gen\|runAsteroidCore\|bootRuntime" "$HOST_JS"; then
  echo "asteroid host glue too fat — abort" >&2
  exit 1
fi

perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../" node "$SHIP/bake-html.mjs"

PAGES_AST="$DOCS/uxe/asteroid"
mkdir -p "$PAGES_AST"
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../../" node "$SHIP/bake-html.mjs"
cp -f "$SHIP/index.html" "$SHIP/asteroid.wasm" "$SHIP/gameEngine.wasm" "$PAGES_AST/"
perl -e 'alarm 30; exec @ARGV' env UXE_SHIP_HOME="../" node "$SHIP/bake-html.mjs"

# ——— Monopoly：归档，不进 Pages 索引 / 不镜像 docs ———
# 源码仍留在 demo/monopoly · ship/monopoly（本地可开）；Pages 只公开 asteroid。
rm -rf "$DOCS/uxe/monopoly"

# ——— Game index（已手写；确保 .nojekyll） ———
touch "$DOCS/.nojekyll"

echo "ship ok:"
echo "  asteroid  $PAGES_AST"
echo "  index     $DOCS/index.html"
echo "  monopoly  archived (not on Pages)"
echo "local asteroid: http://127.0.0.1:8765/engine/ship/"
echo "pages root:     docs/ → GitHub Pages"
