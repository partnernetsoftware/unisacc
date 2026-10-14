#!/bin/sh
# Model-only. Old product is the dynamic-C observer; no library API/fresh claim.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
b(){ "$R/tests/bound" 20 "$@"; }
# Content-keyed preparation; --prepare permits a separate bounded preparation window.
key(){ { rg --files exec unisa seed include weights | LC_ALL=C sort | rg '\.(tsv|py|c|h|sh|inc|json)$' | xargs sha256sum; cc --version; python3 --version; printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1; }
K=$(key);D=${TMPDIR:-/tmp}/unisacc-libraryfunctionimports-$K
if [ ! -r "$D/artifacts.sha256" ] || ! (cd "$D" && sha256sum -c --status artifacts.sha256);then
 T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfunctionimports-prep.XXXXXX")
 trap 'rm -rf "$T"' EXIT HUP INT TERM
 b cc -std=c99 -O2 -w -Iseed seed/gen.c -o "$T/gen"
 b "$T/gen" parse2 "$T/c.json"
 b python3 exec/build/gen.py parse2 "$T/model.json"
 cmp "$T/c.json" "$T/model.json"
 b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
 b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
 b cc -O2 -w exec/parse2/libraryfunctionimports-observer.c -o "$T/runner"
 [ "$(key)" = "$K" ] || { echo 'libraryfunctionimports: preparation inputs changed' >&2; exit 2; }
 (cd "$T" && sha256sum model.json model.tbl model.net runner > artifacts.sha256)
 mkdir -p "$D";cp "$T/model.json" "$T/model.tbl" "$T/model.net" "$T/runner" "$T/artifacts.sha256" "$D/"
 rm -rf "$T";trap - EXIT HUP INT TERM
fi
case ${1:-check} in --prepare) echo "libraryfunctionimports: prepared $K";exit 0;; check) ;; *) exit 2;; esac
T=$(mktemp -d "${TMPDIR:-/tmp}/libraryfunctionimports-check.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
cp "$D/model.net" "$D/runner" "$T/";cp tests/libraryfunctionimports-cases.tsv "$T/cases.tsv"
"$R/tests/bound" 35 sh "${MODEL_COM:-./unisacc.com}" -run tests/libraryfunctionimportscheck.cx -- "$T"
