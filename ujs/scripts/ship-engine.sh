#!/usr/bin/env bash
# Bundle UXE demo into one ESM file for publish. Dev keeps split sources.
# Usage (repo root or ujs/): ./ujs/scripts/ship-engine.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WEB="$ROOT/ujs/web"
ENG="$WEB/engine"
OUT="$ENG/ship/uxe-demo.js"

if [[ ! -f "$WEB/compiler.gen.js" || ! -f "$WEB/ujs_full.wasm" ]]; then
  echo "need web-build first: python3 -m ujs web-build" >&2
  exit 1
fi

cd "$ROOT/ujs"
perl -e 'alarm 120; exec @ARGV' npx --yes esbuild@0.23.1 \
  "$ENG/ship/entry.js" \
  --bundle \
  --format=esm \
  --platform=browser \
  --target=es2022 \
  --external:fs \
  --external:path \
  --external:url \
  --outfile="$OUT"

BYTES=$(wc -c < "$OUT" | tr -d ' ')
echo "wrote $OUT ($BYTES bytes)"
echo "open: http://127.0.0.1:8765/engine/ship/"
