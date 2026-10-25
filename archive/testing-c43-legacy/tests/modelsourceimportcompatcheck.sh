#!/bin/sh
# Cross-origin model compatibility and strict equality mode isolation.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
T=$(scratch)
bound 20 cc -O2 "$R/exec/c/run.c" -o "$T/run"
bound 55 python3 "$R/tests/modelsourceimportcompatcheck.py" "$T/run"
