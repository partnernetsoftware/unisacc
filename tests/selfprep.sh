#!/bin/sh
# Warm the content-keyed model cache (exec/pipeline/models.py) for one
# selfcheck.sh target: cold construction is ~40 s and the warm stages part
# ~18 s, so exec-selfelf cold (58 s) hit the 60 s watchdog with rc=142.
# selfcheck.sh still self-builds on a miss, so queue order does not matter.
#   usage: tests/selfprep.sh TARGET [NETWORK]
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
TARGET=$1 tests/bound 58 python3 exec/pipeline/models.py "$T" "$1" "${2:-1}" "${EXEC_CC:-cc}" > "$T/log" 2>&1 || { cat "$T/log"; exit 1; }
tail -1 "$T/log"
