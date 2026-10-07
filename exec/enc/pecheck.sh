#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b cc -O2 -o "$T/run" exec/c/run.c
case ${PE_ARCH:-arm64} in arm64) encoder=enc/arm; lowerarch=--arm64;; x86_64) encoder=enc; lowerarch=;; *) exit 2;; esac
b python3 exec/build/gen.py "$encoder" "$T/arm.json" --pe
b python3 exec/c/tbl.py "$T/arm.json" "$T/arm.tbl"
b python3 exec/enc/pecheck.py "$T/run" "$T/arm.tbl" "$T/arm.json"
b python3 tests/enc/pereal.py "$T/run" "$T/arm.tbl" "${PE_OUT:-$T/real}"
b ./tests/build_ref.sh "$T/ref.c" "$T/ref"
b python3 exec/build/gen.py lower "$T/lower.json" $lowerarch --win --full
b python3 tests/enc/peforward.py "$T/ref" "$T/lower.json" "$T/arm.json" "$T/run" "$T/arm.tbl" "${PE_ARCH:-arm64}"
