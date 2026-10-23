#!/bin/sh
# Model-only: old product is a dynamic-C observer, never product acceptance.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
b(){ "$R/tests/bound" 20 "$@"; }
key(){ { rg --files exec unisa seed include weights | LC_ALL=C sort | rg '\.(tsv|py|c|h|sh|inc|json)$' | xargs sha256sum; cc --version; python3 --version; printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1; }
K=$(key);D=${TMPDIR:-/tmp}/unisacc-libraryfn-$K
if [ ! -r "$D/artifacts.sha256" ] || ! (cd "$D" && sha256sum -c --status artifacts.sha256);then
 T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfn-prep.XXXXXX")
 trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT HUP INT TERM
 b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen"
 b "$T/gen" lower "$T/c.json" --full
 b python3 exec/build/gen.py lower "$T/model.json" --full
 cmp "$T/c.json" "$T/model.json"
 b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
 b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
 b cc -O2 -w exec/lower/libraryfn-observer.c -o "$T/runner"
 [ "$(key)" = "$K" ] || { echo 'libraryfn: preparation inputs changed' >&2; exit 2; }
 (cd "$T" && sha256sum model.json model.tbl model.net runner > artifacts.sha256)
 mkdir -p "$D";cp "$T/model.json" "$T/model.tbl" "$T/model.net" "$T/runner" "$T/artifacts.sha256" "$D/"
 sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}";trap - EXIT HUP INT TERM
fi
case ${1:-check} in --prepare) echo "libraryfn: prepared $K";exit 0;; check) ;; *) exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfn-check.XXXXXX")
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT HUP INT TERM
cp "$D/model.net" "$D/model.tbl" "$D/runner" "$T/"
"$R/tests/bound" 45 sh "${MODEL_COM:-./unisacc.com}" -run tests/libraryfncheck.cx -- "$T"
