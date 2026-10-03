#!/bin/sh
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
# Cold model preparation. Called by models.py under a content-addressed lock.
set -eu
PYTHONHASHSEED=0; export PYTHONHASHSEED
OUT=$1; TARGET=$2; NETWORK=$3; EXEC_CC=$4
R=$(pwd)
b() { "$_BOUND" 60 "$@"; }
b "${EXEC_CC:-cc}" -O2 -o "$OUT/run" exec/c/run.c
b python3 exec/build/gen.py lex "$OUT/e1.json" --typed
b python3 exec/build/gen.py parse2 "$OUT/e3.json"
b python3 exec/build/gen.py opt "$OUT/e4.json" --o2
b python3 exec/build/gen.py prune "$OUT/prune.json"
case $TARGET in win/*) OSFLAG=--win; IMAGE=pe;; osx/*) OSFLAG=--osx; IMAGE=macho;; *) OSFLAG=; IMAGE=elf;; esac
case $TARGET in */arm64) ARCHFLAG=--arm64; ENCODER=enc/arm;; *) ARCHFLAG=; ENCODER=enc;; esac
b python3 exec/build/gen.py pp "$OUT/e2.json" $OSFLAG $ARCHFLAG
b python3 exec/build/gen.py lower "$OUT/lower.json" --full $OSFLAG $ARCHFLAG
b python3 exec/build/gen.py "$ENCODER" "$OUT/elf.json" "--$IMAGE"
MODEL=tbl
case ${NETWORK:-1} in 0) ;; 1) MODEL=net;; *) echo "NETWORK must be 0 or 1" >&2; exit 2;; esac
STAGES=$(awk '!/^#/ && NF {print $1}' exec/pipeline/image-stages.tsv)
for s in $STAGES; do
    b python3 exec/c/tbl.py "$OUT/$s.json" "$OUT/$s.tbl"
    if [ "$MODEL" = net ]; then
        b python3 exec/c/net.py "$OUT/$s.tbl" "$OUT/$s.net"
        b "$OUT/run" --check-net "$OUT/$s.tbl" "$OUT/$s.net"
    fi
done
if [ "$MODEL" = net ]; then
    awk -v route="$TARGET" '!/^#/ && NF {print route "\t" $0}' exec/pipeline/image-stages.tsv > "$OUT/route.tsv"
    b python3 exec/c/pack.py --mount 006864722f "$R/include" -o "$OUT/models.pkg" "$OUT/route.tsv"
fi
