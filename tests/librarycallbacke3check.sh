#!/bin/sh
# E3 semantic owner, shared execution/verdict/RF adapter and typed cache owner.
set -eu
# Consumption is strictly prepared-only; this explicit flag cannot prepare.
if [ "${1:-}" = --require-cached ]; then
 shift
 case ${1:-check} in --*) echo 'require-cached: prepare options forbidden' >&2; exit 2;; esac
fi
R=$(cd "$(dirname "$0")/.." && pwd);cd "$R"
if [ "${1:-}" = --selftest ];then exec python3 tests/librarycallbackentrycheck.py --reader;fi
UA=${MODEL_COM:-$R/unisacc.com}
BASE=$(sh exec/lower/librarytypedcheck.sh --cache-dir)
K=$({ find tests/seals/librarycallbacke3/control -type f -print | LC_ALL=C sort | xargs sha256sum;sha256sum tests/reseal-librarycallbacke3.py; cut -f1 tests/librarycallback-reads.tsv | xargs sha256sum;sha256sum tests/librarycallback-reads.tsv tests/librarycallbacke3check.sh tests/librarycallbacke3check.cx tests/librarycallbacke3capture.c tests/librarycallbackproduce.py; } | sha256sum | cut -d' ' -f1)
D=$BASE/callback-e3-$K;mkdir -p "$D"
b(){ python3 "$R/tests/bound.py" 55 "$@"; }
ready(){ [ -f "$1/artifacts.sha256" ] && (cd "$1" && sha256sum -c --status artifacts.sha256); }
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";;
--prepare-gen) mkdir -p "$D/tools";b cc -std=c99 -O2 -Iseed seed/gen.c -o "$D/tools/gen";(cd "$D/tools" && sha256sum gen >gen.sha256);;
--prepare-runner) mkdir -p "$D/tools";b cc -O2 -w tests/librarycallbackcapture.c -o "$D/tools/runner";b cc -O2 -w exec/c/run.c -o "$D/tools/run";(cd "$D/tools" && sha256sum runner run >runner.sha256);;
--prepare-capture) mkdir -p "$D/tools";case $(uname -m) in x86_64) ASM=exec/c/librarycall_x86_64.S;;aarch64|arm64) ASM=exec/c/librarycall_arm64.S;;*) exit 77;;esac;b cc -O1 -w -I. tests/librarycallbacke3capture.c "$ASM" -lffi -ldl -o "$D/tools/capture";(cd "$D/tools" && sha256sum capture >capture.sha256);;
--prepare-control-source)
 b python3 tests/reseal-librarycallbacke3.py --family control --spec tests/seals/librarycallbacke3/control/spec.json --out "$D/control";;
--prepare-control-c)
 (cd "$D/control/source" && b "$D/tools/gen" parse2/librarycallbacke3 "$D/control/model-c.json" --librarytyped --librarycallback --locations);;
--prepare-control-python)
 (cd "$D/control/source" && b python3 exec/build/gen.py parse2/librarycallbacke3 "$D/control/model-p.json" --librarytyped --librarycallback --locations);;
--prepare-control-tbl)
 b python3 exec/c/tbl.py "$D/control/model-c.json" "$D/control/model.tbl";;
--prepare-control-check)
 b cmp "$D/control/model-c.json" "$D/control/model-p.json"
 (cd "$D/control" && sha256sum -c "$R/tests/seals/librarycallbacke3/control/expected.sha256");;
--prepare-c|--prepare-python|--prepare-tbl|--prepare-net|--prepare-check)
 [ "$#" = 2 ] || exit 2;mode=$2;case $mode in off) FLAGS='--locations';;on) FLAGS='--librarytyped --librarycallback --locations';;*) exit 2;;esac
 M=$D/models/$mode;mkdir -p "$M";(cd "$D/tools" && sha256sum -c --status gen.sha256)
 case $1 in
 --prepare-c) b "$D/tools/gen" parse2/librarycallbacke3 "$M/model-c.json" $FLAGS;;
 --prepare-python) b python3 exec/build/gen.py parse2/librarycallbacke3 "$M/model-p.json" $FLAGS;b cmp "$M/model-c.json" "$M/model-p.json";;
 --prepare-tbl) b python3 exec/c/tbl.py "$M/model-c.json" "$M/model.tbl";;
 --prepare-net) b python3 exec/c/net.py "$M/model.tbl" "$M/model.net";;
 --prepare-check) b cmp "$M/model-c.json" "$M/model-p.json";(cd "$D/tools" && sha256sum -c --status runner.sha256);b "$D/tools/run" --check-net "$M/model.tbl" "$M/model.net";(cd "$M" && sha256sum model-c.json model-p.json model.tbl model.net >artifacts.sha256);;
 esac;;
check)
 ready "$D/models/on" || { echo 'NEED_PREPARE callback E3 on models' >&2;exit 75; }
 (cd "$D/tools" && sha256sum -c --status runner.sha256)
 [ -f tests/seals/librarycallbacke3/cases.json ] || { echo 'NEED_PREPARE hash-bound callback E3 cases, not PASS' >&2;exit 75; }
 O=$(mktemp -d "${TMPDIR:-/tmp}/callback-e3-check.XXXXXX");trap 'rm -rf "$O"' EXIT HUP INT TERM
 b python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --cache "$D" --out "$O" --part 1 --cases tests/seals/librarycallbacke3/cases.json --checker tests/librarycallbacke3check.cx --run;;
*) echo 'usage: --selftest | --cache-dir | --prepare-gen/runner/capture | --prepare-c/python/tbl/net/check off|on | check' >&2;exit 2;;
esac
