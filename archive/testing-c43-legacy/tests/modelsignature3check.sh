#!/bin/sh
# SIG3 framing/equality only; each mode is a separately bounded gate job.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd)
. "$R/tests/lib.sh"
case ${1:-} in canonical|equal) ;; *) echo 'usage: modelsignature3check.sh canonical|equal' >&2; exit 2;; esac
T=$(scratch)
bound 20 cc -O2 "$R/exec/c/run.c" -o "$T/run"
bound 50 python3 "$R/tests/modelsignature3check.py" "$T/run" "$1"
