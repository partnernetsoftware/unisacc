#!/bin/sh
# Classic canonical source, constructed weight fragments, independent export.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
bound 30 python3 tests/source_layout_check.py
ua_ready
export UA
bound 45 python3 tests/quotedincludecheck.py
