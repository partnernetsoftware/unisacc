#!/bin/sh
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
b ./exec/pipeline/elf.sh "$T" examples/hello.c examples/fib.c > "$T/build.log" 2>&1 || { cat "$T/build.log"; exit 1; }
b python3 exec/pipeline/check-elf.py "$UA" "$T" examples/hello.c examples/fib.c
