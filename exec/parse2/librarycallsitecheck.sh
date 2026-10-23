#!/bin/sh
# E3 callsite model only. Reuse typed reader's cache/tools owner; cold steps are explicit.
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
BASE=$(sh exec/parse2/librarytypedreadercheck.sh --cache-dir)
K=$(sha256sum exec/parse2/librarycallsitecheck.cx exec/parse2/librarycallsite-cases.tsv | sha256sum | cut -d' ' -f1)
D=$BASE/callsite-$K
mkdir -p "$D"
ready(){ [ -r "$D/$1.sha256" ] && (cd "$D" && sha256sum -c --status "$1.sha256"); }
seal(){ (cd "$D" && sha256sum "$@" >"$STEP.sha256"); }
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";exit 0;;
--prepare-tools)
 if ! { [ -r "$BASE/tools.sha256" ] && (cd "$BASE" && sha256sum -c --status tools.sha256); }; then
  b sh exec/parse2/librarytypedreadercheck.sh --prepare-tools
 fi
 (cd "$BASE" && sha256sum -c --status tools.sha256)
 cp "$BASE/gen" "$BASE/runner" "$D/"
 b cc -O2 -w exec/c/run.c -o "$D/run"
 STEP=tools;seal gen runner run;exit 0;;
--prepare-base-c|--prepare-off-c|--prepare-off-p|--prepare-typed-c|--prepare-typed-reference|--prepare-on-c|--prepare-on-p)
 ready tools || { echo 'callsite E3: NEED_PREPARE tools' >&2;exit 75; }
 STEP=${1#--prepare-}
 case "$STEP" in
 base-c) b "$D/gen" parse2 "$D/$STEP.json" --locations;;
 off-c) b "$D/gen" parse2/librarycallsite "$D/$STEP.json" --locations;;
 off-p) b python3 exec/build/gen.py parse2/librarycallsite "$D/$STEP.json" --locations;;
 typed-c) b "$D/gen" parse2/librarycallsite "$D/$STEP.json" --librarytyped --locations;;
 typed-reference) b "$D/gen" parse2/librarytyped "$D/$STEP.json" --librarytyped --locations;;
 on-c) b "$D/gen" parse2/librarycallsite "$D/$STEP.json" --librarytyped --librarycallsite --locations;;
 on-p) b python3 exec/build/gen.py parse2/librarycallsite "$D/$STEP.json" --librarytyped --librarycallsite --locations;;
 esac
 seal "$STEP.json";exit 0;;
--prepare-tables)
 ready on-c && ready on-p || { echo 'callsite E3: NEED_PREPARE on-c/on-p' >&2;exit 75; }
 cmp "$D/on-c.json" "$D/on-p.json"
 b python3 exec/c/tbl.py "$D/on-c.json" "$D/model.tbl"
 STEP=tables;seal model.tbl;exit 0;;
--prepare-net)
 ready tables || { echo 'callsite E3: NEED_PREPARE tables' >&2;exit 75; }
 b python3 exec/c/net.py "$D/model.tbl" "$D/model.net"
 b "$D/run" --check-net "$D/model.tbl" "$D/model.net" >"$D/exhaustive.log"
 STEP=net;seal model.net exhaustive.log;exit 0;;
check) ;;
*) echo 'usage: librarycallsitecheck.sh --prepare-tools/base-c/off-c/off-p/typed-c/typed-reference/on-c/on-p/tables/net | check' >&2;exit 2;;
esac
for step in tools base-c off-c off-p typed-c typed-reference on-c on-p tables net;do
 ready "$step" || { echo "callsite E3: NEED_PREPARE $step" >&2;exit 75; }
done
cmp "$D/base-c.json" "$D/off-c.json";cmp "$D/off-c.json" "$D/off-p.json"
cmp "$D/typed-c.json" "$D/typed-reference.json"
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
T=$(mktemp -d "${TMPDIR:-/tmp}/callsite-e3.XXXXXX");trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT HUP INT TERM
cp "$D/runner" "$D/model.tbl" "$D/model.net" "$T/"
b sh "$UA" -run exec/parse2/librarycallsitecheck.cx -- "$T"
