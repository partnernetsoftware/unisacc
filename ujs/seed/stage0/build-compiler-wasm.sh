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
R="$(cd "$UJS/.." && pwd)"
# a fresh private zig cache per build (was a fixed stage0/.zig-cache-compiler, deleted first); the artifact is built
# beside it and moved over OUT only when it is wasm.  Deletion goes through the one guard (tests/safedel.cx); the
# cache is zig's object tree, so a site ceiling.
CACHE="$(mktemp -d "${TMPDIR:-/tmp}/ujs-zig-cache.XXXXXX")"
trap '[ -z "$CACHE" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge --max-files 20000 -- "$CACHE"' EXIT
NEW="$CACHE/compiler.wasm"
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
  -o "$NEW" \
  "$SRC"
if [ ! -f "$NEW" ]; then
  echo "FAIL: no $NEW"
  exit 1
fi
magic=$(head -c 4 "$NEW" | od -An -tx1 | tr -d ' \n')
[ "$magic" = "0061736d" ] || { echo "FAIL: not wasm ($magic)"; exit 1; }
mv -f "$NEW" "$OUT"
echo "wrote $OUT ($(wc -c < "$OUT") bytes)"
