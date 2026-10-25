#!/usr/bin/env bash
# Thin wrapper: prefer unisacc.com + .cx (no zig required for host1).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
exec ./unisacc.com -run ujs/product/build.cx
