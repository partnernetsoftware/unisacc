#!/bin/sh
# Exact decimal bits and boundary rounding; no floating helper in the executor.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b python3 exec/parse2/gen2.py "$T/e3.json"
b python3 exec/parse2/floatconstcheck.py "$T/e3.json" "$T/check"
