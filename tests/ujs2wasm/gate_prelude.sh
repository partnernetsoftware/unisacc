# Shared early checks for ujs2wasm_compiler.sh (product gate).
# Sourced by the gate and by isolation probes — one implementation.
#
# Expects: ROOT already set to repo root (absolute).
# Sets: COMPILER_WASM (relative path under ROOT), TINYVM (absolute or empty)
# Exits 1 on missing required artifacts (never exit 0 for skip).

ujs_gate_find_compiler() {
  COMPILER_WASM=""
  local cand
  for cand in ujs/core/compiler.wasm ujs/compiler.wasm ujs/uxe/ship/compiler.wasm \
              ujs/iterate/compiler.wasm; do
    if [ -f "$ROOT/$cand" ]; then
      COMPILER_WASM="$cand"
      return 0
    fi
  done
  return 1
}

ujs_gate_find_tinyvm() {
  TINYVM=""
  if command -v tinyvm >/dev/null 2>&1; then
    TINYVM="$(command -v tinyvm)"
    return 0
  fi
  local c
  for c in \
    "$ROOT/../tinyvm/target/release/tinyvm" \
    "$ROOT/../tinyvm/target/debug/tinyvm" \
    "$HOME/repos/tinyvm/target/release/tinyvm" \
    "$HOME/repos/tinyvm/target/debug/tinyvm"
  do
    if [[ -x "$c" ]]; then
      TINYVM="$c"
      return 0
    fi
  done
  return 1
}

# Resolve compiler + optional tinyvm policy. Call from gate after ROOT is set.
ujs_gate_prelude() {
  if ! ujs_gate_find_compiler; then
    echo "FAIL: no compiler.wasm (M2/M3 product artifact required)"
    echo "build: ./ujs/seed/stage0/build-compiler-wasm.sh"
    exit 1
  fi
  ujs_gate_find_tinyvm || true
  if [[ -z "${TINYVM:-}" && "${UJS_REQUIRE_TINYVM:-}" == "1" ]]; then
    echo "FAIL: UJS_REQUIRE_TINYVM=1 but tinyvm not found"
    exit 1
  fi
}
