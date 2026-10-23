#!/bin/sh
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
P=$(mktemp -d "$R/out/watchdog-replay.XXXXXXXX")
mkdir -p "$P/tests/cxlib"
cp "$R/tests/cxlib/seedprep.cx" "$P/tests/cxlib/"
cp "$R/tests/bound" "$R/tests/bound.c" "$P/tests/"
cp "$(dirname "$0")/probe.cx" "$P/tests/probe.cx"
cp "$(dirname "$0")/hang.c" "$(dirname "$0")/check.c" "$P/"
"$R/tests/bound" 55 cc -std=c99 -O2 -o "$P/hang" "$P/hang.c"
"$R/tests/bound" 55 cc -std=c99 -O2 -o "$P/check" "$P/check.c"
"$R/tests/bound" 55 sh "$R/unisacc.com" -o "$P/probe" "$P/tests/probe.cx"
UNISACC_BOUND_CLEANUP_TRACE="$P/cleanup.trace" "$R/tests/bound" 40 "$P/check" "$P/probe" "$P/hang" "$P/parent.pid" "$P/grandchild.pid" > "$P/result.log" 2> "$P/command.log"
rg -q 'modelobject command rc=142:' "$P/command.log"
cat "$P/result.log"
printf 'retained proof %s\n' "$P"
