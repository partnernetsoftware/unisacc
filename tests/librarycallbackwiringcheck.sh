#!/bin/sh
# Preparation follows the existing compilercheck/typedlower cache and bounded steps.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd);cd "$R"
B=$(tests/bound --helper);b(){ "$B" 55 "$@"; }
PRODUCT=${MODEL_COM:-$R/unisacc.com}
BASE=$(sh exec/lower/librarytypedcheck.sh --cache-dir)
K=$({ sha256sum tests/librarycallbackwiringcheck.c tests/librarycallbackwiringcheck.sh tests/checklib.py src/host_dl.h src/tapebin_shape.inc src/tapebin_encode.inc; } | sha256sum | cut -d' ' -f1)
D=$BASE/callback-wiring-$K;mkdir -p "$D"
case $(uname -s)/$(uname -m) in Linux/x86_64) TARGET=lnx/x86_64;ASM=x86_64;;Linux/aarch64) TARGET=lnx/arm64;ASM=arm64;;Darwin/arm64) TARGET=osx/arm64;ASM=arm64;;Darwin/x86_64) TARGET=osx/x86_64;ASM=x86_64;;*) echo 'UNVERIFIED_HOST callback prepare wiring';exit 77;;esac
case $TARGET in osx/*) DL=;;*) DL=-ldl;;esac
export TARGET
need(){ [ -r "$1" ] || { echo "NEED_PREPARE callback wiring $1" >&2;exit 75; }; }
pack(){ b python3 exec/c/compilerpack.py --librarytyped --librarycallback --o1 "$D/base/o1.net" --include include -o "$D/callback.pkg" "$@" "$D/base/route.tsv"; }
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";;
--prepare-elf) mkdir -p "$D/base";b sh exec/pipeline/elf.sh "$D/base" examples/hello.c;;
--prepare-opt)
 need "$D/base/run";b python3 exec/build/gen.py opt "$D/base/o1.json";b python3 exec/c/tbl.py "$D/base/o1.json" "$D/base/o1.tbl";b python3 exec/c/net.py "$D/base/o1.tbl" "$D/base/o1.net";b "$D/base/run" --check-net "$D/base/o1.tbl" "$D/base/o1.net";;
--prepare-model-1|--prepare-model-2|--prepare-model-3|--prepare-model-4|--prepare-model-5)
 need "$D/base/route.tsv";need "$D/base/o1.net";n=${1##*-};pack --prepare-only --part "$n/5";;
--prepare-package)
 need "$D/base/route.tsv";need "$D/base/o1.net";pack --require-cached;(cd "$D" && sha256sum callback.pkg >package.sha256);;
--prepare-probe)
 b cc -std=c11 -O1 -g -w -fsanitize=address -fno-omit-frame-pointer -I. tests/librarycallbackwiringcheck.c "exec/c/librarycall_$ASM.S" -lffi $DL -o "$D/probe";(cd "$D" && sha256sum probe >probe.sha256);;
check)
 need "$D/package.sha256";need "$D/probe.sha256";(cd "$D" && sha256sum -c --status package.sha256 && sha256sum -c --status probe.sha256)
 # Shipping condition: the product and its two production build entries contain no test observer.
 if LC_ALL=C grep -a -E 'us_late_test_callback_carrier_status|test callback certificate mechanism failure' "$PRODUCT";then echo 'shipping test observer leaked' >&2;exit 1;fi
 if grep -E 'US_LATE_TEST_OBSERVERS' release/tools/build_candidate.sh exec/c/compilerpack.py;then echo 'shipping build defines test observer' >&2;exit 1;fi
 ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 b "$D/probe" "$PRODUCT" "$D/callback.pkg" "$TARGET";;
*) echo 'usage: --cache-dir | --prepare-elf/opt/model-{1..5}/package/probe | check' >&2;exit 2;;
esac
