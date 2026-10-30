#!/usr/bin/env bash
# Pack UJS release artifacts for manual GitHub Release upload.
# Does NOT call `gh` / publish. Re-runnable; fails if suite or fingerprint fails.
#
# Usage (from repo root):
#   ./ujs/scripts/release-artifacts.sh           # core only
#   ./ujs/scripts/release-artifacts.sh --with-demos
#   ./ujs/scripts/release-artifacts.sh --skip-tests   # only if you already ran ujs.sh
#
# Output: dist/ujs-<version>-artifacts.zip (+ .sha256)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

WITH_DEMOS=0
SKIP_TESTS=0
for a in "$@"; do
  case "$a" in
    --with-demos) WITH_DEMOS=1 ;;
    --skip-tests) SKIP_TESTS=1 ;;
    -h|--help)
      sed -n '2,12p' "$0" | sed 's/^# //'
      exit 0
      ;;
    *) echo "unknown arg: $a" >&2; exit 2 ;;
  esac
done

CHECK="$ROOT/ujs/scripts/release-check.mjs"   # node helpers; this script runs no Python
VER=$(node "$CHECK" version ujs/package.json)
OUT_DIR="$ROOT/dist"
# the stage and the zip are built in a private scratch under the temp root and only the finished zip is renamed into
# dist/: dist/ lies outside out/, where the deletion guard (tests/safedel.cx, commissar ruling 2026-10-23) removes
# nothing, so nothing in dist/ is ever deleted -- a rename replaces the previous zip
SCRATCH=$(mktemp -d "${TMPDIR:-/tmp}/ujs-release.XXXXXX")
trap 'sh "$ROOT/tests/cxbin" "$ROOT/tests/safedel.cx" --purge --force-count 20000 -- "${SCRATCH:?}"' EXIT
STAGE="$SCRATCH/ujs-$VER-stage"
ZIP="$OUT_DIR/ujs-$VER-artifacts.zip"
CORE="$ROOT/ujs/core"
UJS="$ROOT/ujs"

echo "== ujs release-artifacts v$VER =="

if [[ "$SKIP_TESTS" -eq 0 ]]; then
  echo "== tests/ujs.sh =="
  ./tests/ujs.sh
else
  echo "== skip tests (requested) =="
  # Prefer committed core/ fingerprint; FORCE_WEB_BUILD=1 to rebuild A-kernel
  if [[ "${FORCE_WEB_BUILD:-0}" == "1" ]]; then
    # `python3 -m ujs web-build` was retired with the ujs Python (archive/ujs-legacy-py-c/)
    echo "FORCE_WEB_BUILD: the Python web-build is archived; rebuild core/ with the node tooling first" >&2
    exit 1
  else
    echo "== verify core/ (no web-build unless FORCE_WEB_BUILD=1) =="
  fi
fi

test -f "$CORE/BUILD.json"
test -f "$CORE/ujs_full.wasm"
test -f "$CORE/compiler.gen.js"

node "$CHECK" fingerprint "$CORE" ujs/package.json

mkdir -p "$STAGE/core" "$OUT_DIR"

for f in index.js wasm_run.js compiler.js jspi.js README.md BUILD.json \
         ujs_full.wasm compiler.gen.js ujs_rt.wasm; do
  [[ -f "$CORE/$f" ]] && cp "$CORE/$f" "$STAGE/core/"
done
cp "$ROOT/ujs/package.json" "$STAGE/"
cp "$ROOT/ujs/README.md" "$STAGE/"

if [[ "$WITH_DEMOS" -eq 1 ]]; then
  mkdir -p "$STAGE/web" "$STAGE/uxe/demo"
  for f in index.html demo.js style.css README.md; do
    [[ -f "$UJS/web/$f" ]] && cp "$UJS/web/$f" "$STAGE/web/"
  done
  if [[ -d "$UJS/web/progs" ]]; then
    mkdir -p "$STAGE/web/progs"
    cp "$UJS/web/progs/"*.wasm "$STAGE/web/progs/" 2>/dev/null || true
  fi
  for f in README.md HOST_ABI.md host-abi.js host-browser.js packet.js input.js \
           app-asteroid.js app-drone.js math.js scene.js meshes.js \
           renderer-webgl.js renderer-webgpu.js; do
    [[ -f "$UJS/uxe/$f" ]] && cp "$UJS/uxe/$f" "$STAGE/uxe/"
  done
  for f in index.html host.js; do
    [[ -f "$UJS/uxe/demo/$f" ]] && cp "$UJS/uxe/demo/$f" "$STAGE/uxe/demo/"
  done
fi

node "$CHECK" note "$STAGE"

( cd "$SCRATCH" && zip -r -q "$(basename "$ZIP")" "$(basename "$STAGE")" )
mv -f "$SCRATCH/$(basename "$ZIP")" "$ZIP"
# flatten: zip contains ujs-VER-stage/... — rename inner for nicer extract
# Actually keep stage name clear. Also write checksum of zip.
(
  cd "$OUT_DIR"
  shasum -a 256 "$(basename "$ZIP")" > "$(basename "$ZIP").sha256"
)

echo "wrote $ZIP"
echo "wrote $ZIP.sha256"
ls -la "$ZIP" "$ZIP.sha256"
cat "$ZIP.sha256"
echo "Next: create a GitHub Release for v$VER and upload the zip (manual)."
