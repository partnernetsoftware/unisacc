#!/bin/sh
# Model selftest; old product runs dynamic C as an observation tool.
# Prepare each route in a separate <=55s window before the named check.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
B=$("$R/tests/bound" --helper)
b(){ "$B" 20 "$@"; }
key(){
 { rg --files exec unisa seed include weights | LC_ALL=C sort | rg '\.(tsv|py|c|h|sh|inc|json)$' | xargs sha256sum
   sha256sum tests/librarydatacheck.cx tests/librarydata-cases.tsv
   cc --version;python3 --version;printf '%s\n' "$R"; } | sha256sum | cut -d' ' -f1
}
K=$(key);D=${TMPDIR:-/tmp}/unisacc-librarydata-$K
ready(){ [ -r "$D/$1/artifacts.sha256" ] && (cd "$D/$1" && sha256sum -c --status artifacts.sha256); }
case ${1:-check} in
 --prepare)
  [ "$#" = 2 ] || exit 2
  r=$2
  case "$r" in full) set -- --full;; arm) set -- --full --arm64;; object) set -- --full --object;; *) exit 2;; esac
  if ready "$r";then echo "librarydata: prepared $r $K";exit 0;fi
  T=$(mktemp -d "${TMPDIR:-/tmp}/librarydata-prep.XXXXXX")
  trap 'rm -rf "$T"' EXIT HUP INT TERM
  b cc -O2 -w seed/gen.c -o "$T/gen"
  b "$T/gen" lower "$T/c.json" "$@"
  b python3 exec/build/gen.py lower "$T/model.json" "$@"
  cmp "$T/c.json" "$T/model.json"
  b python3 exec/c/tbl.py "$T/model.json" "$T/model.tbl"
  b python3 exec/c/net.py "$T/model.tbl" "$T/model.net"
  b cc -O2 -w exec/lower/librarydata-observer.c -o "$T/runner"
  [ "$(key)" = "$K" ] || { echo 'librarydata: preparation inputs changed' >&2;exit 2; }
  (cd "$T" && sha256sum model.json model.tbl model.net runner > artifacts.sha256)
  mkdir -p "$D/$r"
  cp "$T/model.json" "$T/model.tbl" "$T/model.net" "$T/runner" "$T/artifacts.sha256" "$D/$r/"
  echo "librarydata: prepared $r $K"
  exit 0;;
 check) ;;
 *) exit 2;;
esac
for r in full arm object;do ready "$r" || { echo "librarydata: NEED_PREPARE $r: tests/bound 55 sh exec/lower/librarydatacheck.sh --prepare $r" >&2;exit 75; };done
T=$(mktemp -d "${TMPDIR:-/tmp}/librarydata-check.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
for r in full arm object;do
 cp "$D/$r/model.tbl" "$T/model.$r.tbl"
 cp "$D/$r/model.net" "$T/model.$r.net"
done
cp "$D/full/runner" "$T/runner"
"$B" 45 sh "${MODEL_COM:-./unisacc.com}" -run tests/librarydatacheck.cx -- "$T"
