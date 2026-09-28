#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Host-side Windows binding check; native runs are opt-in winmemoryrun.py.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
ARCH=${1:-arm64}
case $ARCH in arm64) FLAG=--arm64; ENCODER=arm.py;; x86_64) FLAG=; ENCODER=gen.py;; *) exit 2;; esac
b cc -O2 exec/c/run.c -o "$T/run"
b python3 exec/lower/gen.py "$T/lower.json" --full --win $FLAG
b python3 "exec/enc/$ENCODER" "$T/elf.json" --pe
for s in lower elf; do
 b python3 exec/c/tbl.py "$T/$s.json" "$T/$s.tbl"
 b python3 exec/c/net.py "$T/$s.tbl" "$T/$s.net"
 b "$T/run" --check-net "$T/$s.tbl" "$T/$s.net"
done
b python3 exec/c/winmemorycheck.py "$T" "win/$ARCH" "$UA"
b "$UA" -O2 -b "win/$ARCH" exec/c/compiler.c -o "$T/driver.exe"
b python3 exec/c/processcheck.py "$T" "$T/driver.exe" "$UA"
