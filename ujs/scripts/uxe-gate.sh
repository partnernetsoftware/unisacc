#!/usr/bin/env bash
# UXE autonomous gate — start local static server if needed, run all UXE alarms.
# No human: green or exit non-zero. Bypass HTTP proxies for localhost.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UJS="$ROOT/ujs"
export NO_PROXY='*' no_proxy='*'
unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY ALL_PROXY all_proxy || true

cd "$UJS"

STARTED=0
if ! curl --noproxy '*' -sf -o /dev/null "http://127.0.0.1:8765/uxe/" 2>/dev/null; then
  # unisacc.com execs the server binary, so $! is the server itself; a cold start compiles it first (~4 s)
  sh "$ROOT/unisacc.com" "$UJS/scripts/static-serve.cx" -- 8765 "$UJS" >/tmp/uxe-gate-8765.log 2>&1 &
  SPID=$!
  STARTED=1
  cleanup() { kill -9 "$SPID" 2>/dev/null || true; }
  trap cleanup EXIT
  for i in $(seq 1 100); do
    curl --noproxy '*' -sf -o /dev/null "http://127.0.0.1:8765/uxe/" && break
    sleep 0.2
  done
  curl --noproxy '*' -sf -o /dev/null "http://127.0.0.1:8765/uxe/" \
    || { echo "FAIL_GATE server"; exit 1; }
fi

echo "== UXE gate =="
npm run test:uxe:packet
npm run test:uxe:input
npm run test:uxe
npm run test:uxe:ship
npm run test:uxe:drone:rules
npm run test:uxe:drone
npm run test:uxe:drone:ship
npm run test:uxe:snap
echo "OK_UXE_GATE"
