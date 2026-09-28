#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Exact decimal bits and boundary rounding; no floating helper in the executor.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { "$_BOUND" 60 "$@"; }
b python3 exec/parse2/gen2.py "$T/e3.json"
b python3 exec/parse2/floatconstcheck.py "$T/e3.json" "$T/check"
