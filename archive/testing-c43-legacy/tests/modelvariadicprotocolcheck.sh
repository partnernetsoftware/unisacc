#!/bin/sh
# Protocol-only model network, not source variadic callsite integration.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
T=$(scratch)
bound 20 cc -O2 "$R/exec/c/run.c" -o "$T/run"
bound 30 python3 "$R/tests/modelvariadicprotocolcheck.py" "$T/run"
