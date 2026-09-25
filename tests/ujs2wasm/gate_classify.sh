# Classify compile CLI outcomes for short-str negative tests.
# Only "parse_reject" is a greened reject. Sourced by the product gate.
#
# Exact allowlisted diagnostics (from compile.mjs / stage0):
#   compile: bad primary
#   compile: compiler.ujs core compile failed

classify_compile_outcome() {
  local ec="$1" err="$2" wasm="$3"
  if [ "$ec" -eq 0 ]; then echo accept; return; fi
  if [ "$ec" -ge 128 ]; then echo timeout_or_signal; return; fi
  if [ -f "$wasm" ]; then
    local magic
    magic=$(head -c 4 "$wasm" 2>/dev/null | od -An -tx1 | tr -d ' \n' || true)
    if [ "$magic" = "0061736d" ]; then echo wrote_wasm; return; fi
  fi
  if [ ! -s "$err" ]; then echo empty_diag; return; fi
  # Traps / load failures win over any coincidental "compile" substring in stacks.
  if grep -qiE 'RuntimeError|wasm trap|WebAssembly\.|segmentation|SIG(SEGV|ABRT|BUS)|alarm|timed? out|ENOENT|EACCES|module not found' "$err"; then
    echo runtime_or_timeout_diag; return
  fi
  if [ "$ec" -ne 1 ]; then echo other_nonzero; return; fi
  # Exact CLI reject lines only (whole-line match).
  if grep -qxE 'compile: bad primary|compile: compiler\.ujs core compile failed' "$err"; then
    echo parse_reject; return
  fi
  echo other_nonzero
}

require_parse_reject() {
  local label="$1" ec="$2" err="$3" wasm="$4"
  local cls
  cls=$(classify_compile_outcome "$ec" "$err" "$wasm")
  if [ "$cls" != "parse_reject" ]; then
    echo "FAIL: $label expected parse_reject got class=$cls ec=$ec"
    echo "--- diag ---"; cat "$err" 2>/dev/null || true; echo "---"
    exit 1
  fi
}
