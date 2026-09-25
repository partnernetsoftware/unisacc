#!/usr/bin/env bash
# Seed stage0: C subset compiler → ujs/iterate/compiler.wasm (+ practice/core symlink)
# Product path loads the artifact; this script may use zig (not runtime).
set -euo pipefail
STAGE0="$(cd "$(dirname "$0")" && pwd)"
UJS="$(cd "$STAGE0/../.." && pwd)"
SRC="$STAGE0/compiler_min.c"
OUT="$UJS/iterate/compiler.wasm"
command -v zig >/dev/null || { echo "needs zig"; exit 1; }
mkdir -p "$(dirname "$OUT")"
CACHE="$STAGE0/.zig-cache-compiler"
rm -rf "$CACHE" "$OUT"
export ZIG_LOCAL_CACHE_DIR="$CACHE"
perl -e 'alarm 120; exec @ARGV' zig cc \
  -target wasm32-freestanding -O2 \
  -nostdlib \
  -Wl,--no-entry \
  -Wl,--export-memory \
  -Wl,--export=alloc \
  -Wl,--export=compile \
  -Wl,--export=out_ptr \
  -Wl,--export=out_len \
  -Wl,--export=meta_ptr \
  -Wl,--export=meta_len \
  -Wl,--export=err_ptr \
  -Wl,--export=err_len \
  -o "$OUT" \
  "$SRC"
if [ ! -f "$OUT" ]; then
  echo "FAIL: no $OUT"
  exit 1
fi
magic=$(head -c 4 "$OUT" | od -An -tx1 | tr -d ' \n')
[ "$magic" = "0061736d" ] || { echo "FAIL: not wasm ($magic)"; exit 1; }
echo "wrote $OUT ($(wc -c < "$OUT") bytes)"
