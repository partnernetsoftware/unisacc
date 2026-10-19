#!/bin/bash
# m4 轨最小套件：10 网 acc + 四流 check（与 ujs 旧路径逐字节对比）。
#   ./tests/m4.sh
# 每步各自 60 秒看门狗（tests/bound.py）。
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
B="python3 tests/bound.py 55"

$B python3 -m m4 acc
$B python3 -m m4 check
$B python3 -m m4 selftest

# 可跑：m4 产出的模块在 Node 里实例化并求值，与 expect.json 的 fact=120 一致
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
$B python3 -m m4 build tests/ujs2wasm/corpus/fact.ujs -o "$TMP/fact.wasm" >/dev/null
GOT="$($B node tests/ujs2wasm/run_wasm.mjs "$TMP/fact.wasm")"
[ "$GOT" = "120" ] || { echo "m4 run: fact got '$GOT' want 120"; exit 1; }
echo "m4 run: fact = 120"
echo "m4: PASS"
