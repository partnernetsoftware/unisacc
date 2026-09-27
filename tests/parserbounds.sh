#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. ./tests/lib.sh
ua_ready
export UA
exec python3 tests/parserbounds.py
