#!/bin/sh
# Run several exec/pp/run.sh shards in turn (each bounded by run.sh), one
# summary line per shard.  Usage: exec/pp/shards.sh SHARD...
cd "$(dirname "$0")/../.."
for s in "$@"; do
    echo "== $s"
    sh exec/pp/run.sh "$s" 2>&1 | grep -v '^  not-covered' | tail -4
done
