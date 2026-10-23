#!/bin/sh
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
cd "$R"
. ./tests/lib.sh
D=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-package-seed.XXXXXX")   # under the deletion guard's temp root
_SD="$R"/tests   # the one deletion guard: argv CLI tests/safedel.cx over include/unisa_safedel.h (this trap replaces
# tests/lib.sh's, so it removes lib.sh's scratch $_LIB_TMP too)
trap 'sh "$_SD/cxbin" "$_SD/safedel.cx" --purge -- "${D:?}" "${_LIB_TMP:?}"' EXIT HUP INT TERM
if [ "$UA" = /tmp/ua_ref ]; then UA="$D/ref"; fi
ua_ready
sanitize=
case ${PACKAGE_SANITIZE:-0} in 0) ;; 1) sanitize=--sanitize;; *) exit 2;; esac
bound 50 python3 exec/c/packagecheck.py --ua "$UA" $sanitize
