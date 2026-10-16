#!/bin/sh
# Existing candidate certificate/registry probe; shared producer, no guest invocation.
set -eu
# Consumption is strictly prepared-only; this explicit flag cannot prepare.
if [ "${1:-}" = --require-cached ]; then
 shift
 case ${1:-check} in --*) echo 'require-cached: prepare options forbidden' >&2; exit 2;; esac
fi
R=$(cd "$(dirname "$0")/.." && pwd);cd "$R"
if [ "${1:-}" = --selftest ];then exec python3 tests/librarycallbackentrycheck.py --auth;fi
UA=${MODEL_COM:-$R/unisacc.com};BASE=$(sh exec/lower/librarytypedcheck.sh --cache-dir)
K=$({ cut -f1 tests/librarycallback-reads.tsv | xargs sha256sum;sha256sum tests/librarycallback-reads.tsv tests/librarycallbackauthcheck.sh tests/librarycallbackauthcheck.cx tests/librarycallbackauthprobe.c tests/librarycallbacksignaturecapture.c tests/librarycallbackproduce.py tests/fixtures/librarycallback/auth-A.c tests/fixtures/librarycallback/auth-B.c; } | sha256sum | cut -d' ' -f1)
D=$BASE/callback-auth-$K;mkdir -p "$D/tools"
b(){ python3 "$R/tests/bound.py" 55 "$@"; }
case $(uname -m) in x86_64) ASM=exec/c/librarycall_x86_64.S;;aarch64|arm64) ASM=exec/c/librarycall_arm64.S;;*) exit 77;;esac
case ${1:-check} in
--cache-dir) printf '%s\n' "$D";;
--prepare-signaturecapture) b cc -O1 -w -I. tests/librarycallbacksignaturecapture.c "$ASM" -lffi -ldl -o "$D/tools/signature-capture";(cd "$D/tools" && sha256sum signature-capture >signature.sha256);;
--prepare-authprobe) b cc -O1 -w -I. tests/librarycallbackauthprobe.c "$ASM" -lffi -ldl -o "$D/tools/auth-probe";(cd "$D/tools" && sha256sum auth-probe >probe.sha256);;
check)
 (cd "$D/tools" && sha256sum -c --status signature.sha256 && sha256sum -c --status probe.sha256)
 O=$(mktemp -d "${TMPDIR:-/tmp}/callback-auth-check.XXXXXX");trap 'rm -rf "$O"' EXIT HUP INT TERM
 b python3 tests/librarycallbackproduce.py --root "$R" --product "$UA" --out "$O" --part 1 --checker tests/librarycallbackauthcheck.cx --auth "$D/tools/auth-probe" "$D/tools/signature-capture" tests/fixtures/librarycallback/auth-A.c tests/fixtures/librarycallback/auth-B.c;;
*) echo 'usage: --selftest | --cache-dir | --prepare-signaturecapture/authprobe | check' >&2;exit 2;;
esac
