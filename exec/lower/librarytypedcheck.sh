#!/bin/sh
# Model-only: existing cx + native executor/reference are observation tools.
set -eu
# Consumption is strictly prepared-only; this explicit flag cannot prepare.
if [ "${1:-}" = --require-cached ]; then
 shift
 case ${1:-check} in --*) echo 'require-cached: prepare options forbidden' >&2; exit 2;; esac
fi
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
B=$("$R/tests/bound" --helper)
b(){ "$B" 55 "$@"; }
UA=${MODEL_COM:-$R/unisacc.com}
K=$({ rg --files exec seed unisa include weights | LC_ALL=C sort | rg '\.(tsv|py|c|h|sh|inc|json)$' | xargs sha256sum;sha256sum tests/librarytypedlowercheck.cx tests/librarytypedlower-cases.tsv "$UA";cc --version;python3 --version;uname -sm; } | sha256sum | cut -d' ' -f1)
D=${TMPDIR:-/tmp}/unisacc-librarytypedlower-$K
mkdir -p "$D"
ready(){ [ -r "$D/$1/artifacts.sha256" ] && (cd "$D/$1" && sha256sum -c --status artifacts.sha256); }
case ${1:-} in
 --cache-dir) printf '%s\n' "$D";exit 0;;
 --prepare-tools)
  mkdir -p "$D/tools"
  b cc -std=c99 -O2 -Iseed seed/gen.c -o "$D/tools/gen"
  b cc -O2 -w exec/lower/librarytyped-observer.c -o "$D/tools/runner"
  b cc -O2 -w exec/lower/librarytyped-oracle.c -lffi -o "$D/tools/abi-oracle"
  b cc -O2 -w exec/c/run.c -o "$D/tools/run"
  (cd "$D/tools" && sha256sum gen runner abi-oracle run >artifacts.sha256);exit 0;;
 --prepare)
  [ "$#" = 2 ] || exit 2
  name=$2
  case $name in
   lnx-x) set -- --full;;lnx-a) set -- --full --arm64;;
   osx-x) set -- --full --osx;;osx-a) set -- --full --osx --arm64;;
   win-x) set -- --full --win;;win-a) set -- --full --win --arm64;;
   obj-lnx-x) set -- --full --object;;obj-lnx-a) set -- --full --object --arm64;;
   obj-osx-x) set -- --full --object --osx;;obj-osx-a) set -- --full --object --osx --arm64;;
   obj-win-x) set -- --full --object --win;;obj-win-a) set -- --full --object --win --arm64;;*) exit 2;;
  esac
  ready tools || { echo 'typed lower: NEED_PREPARE tools' >&2;exit 75; }
  T=$(mktemp -d "${TMPDIR:-/tmp}/typedlower-prep.XXXXXX")
  trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge --max-files 10000 -- "$T"' EXIT HUP INT TERM   # T holds a copy of exec/ (~1700 entries)
  b "$D/tools/gen" lower/librarytyped-model "$T/off-c.json" "$@"
  b python3 exec/build/gen.py lower/librarytyped-model "$T/off.json" "$@"
  cmp "$T/off-c.json" "$T/off.json"
  mkdir "$T/base";cp -R exec "$T/base/exec"
  b sh "$UA" -run tests/librarytypedlowercheck.cx -- --baseline "$T/base/exec/lower/gen-manifest.tsv"
  (cd "$T/base" && "$B" 55 python3 exec/build/gen.py lower "$T/base.json" "$@")
  cmp "$T/base.json" "$T/off.json"
  b "$D/tools/gen" lower/librarytyped-model "$T/model-c.json" "$@" --librarytyped
  b python3 exec/build/gen.py lower/librarytyped-model "$T/model.json" "$@" --librarytyped
  cmp "$T/model-c.json" "$T/model.json"
  for phase in base off model;do
   b python3 exec/c/tbl.py "$T/$phase.json" "$T/$phase.tbl"
   b python3 exec/c/net.py "$T/$phase.tbl" "$T/$phase.net"
  done
  cmp "$T/base.tbl" "$T/off.tbl";cmp "$T/base.net" "$T/off.net"
  b "$D/tools/run" --check-net "$T/model.tbl" "$T/model.net" >"$T/exhaustive.log"
  (cd "$T" && sha256sum off-c.json off.json base.json base.tbl base.net off.tbl off.net model-c.json model.json model.tbl model.net exhaustive.log >artifacts.sha256)
  mkdir -p "$D/$name";cp "$T/"*.json "$T/"*.tbl "$T/"*.net "$T/artifacts.sha256" "$T/exhaustive.log" "$D/$name/"
  exit 0;;
 1|2|3|4) shard=$(($1-1));;
 *) echo 'usage: librarytypedcheck.sh --prepare-tools | --prepare VARIANT | 1|2|3|4' >&2;exit 2;;
esac
ready tools || { echo 'typed lower: NEED_PREPARE tools' >&2;exit 75; }
case $shard in 0) variants='lnx-x osx-x obj-osx-x';;1) variants='lnx-a win-x obj-win-x';;2) variants='osx-a obj-lnx-x obj-osx-a';;3) variants='win-a obj-lnx-a obj-win-a';;esac
for v in lnx-x $variants;do ready "$v" || { echo "typed lower: NEED_PREPARE $v" >&2;exit 75; };done
T=$(mktemp -d "${TMPDIR:-/tmp}/typedlower-check.XXXXXX");trap '[ -z "$T" ] || sh "$R/tests/cxbin" "$R/tests/safedel.cx" --purge -- "$T"' EXIT HUP INT TERM
cp "$D/tools/runner" "$D/tools/abi-oracle" "$T/"
cp "$D/lnx-x/model.tbl" "$D/lnx-x/model.net" "$T/"
b sh "$UA" -run tests/librarytypedlowercheck.cx -- "$T" "$shard" 1
for v in $variants;do
 case $v in *win*) target=0;;*osx*-a) target=4;;*osx*-x) target=3;;*-a) target=2;;*) target=1;;esac
 cp "$D/$v/model.tbl" "$D/$v/model.net" "$T/"
 b sh "$UA" -run tests/librarytypedlowercheck.cx -- "$T" 0 "$target" smoke
done
