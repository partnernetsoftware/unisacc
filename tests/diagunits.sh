#!/bin/sh
# Per-unit source positions: fixed expectations plus a host-compiler check.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. ./tests/lib.sh; ua_ready
perl -e 'alarm 60; exec @ARGV' python3 tests/diagunits.py "$UA"
