#!/bin/sh
# Independent category model. Product is a dynamic-C observer, not product evidence.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
case ${1:-} in 1|2|3|4) shard=$((${1}-1));; *) echo 'usage: check.sh 1|2|3|4' >&2;exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-librarytypedpartition.XXXXXX")
trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT HUP INT TERM
B=$("$R/tests/bound" --helper)
b(){ "$B" 15 "$@"; }
b cc -O2 -w -Iseed seed/gen.c -o "$T/gen"
b "$T/gen" librarytypedpartition/gen "$T/c.json"
b python3 exec/build/gen.py librarytypedpartition/gen "$T/model.json"
cmp "$T/c.json" "$T/model.json"
b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
b cc -O2 exec/c/run.c -o "$T/run"
b "$T/run" --check-net "$T/model.tbl" "$T/model.net"
"$B" 53 sh "${MODEL_COM:-./unisacc.com}" -run tests/librarytypedpartitioncheck.cx -- "$T" "$shard"
