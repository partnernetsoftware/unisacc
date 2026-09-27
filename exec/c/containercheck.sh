#!/bin/sh
# One carried model package and two carried ISA cores, no external kernel.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
if [ "${MODEL_COM+x}" = x ]; then
    [ -n "$MODEL_COM" ] && [ -f "$MODEL_COM" ] && [ -x "$MODEL_COM" ] || {
        echo 'containercheck: MODEL_COM must name an executable candidate' >&2; exit 2;
    }
fi
. ./tests/lib.sh; ua_ready
if [ "${MODEL_COM+x}" != x ]; then
    b ./exec/c/buildcompiler.sh "$T/build" > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
fi
b python3 exec/c/containercheck.py "$T" "$UA"
