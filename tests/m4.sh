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
echo "m4: PASS"
