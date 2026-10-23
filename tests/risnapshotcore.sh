#!/bin/sh
# RI snapshot core (writer and loader, resourceledger.h): positive load, wrong stage, existing target, no COMMIT, capacity
# overflow (nothing published), read-only target (write failure). Each step is bounded by tests/bound.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
B() { "$R/tests/bound" 55 "$@"; }
W=$(mktemp -d "${TMPDIR:-/tmp}/risnapshotcore.XXXXXX"); trap 'sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "${W:?}"' EXIT HUP INT TERM
B cc -O2 -w -I exec/c tests/ri-snapshot-core.c -o "$W/core" || { echo "risnapshotcore: build failed"; exit 1; }
mkdir -p "$W/run"
B "$W/core" "$W/run" > "$W/core.out" 2>&1 || { cat "$W/core.out"; echo "risnapshotcore FAIL"; exit 1; }
cat "$W/core.out"
