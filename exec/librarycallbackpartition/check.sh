#!/bin/sh
# Model selftest; installed product runs dynamic C as an observation tool.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
case ${1:-} in 1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16) shard=$((${1}-1));; *) echo 'usage: check.sh 1..16' >&2;exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-librarycallbackpartition.XXXXXX")
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT HUP INT TERM
B=$("$R/tests/bound" --helper)
b(){ "$B" 15 "$@"; }
b cc -O2 -w -Iseed seed/gen.c -o "$T/gen"
b "$T/gen" librarycallbackpartition/gen "$T/c.json" --librarycallback
b python3 exec/build/gen.py librarycallbackpartition/gen "$T/model.json" --librarycallback
b cmp "$T/c.json" "$T/model.json"
b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
b cc -O2 exec/c/run.c -o "$T/run"
b "$T/run" --check-net "$T/model.tbl" "$T/model.net"
"$B" 53 sh "${MODEL_COM:-./unisacc.com}" -run tests/librarycallbackpartitioncheck.cx -- "$T" "$shard"
