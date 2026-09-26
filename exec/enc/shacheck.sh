#!/bin/sh
# Mach-O signing prerequisite. No hash-specific executor instruction.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
b python3 exec/enc/shacheck.py "$T/run" "$T/sha.json"
