#!/bin/sh
# One carried model package and two carried ISA cores, no external kernel.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b ./exec/c/buildcompiler.sh "$T/build" > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/c/containercheck.py "$T" "$UA"
