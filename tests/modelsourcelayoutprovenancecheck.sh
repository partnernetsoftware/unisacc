#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mode=${1:-plain}
case $mode in plain) flag=;; located) flag=--locations;; *) echo 'expected plain or located' >&2;exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/r10-source-provenance.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
b() { ./tests/bound 20 "$@"; }
b python3 exec/build/gen.py pp "$T/e2.json" --osx --arm64 $flag > "$T/e2.log"
b python3 exec/build/gen.py lex "$T/e1.json" --typed $flag > "$T/e1.log"
b python3 exec/parse2/gen2.py $flag "$T/e3.json" > "$T/e3.log"
if [ -z "$flag" ]; then b python3 exec/build/gen.py parse2/units "$T/units.json" > "$T/units.log" 2>&1; else b python3 exec/parse2/units.py "$T/units.json" $flag > "$T/units.log"; fi
b python3 tests/modelsourcelayoutprovenancecheck.py $flag --e2 "$T/e2.json" --e1 "$T/e1.json" --e3 "$T/e3.json" --units "$T/units.json"
