#!/bin/sh
# Classic canonical source, constructed weight fragments, independent export.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"
bound 30 sh ./tests/cxbin tests/sourcelayoutcheck.cx
ua_ready
export UA
bound 45 python3 tests/quotedincludecheck.py
