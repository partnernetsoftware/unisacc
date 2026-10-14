#!/bin/sh
# Build the declared finite stage; dynamic C owns the decision/output contract.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
part=${1:?part 1..3}; case $part in 1) lo=0; hi=140;; 2) lo=140; hi=280;; 3) lo=280; hi=445;; *) exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-tapecheck.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
b() { "$R/tests/bound" 15 "$@"; }
b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen"
b "$T/gen" tapecheck "$T/c.json"
b python3 exec/build/gen.py tapecheck "$T/tapecheck.json"
cmp "$T/c.json" "$T/tapecheck.json"
b python3 -c 'from unisa.tape import SHAPE; print("".join(k+"\t"+"".join(v)+"\n" for k,v in SHAPE.items()),end="")' > "$T/oracle-shape.tsv"
tail -n +2 exec/tapecheck/shape.tsv > "$T/shape.tsv"
cmp "$T/oracle-shape.tsv" "$T/shape.tsv"
b python3 exec/c/tbl.py "$T/tapecheck.json" "$T/tapecheck.tbl"
b python3 exec/c/net.py "$T/tapecheck.tbl" "$T/tapecheck.net"
b cc -O2 exec/c/run.c -o "$T/run"
runner=${MODEL_COM:-./unisacc.com}
"$R/tests/bound" 40 sh "$runner" -run tests/tapecheckmodel.cx -- tests/tapecheck-cases.tsv "$lo" "$hi" "$T" "$T"
