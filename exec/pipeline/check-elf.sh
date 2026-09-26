#!/bin/sh
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
# Fixed end-to-end set; a missing source or any rejecting stage fails.
[ -s exec/pipeline/keep-elf.txt ] || { echo 'empty ELF keep list'; exit 1; }
set -- $(cat exec/pipeline/keep-elf.txt)
b ./exec/pipeline/elf.sh "$T" "$@" > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/pipeline/check-elf.py "$UA" "$T" "$@"
