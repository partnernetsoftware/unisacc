#!/bin/sh
# Four named model shards; common execution/timeout/cache owner are existing ones.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd);cd "$R"
UA=${MODEL_COM:-$R/unisacc.com}
# Entry selftests are deliberately independent of construction/cache preparation.
if [ "${1:-}" = --selftest ];then
 part=${2:?part};case $part in 1|2|3|4) ;;*) exit 2;;esac
 O=$(mktemp -d "${TMPDIR:-/tmp}/callback-entry-selftest.XXXXXX")
 trap 'rm -rf "$O"' EXIT HUP INT TERM
 python3 tests/bound.py 55 python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --out "$O" --part "$part" --selftest
 exit $?
fi
B=$("$R/tests/bound" --helper)
b(){ "$B" 55 "$@"; }
BASE=$(sh exec/lower/librarytypedcheck.sh --cache-dir)
K=$({ find tests/seals/librarycallbacklower -name '*.sha256' -type f -print | LC_ALL=C sort | xargs sha256sum; cut -f1 tests/librarycallback-reads.tsv | xargs sha256sum;sha256sum tests/librarycallback-reads.tsv tests/librarycallbacklowercheck.sh tests/librarycallbacklowercheck.cx tests/librarycallback-refusalcheck.cx tests/librarycallbackproduce.py tests/librarycallbackcapture.c; } | sha256sum | cut -d' ' -f1)
D=$BASE/callback-$K
mkdir -p "$D"
ready(){ [ -f "$1/artifacts.sha256" ] && (cd "$1" && sha256sum -c --status artifacts.sha256); }
flags(){
 case "$1" in
  lnx-x) FLAGS='--full';;lnx-a) FLAGS='--full --arm64';;osx-x) FLAGS='--full --osx';;osx-a) FLAGS='--full --osx --arm64';;
  win-x) FLAGS='--full --win';;win-a) FLAGS='--full --win --arm64';;obj-lnx-x) FLAGS='--full --object';;obj-lnx-a) FLAGS='--full --object --arm64';;
  obj-osx-x) FLAGS='--full --object --osx';;obj-osx-a) FLAGS='--full --object --osx --arm64';;obj-win-x) FLAGS='--full --object --win';;obj-win-a) FLAGS='--full --object --win --arm64';;*) exit 2;;
 esac
}
case ${1:-} in
 --cache-dir) printf '%s\n' "$D";exit 0;;
 --prepare-tools)
  b sh exec/lower/librarytypedcheck.sh --prepare-tools
  (cd "$BASE/tools" && sha256sum -c --status artifacts.sha256)
  mkdir -p "$D/tools";cp "$BASE/tools/gen" "$BASE/tools/run" "$D/tools/"
  b cc -O2 -w tests/librarycallbackcapture.c -o "$D/tools/runner"
  case $(uname -m) in x86_64) ASM=exec/c/librarycall_x86_64.S;;aarch64|arm64) ASM=exec/c/librarycall_arm64.S;;*) echo 'UNVERIFIED_HOST capture' >&2;exit 77;;esac
  b cc -O1 -w -I. -DUS_CALLBACK_CAPTURE_API tests/librarycallbackcapture.c "$ASM" -lffi -ldl -o "$D/tools/capture"
  (cd "$D/tools" && sha256sum gen run runner capture >artifacts.sha256);exit 0;;
 --prepare-off-c|--prepare-off-python|--prepare-off-tbl|--prepare-off-net|--prepare-off-check)
  [ "$#" = 2 ] || exit 2;step=$1;v=$2;flags "$v";FLAGS="$FLAGS --librarytyped"
  ready "$D/tools" || { echo 'NEED_PREPARE callback tools' >&2;exit 75; }
  M=$D/off12/$v;mkdir -p "$M"
  case $step in
   --prepare-off-c) b "$D/tools/gen" lower/librarytyped-model "$M/base-c.json" $FLAGS;b "$D/tools/gen" lower/librarycallback-model "$M/off-c.json" $FLAGS;;
   --prepare-off-python) b python3 exec/build/gen.py lower/librarytyped-model "$M/base-p.json" $FLAGS;b python3 exec/build/gen.py lower/librarycallback-model "$M/off-p.json" $FLAGS;;
   --prepare-off-tbl) b python3 exec/c/tbl.py "$M/base-c.json" "$M/base.tbl";b python3 exec/c/tbl.py "$M/off-c.json" "$M/off.tbl";;
   --prepare-off-net) b python3 exec/c/net.py "$M/base.tbl" "$M/base.net";b python3 exec/c/net.py "$M/off.tbl" "$M/off.net";;
   --prepare-off-check)
    (cd "$M" && sha256sum -c "$R/tests/seals/librarycallbacklower/$v.sha256")
    b cmp "$M/base-c.json" "$M/base-p.json";b cmp "$M/off-c.json" "$M/off-p.json"
    for suffix in -c.json -p.json .tbl .net;do b cmp "$M/base$suffix" "$M/off$suffix";done
    b "$D/tools/run" --check-net "$M/off.tbl" "$M/off.net" >"$M/exhaustive.log"
    (cd "$M" && sha256sum base-c.json base-p.json base.tbl base.net off-c.json off-p.json off.tbl off.net exhaustive.log >artifacts.sha256);;
  esac;exit 0;;
 --prepare-c|--prepare-python|--prepare-tbl|--prepare-net|--prepare-check)
  [ "$#" = 4 ] || exit 2;step=$1;v=$2;cs=$3;cb=$4;case $cs$cb in 00|01|10|11) ;;*) exit 2;;esac
  ready "$D/tools" || { echo 'NEED_PREPARE callback tools' >&2;exit 75; };flags "$v";FLAGS="$FLAGS --librarytyped"
  [ "$cs" = 0 ] || FLAGS="$FLAGS --librarycallsite";[ "$cb" = 0 ] || FLAGS="$FLAGS --librarycallback"
  M=$D/models/$v/cs$cs-cb$cb;mkdir -p "$M"
  case $step in
   --prepare-c) b "$D/tools/gen" lower/librarycallback-model "$M/model-c.json" $FLAGS;;
   --prepare-python) b python3 exec/build/gen.py lower/librarycallback-model "$M/model-p.json" $FLAGS;b cmp "$M/model-c.json" "$M/model-p.json";;
   --prepare-tbl) b python3 exec/c/tbl.py "$M/model-c.json" "$M/model.tbl";;
   --prepare-net) b python3 exec/c/net.py "$M/model.tbl" "$M/model.net";;
   --prepare-check) b cmp "$M/model-c.json" "$M/model-p.json";b "$D/tools/run" --check-net "$M/model.tbl" "$M/model.net" >"$M/exhaustive.log";(cd "$M" && sha256sum model-c.json model-p.json model.tbl model.net exhaustive.log >artifacts.sha256);;
  esac;exit 0;;
 1|2|3|4) part=$1;;
 *) echo 'usage: --selftest 1..4 | --cache-dir | --prepare-tools | --prepare-c/python/tbl/net/check VARIANT CS CB | 1..4' >&2;exit 2;;
esac
ready "$D/tools" || { echo 'NEED_PREPARE callback tools' >&2;exit 75; }
# Fixture producer owns an explicit hash-bound case plan; never guess a ledger,
# synthesize ABSENT or report an empty plan as PASS.
[ -f tests/seals/librarycallbacklower/cases.json ] || { echo 'NEED_PREPARE paired callback fixture/producer' >&2;exit 75; }
T=$(mktemp -d "${TMPDIR:-/tmp}/callback-model-check.XXXXXX");trap 'rm -rf "$T"' EXIT HUP INT TERM
# All twelve pairs are explicitly prepared/hash-bound, three per verdict shard.
for v in lnx-x lnx-a osx-x osx-a win-x win-a obj-lnx-x obj-lnx-a obj-osx-x obj-osx-a obj-win-x obj-win-a;do
 ready "$D/off12/$v" || { echo "NEED_PREPARE callback off $v" >&2;exit 75; }
done
b python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --out "$T/off-cases" --part "$part" --off-artifacts "$D/off12"
b python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --cache "$D" --out "$T/off-verdict" --part "$part" --cases "$T/off-cases/cases.json" --run
if [ "$part" = 1 ];then
 b python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --cache "$D" --out "$T/on-verdict" --part 1 --run
fi
