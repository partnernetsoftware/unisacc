#!/usr/bin/env bash
# Ship deliverable:
#   index.html       — static page + tiny Host glue (inlined)
#   asteroid.wasm    — {gameName}.wasm（玩法核，不含引擎）
#   gameEngine.wasm  — UJS VM（= ujs_full.wasm 交付名）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WEB="$ROOT/ujs/web"
SHIP="$WEB/engine/ship"
HOST_JS="$SHIP/uxe-host.js"

if [[ ! -f "$WEB/compiler.gen.js" || ! -f "$WEB/ujs_full.wasm" ]]; then
  echo "need web-build first: python3 -m ujs web-build" >&2
  exit 1
fi

cd "$ROOT/ujs"

# 1) {game}.wasm only
perl -e 'alarm 120; exec @ARGV' node "$SHIP/build-asteroid.mjs"

# 2) gameEngine.wasm = rename/copy of ujs_full.wasm
cp -f "$WEB/ujs_full.wasm" "$SHIP/gameEngine.wasm"

# 3) Host glue only (bridges the two wasms + GPU)
perl -e 'alarm 120; exec @ARGV' npx --yes esbuild@0.23.1 \
  "$SHIP/host-entry.js" \
  --bundle \
  --format=esm \
  --platform=browser \
  --target=es2022 \
  --loader:.json=json \
  --outfile="$HOST_JS"

if grep -q "compiler.gen\|runAsteroidCore\|bootRuntime" "$HOST_JS"; then
  echo "host glue too fat — abort" >&2
  exit 1
fi

# 4) Bake HTML
perl -e 'alarm 30; exec @ARGV' node "$SHIP/bake-html.mjs"

echo "ship ok:"
echo "  asteroid.wasm    $(wc -c < "$SHIP/asteroid.wasm" | tr -d ' ')B"
echo "  gameEngine.wasm  $(wc -c < "$SHIP/gameEngine.wasm" | tr -d ' ')B"
echo "  host(in html)    $(wc -c < "$HOST_JS" | tr -d ' ')B"
echo "open: http://127.0.0.1:8765/engine/ship/"
