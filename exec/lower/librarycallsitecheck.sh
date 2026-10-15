#!/bin/sh
# Callsite-only overlay witness. Reuse the existing typed preparation/cache owner.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd);cd "$R"
B=$("$R/tests/bound" --helper)
b(){ "$B" 55 "$@"; }
UA=${MODEL_COM:-$R/unisacc.com}
BASE=$(sh exec/lower/librarytypedcheck.sh --cache-dir)
K=$(sha256sum tests/librarycallsitelowercheck.cx tests/librarycallsitelower-cases.tsv | sha256sum | cut -d' ' -f1)
D=$BASE/callsite-$K
mkdir -p "$D"
ready(){ [ -r "$D/$1/artifacts.sha256" ] && (cd "$D/$1" && sha256sum -c --status artifacts.sha256); }
flags(){
 case "$1" in
  lnx-x) FLAGS='--full';;lnx-a) FLAGS='--full --arm64';;
  osx-x) FLAGS='--full --osx';;osx-a) FLAGS='--full --osx --arm64';;
  win-x) FLAGS='--full --win';;win-a) FLAGS='--full --win --arm64';;
  obj-lnx-x) FLAGS='--full --object';;obj-lnx-a) FLAGS='--full --object --arm64';;
  obj-osx-x) FLAGS='--full --object --osx';;obj-osx-a) FLAGS='--full --object --osx --arm64';;
  obj-win-x) FLAGS='--full --object --win';;obj-win-a) FLAGS='--full --object --win --arm64';;
  *) exit 2;;
 esac
}
case ${1:-} in
 --cache-dir) printf '%s\n' "$D";exit 0;;
 --prepare-tools)
  b sh exec/lower/librarytypedcheck.sh --prepare-tools
  (cd "$BASE/tools" && sha256sum -c --status artifacts.sha256)
  mkdir -p "$D/tools";cp "$BASE/tools/gen" "$BASE/tools/run" "$D/tools/"
  b cc -O2 -w exec/lower/librarycallsite-observer.c -o "$D/tools/runner"
  b cc -O2 -w exec/lower/librarycallsite-oracle.c -lffi -o "$D/tools/abi-oracle"
  (cd "$D/tools" && sha256sum gen run runner abi-oracle >artifacts.sha256);exit 0;;
 --prepare)
  echo 'callsite lower: use --prepare-legacy/on-c/on-p/tables/net/check VARIANT, each separately bounded' >&2;exit 2;;
 --prepare-legacy|--prepare-on-c|--prepare-on-p|--prepare-tables|--prepare-net|--prepare-check)
  [ "$#" = 2 ] || exit 2
  step=$1;v=$2;flags "$v"
  ready tools || { echo 'callsite lower: NEED_PREPARE tools' >&2;exit 75; }
  mkdir -p "$D/$v"
  case $step in
   --prepare-legacy)
    b sh exec/lower/librarytypedcheck.sh --prepare "$v"
    (cd "$BASE/$v" && sha256sum -c --status artifacts.sha256)
    for name in base.json base.tbl base.net off-c.json off.json off.tbl off.net;do cp "$BASE/$v/$name" "$D/$v/$name";done
    (cd "$D/$v" && sha256sum base.json base.tbl base.net off-c.json off.json off.tbl off.net >legacy.sha256);;
   --prepare-on-c) b "$D/tools/gen" lower/librarycallsite-model "$D/$v/model-c.json" $FLAGS --librarytyped --librarycallsite;;
   --prepare-on-p)
    b python3 exec/build/gen.py lower/librarycallsite-model "$D/$v/model.json" $FLAGS --librarytyped --librarycallsite
    b cmp "$D/$v/model-c.json" "$D/$v/model.json";;
   --prepare-tables) b python3 exec/c/tbl.py "$D/$v/model.json" "$D/$v/model.tbl";;
   --prepare-net) b python3 exec/c/net.py "$D/$v/model.tbl" "$D/$v/model.net";;
   --prepare-check)
    (cd "$D/$v" && sha256sum -c --status legacy.sha256)
    b "$D/tools/run" --check-net "$D/$v/model.tbl" "$D/$v/model.net" >"$D/$v/exhaustive.log"
    b cmp "$D/$v/model-c.json" "$D/$v/model.json"
    (cd "$D/$v" && sha256sum model-c.json model.json model.tbl model.net base.json base.tbl base.net off-c.json off.json off.tbl off.net exhaustive.log >artifacts.sha256);;
  esac
  exit 0;;
 1|2|3|4) shard=$(($1-1));;
 *) echo 'usage: librarycallsitecheck.sh --cache-dir | --prepare-tools | --prepare-legacy/on-c/on-p/tables/net/check VARIANT | 1..4' >&2;exit 2;;
esac
ready tools || { echo 'callsite lower: NEED_PREPARE tools' >&2;exit 75; }
case $shard in 0) variants='lnx-x osx-x obj-osx-x';;1) variants='lnx-a win-x obj-win-x';;2) variants='osx-a obj-lnx-x obj-osx-a';;3) variants='win-a obj-lnx-a obj-win-a';;esac
for v in lnx-x $variants;do ready "$v" || { echo "callsite lower: NEED_PREPARE $v" >&2;exit 75; };done
T=$(mktemp -d "${TMPDIR:-/tmp}/callsitelower-check.XXXXXX");trap 'rm -rf "$T"' EXIT HUP INT TERM
cp "$D/tools/runner" "$D/tools/abi-oracle" "$T/"
cp "$D/lnx-x/model.tbl" "$D/lnx-x/model.net" "$T/"
b sh "$UA" -run tests/librarycallsitelowercheck.cx -- "$T" "$shard" 1
for v in $variants;do
 case $v in *win*) target=0;;*osx*-a) target=4;;*osx*-x) target=3;;*-a) target=2;;*) target=1;;esac
 cp "$D/$v/model.tbl" "$D/$v/model.net" "$T/"
 b sh "$UA" -run tests/librarycallsitelowercheck.cx -- "$T" 0 "$target" smoke
done
