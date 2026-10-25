#!/usr/bin/env bash
# Thin wrapper — no zig/.c. Verifies committed compiler.wasm.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
exec ./unisacc.com -run ujs/seed/stage0/build-compiler-wasm.cx
