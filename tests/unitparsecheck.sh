#!/bin/sh
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
_BOUND=$(./tests/bound --helper)
b() { "$_BOUND" 58 "$@"; }
. ./exec/stamp.sh
fresh "$X/unitparse-ref" b ./tests/build_ref.sh "$X/unitparse-ref.c" "$X/unitparse-ref" -- $REFSRC
fresh "$X/unitparse.json" b python3 exec/build/gen.py parse2 "$X/unitparse.json" -- exec/parse2/*.py exec/parse2/*.tsv exec/parse/*.py $PYSRC
fresh "$X/unitparse-dump" b ./exec/parse/mkdump.sh "$X/unitparse-ref.c" "$X/unitparse-dump" -- "$X/unitparse-ref.c" exec/parse/mkdump.sh
b python3 tests/unitparsecheck.py "$X/unitparse.json" "$X/unitparse-ref" "$X/unitparse-dump"
