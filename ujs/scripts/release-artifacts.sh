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
WEB="$ROOT/ujs/web"

echo "== ujs release-artifacts v$VER =="

if [[ "$SKIP_TESTS" -eq 0 ]]; then
  echo "== tests/ujs.sh =="
  ./tests/ujs.sh
else
  echo "== skip tests (requested) =="
  python3 -m ujs web-build
fi

test -f "$WEB/BUILD.json"
test -f "$WEB/ujs_full.wasm"
test -f "$WEB/compiler.gen.js"

python3 - <<PY
import json, hashlib, pathlib, sys
web = pathlib.Path("$WEB")
b = json.loads((web / "BUILD.json").read_text())
pkg = json.loads(pathlib.Path("ujs/package.json").read_text())
assert b["version"] == pkg["version"], (b["version"], pkg["version"])
for name, meta in b["artifacts"].items():
    p = web / name
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
mkdir -p "$STAGE/web" "$OUT_DIR"

# hand-written product surface (needed to boot the prebuilt wasm)
for f in wasm_run.js compiler.js index.html demo.js style.css jspi.js README.md; do
  [[ -f "$WEB/$f" ]] && cp "$WEB/$f" "$STAGE/web/"
done
cp "$ROOT/ujs/package.json" "$STAGE/"
cp "$ROOT/ujs/README.md" "$STAGE/"
cp "$WEB/BUILD.json" "$STAGE/web/"

# core binaries (Release payload)
cp "$WEB/ujs_full.wasm" "$STAGE/web/"
cp "$WEB/compiler.gen.js" "$STAGE/web/"
[[ -f "$WEB/ujs_rt.wasm" ]] && cp "$WEB/ujs_rt.wasm" "$STAGE/web/"

if [[ "$WITH_DEMOS" -eq 1 ]]; then
  mkdir -p "$STAGE/web/progs"
  cp "$WEB/progs/"*.wasm "$STAGE/web/progs/" 2>/dev/null || true
  [[ -f "$WEB/full_demos.json" ]] && cp "$WEB/full_demos.json" "$STAGE/web/"
  [[ -f "$WEB/demos.json" ]] && cp "$WEB/demos.json" "$STAGE/web/"
  # game demo sources (no vendored three.module.js)
  if [[ -d "$WEB/game" ]]; then
    mkdir -p "$STAGE/web/game/exp"
    for f in README.md index.html host.js sim.ujs; do
      [[ -f "$WEB/game/$f" ]] && cp "$WEB/game/$f" "$STAGE/web/game/"
    done
    for f in README.md index.html host.js rawgl.js; do
      [[ -f "$WEB/game/exp/$f" ]] && cp "$WEB/game/exp/$f" "$STAGE/web/game/exp/"
    done
  fi
  # UXE engine product path (Host ABI + demo)
  if [[ -d "$WEB/engine" ]]; then
    mkdir -p "$STAGE/web/engine/demo"
    for f in README.md HOST_ABI.md host-abi.js browser-host.js packet.js input.js \
             core-asteroid.js math.js scene.js uxe.js \
             renderer-webgl.js renderer-webgpu.js; do
      [[ -f "$WEB/engine/$f" ]] && cp "$WEB/engine/$f" "$STAGE/web/engine/"
    done
    for f in index.html host.js; do
      [[ -f "$WEB/engine/demo/$f" ]] && cp "$WEB/engine/demo/$f" "$STAGE/web/engine/demo/"
    done
  fi
fi

python3 - <<PY
import json, pathlib
stage = pathlib.Path("$STAGE")
b = json.loads((stage / "web" / "BUILD.json").read_text())
note = {
  "how": "Upload this zip as a GitHub Release asset for tag/version matching package.json.",
  "verify": "sha256 of web/ujs_full.wasm and web/compiler.gen.js must match web/BUILD.json",
  "boot": "import { bootRuntime, wasm_run } from './web/wasm_run.js'; await bootRuntime(new URL('./web/ujs_full.wasm', import.meta.url));",
  "build": b,
}
(stage / "RELEASE.txt").write_text(
  "UJS %(version)s artifacts\\n"
  "git=%(git)s created=%(created)s\\n"
  "Core: web/ujs_full.wasm + web/compiler.gen.js (+ hand-written ESM).\\n"
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
