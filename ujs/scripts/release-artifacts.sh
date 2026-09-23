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

VER=$(python3 -c 'import json; print(json.load(open("ujs/package.json"))["version"])')
OUT_DIR="$ROOT/dist"
STAGE="$OUT_DIR/ujs-$VER-stage"
ZIP="$OUT_DIR/ujs-$VER-artifacts.zip"
CORE="$ROOT/ujs/core"
UJS="$ROOT/ujs"

echo "== ujs release-artifacts v$VER =="

if [[ "$SKIP_TESTS" -eq 0 ]]; then
  echo "== tests/ujs.sh =="
  ./tests/ujs.sh
else
  echo "== skip tests (requested) =="
  python3 -m ujs web-build
fi

test -f "$CORE/BUILD.json"
test -f "$CORE/ujs_full.wasm"
test -f "$CORE/compiler.gen.js"

python3 - <<PY
import json, hashlib, pathlib, sys
core = pathlib.Path("$CORE")
b = json.loads((core / "BUILD.json").read_text())
pkg = json.loads(pathlib.Path("ujs/package.json").read_text())
assert b["version"] == pkg["version"], (b["version"], pkg["version"])
for name, meta in b["artifacts"].items():
    p = core / name
    if not p.is_file():
        print("missing", p, file=sys.stderr); sys.exit(1)
    h = hashlib.sha256(p.read_bytes()).hexdigest()
    if h != meta["sha256"]:
        print("sha mismatch", name, h, meta["sha256"], file=sys.stderr); sys.exit(1)
    if p.stat().st_size != meta["bytes"]:
        print("size mismatch", name, file=sys.stderr); sys.exit(1)
print("fingerprint OK", b["version"], b.get("git"))
PY

rm -rf "$STAGE"
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

python3 - <<PY
import json, pathlib
stage = pathlib.Path("$STAGE")
b = json.loads((stage / "core" / "BUILD.json").read_text())
note = {
  "how": "Upload this zip as a GitHub Release asset for tag/version matching package.json.",
  "verify": "sha256 of core/ujs_full.wasm and core/compiler.gen.js must match core/BUILD.json",
  "boot": "import { bootRuntime, wasm_run } from './core/wasm_run.js'; await bootRuntime(new URL('./core/ujs_full.wasm', import.meta.url));",
  "build": b,
}
(stage / "RELEASE.txt").write_text(
  "UJS %(version)s artifacts\\n"
  "git=%(git)s created=%(created)s\\n"
  "Core: core/ujs_full.wasm + core/compiler.gen.js (+ hand-written ESM).\\n"
  "Do not commit this zip to git; attach to the GitHub Release.\\n" % b
  + "\\n" + json.dumps(note, indent=2) + "\\n"
)
PY

rm -f "$ZIP"
( cd "$OUT_DIR" && zip -r -q "$(basename "$ZIP")" "$(basename "$STAGE")" )
# flatten: zip contains ujs-VER-stage/... — rename inner for nicer extract
# Actually keep stage name clear. Also write checksum of zip.
(
  cd "$OUT_DIR"
  shasum -a 256 "$(basename "$ZIP")" > "$(basename "$ZIP").sha256"
)

rm -rf "$STAGE"
echo "wrote $ZIP"
echo "wrote $ZIP.sha256"
ls -la "$ZIP" "$ZIP.sha256"
cat "$ZIP.sha256"
echo "Next: create a GitHub Release for v$VER and upload the zip (manual)."
