#!/bin/bash
# Diagnostic helper: copy bootstrap scratch outs BEFORE lib.sh EXIT trap removes them.
# Activate only when UNISACC_DIAG_OUT is a writable directory (else no-op).
# Does not change pass/fail; does not loosen gates.
set -eu
diag_copy_bootstrap_tree() {
  local src="${1:-}"
  local dst="${UNISACC_DIAG_OUT:-}"
  [ -n "$dst" ] || return 0
  [ -d "$src" ] || return 0
  mkdir -p "$dst/bootstrap"
  # Best-effort; never fail the suite solely because copy failed.
  set +e
  for f in B.log U.log bs_B.tape bs_C.tape bs_UT.tape bs_B bs_U unisacc.flat.c; do
    [ -e "$src/$f" ] || continue
    cp -p "$src/$f" "$dst/bootstrap/$f" 2>/dev/null
  done
  # Sign / identity crumbs
  {
    echo "host=$(uname -a)"
    echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    command -v sw_vers >/dev/null && sw_vers
    [ -f "$src/bs_B" ] && shasum -a 256 "$src/bs_B"
    [ -f "$src/bs_U" ] && shasum -a 256 "$src/bs_U"
    [ -f "$src/bs_B" ] && (codesign -dv --verbose=2 "$src/bs_B" 2>&1 || true)
  } >"$dst/bootstrap/identity.txt" 2>/dev/null
  # Optional crash crumbs (macOS)
  if [ "$(uname -s)" = Darwin ]; then
    mkdir -p "$dst/crash"
    ls -lt "$HOME/Library/Logs/DiagnosticReports" 2>/dev/null | head -20 >"$dst/crash/diagreports.head" || true
    # sample/lldb only if bs_B still exists and caller set UNISACC_DIAG_LLDB=1
    if [ "${UNISACC_DIAG_LLDB:-0}" = 1 ] && [ -x "$src/bs_B" ]; then
      python3 - <<PY 2>"$dst/crash/lldb.err" | head -80 >"$dst/crash/lldb.out" || true
import subprocess, os, sys
bs=os.path.join("$src","bs_B")
selfp=os.path.join("$src","unisacc.flat.c")
cmd=["lldb","--batch","-o","run","-o","bt all","-o","register read pc sp","-o","quit","--",bs,selfp,"-c"]
subprocess.run(cmd,timeout=20)
PY
    fi
  fi
  set -e
  return 0
}

# --- drop-in instrumentation sketch for tests/bootstrap.sh (env-gated) ---
# After: T=$(scratch)
# Before each exit / after wait on branches, call:
#   diag_copy_bootstrap_tree "$T"
# Or source this file and wrap the FAIL path:
#   if [ "$brc" -ne 0 ] || [ "$urc" -ne 0 ]; then
#     diag_copy_bootstrap_tree "$T"
#     echo "  FAIL bootstrap branches B=$brc U=$urc"
#     ...
#   fi
# Always copy once before final exit as well when UNISACC_DIAG_OUT set:
#   diag_copy_bootstrap_tree "$T"
