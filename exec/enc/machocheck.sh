#!/bin/sh
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
ARCH=${ARCH:-arm64}
case $ARCH in arm64) GEN=arm.py;; x86_64) GEN=gen.py;; *) exit 2;; esac
b python3 "exec/enc/$GEN" "$T/d.json" --macho
b python3 exec/c/tbl.py "$T/d.json" "$T/d.tbl"
b python3 exec/enc/machocheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$ARCH"
