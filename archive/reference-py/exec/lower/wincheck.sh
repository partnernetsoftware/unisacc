#!/bin/sh
# No option keeps the original two-target contract; a leaf selects one target.
set -eu
usage() { echo "usage: $0 [--arch x86_64|arm64] | --interpreter-info | --material-json ARCH OUT | --material-tbl JSON OUT | --material-check ARCH [--part cases|refusals|refusals-1|refusals-2|large] RUN TABLE JSON HELLO FIB" >&2; exit 2; }
MATERIAL=
PART=
case ${1-} in
    --interpreter-info) [ $# = 1 ] || usage; MATERIAL=info;;
    --material-json) [ $# = 3 ] || usage; MATERIAL=json; ARCHES=$2; MATERIAL_OUT=$3;;
    --material-tbl) [ $# = 3 ] || usage; MATERIAL=tbl; MATERIAL_IN=$2; MATERIAL_OUT=$3;;
    --material-check) [ $# = 7 ] || [ $# = 9 ] || usage; MATERIAL=check; ARCHES=$2; shift 2
        if [ "${1-}" = --part ]; then
            [ $# = 7 ] || usage
            case $2 in cases|refusals|refusals-1|refusals-2|large) PART=$2;; *) usage;; esac
            shift 2
        fi
        [ $# = 5 ] || usage;;
esac
if [ "$MATERIAL" = json ] || [ "$MATERIAL" = check ]; then
    case $ARCHES in x86_64|arm64) :;; *) usage;; esac
fi
if [ -z "$MATERIAL" ]; then case $# in
    0) ARCHES='x86_64 arm64';;
    2) [ "$1" = --arch ] || usage
       case $2 in x86_64|arm64) ARCHES=$2;; *) usage;; esac;;
    *) usage;;
esac; fi
_BOUND=$(cd "$(dirname "$0")/../.." && pwd)/tests/bound
_BOUND=$("$_BOUND" --helper) || exit 2
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
# The existing legacy oracle stays visibly owned here; native phase control
# calls these recipes rather than constructing an interpreter argv itself.
legacy_interpreter() { "$_BOUND" 60 python3 "$@"; }
case $MATERIAL in
    info) command -v python3; legacy_interpreter --version; exit;;
    json) case $ARCHES in arm64) legacy_interpreter exec/build/gen.py lower "$MATERIAL_OUT" --full --win --arm64;;
                         x86_64) legacy_interpreter exec/build/gen.py lower "$MATERIAL_OUT" --full --win;; esac; exit;;
    tbl) legacy_interpreter exec/c/tbl.py "$MATERIAL_IN" "$MATERIAL_OUT"; exit;;
    check) export LOWER_TARGET=win/$ARCHES
           if [ -n "$PART" ]; then set -- --part "$PART" "$@"; fi
           legacy_interpreter exec/lower/fullcheck.py "$@"; exit;;
esac
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h
T=$(mktemp -d); trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}"' EXIT
b() { "$_BOUND" 60 "$@"; }
UA=${UA:-/tmp/ua_ref}; . ./tests/lib.sh; ua_ready
# tests/lib.sh replaced the trap above: one trap for both scratch directories, through the one deletion guard
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${T:?}" "${_LIB_TMP:?}"' EXIT
b cc -O2 -o "$T/run" exec/c/run.c
b "$UA" -S examples/hello.c -o "$T/hello"
b "$UA" -O2 -S examples/fib.c -o "$T/fib"
for ARCH in $ARCHES; do
    case $ARCH in arm64) FLAG=--arm64;; *) FLAG=;; esac
    legacy_interpreter exec/build/gen.py lower "$T/d.json" --full --win $FLAG
    legacy_interpreter exec/c/tbl.py "$T/d.json" "$T/d.tbl"
    LOWER_TARGET=win/$ARCH legacy_interpreter exec/lower/fullcheck.py "$T/run" "$T/d.tbl" "$T/d.json" "$T/hello" "$T/fib"
done
