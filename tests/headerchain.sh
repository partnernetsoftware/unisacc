#!/bin/sh
# headerchain.sh -- after editing include/*.h, regenerate everything the headers feed, in order:
#   kernel/unisa_headers.inc (emit-kernel) -> exec/facts (export.py) -> tests/graphhash.tsv, only the
#   pp and parse2 entries the headers reach (the other ~30 entries do not read include/) -> guard check
#   (unisa/libneed.py) -> the previous release still compiles the seed tools (prevheaders.sh).
# Every step is bounded; the graphhash part is sharded so no step passes 58 s.  The whole chain runs
# a few minutes: start it in the background.  0.0.33 D3': done by hand three times, ~10 min each
# with a full 51-entry graphhash pass.
set -u
cd "$(dirname "$0")/.." || exit 2
B="python3 tests/bound.py"
step() { echo "== $*"; "$@" || { echo "headerchain: failed: $*" >&2; exit 1; }; }
step $B 55 python3 -m unisa emit-kernel
step $B 55 python3 exec/facts/export.py
N=${HEADERCHAIN_SHARDS:-4}; k=1
while [ $k -le $N ]; do
  step env GRAPHHASH_JOBS=${GRAPHHASH_JOBS:-2} $B 58 python3 tests/graphhash.py --write --only pp --only parse2 --shard $k/$N
  k=$((k+1))
done
step $B 20 python3 unisa/libneed.py
step ./tests/prevheaders.sh
git status --short kernel exec/facts tests/graphhash.tsv
echo "headerchain: ok"
