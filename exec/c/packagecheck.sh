#!/bin/sh
set -eu
R=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
cd "$R"
. ./tests/lib.sh
D=$(mktemp -d /tmp/unisacc-package-seed.XXXXXX)
trap 'rm -rf "$D"' EXIT HUP INT TERM
if [ "$UA" = /tmp/ua_ref ]; then UA="$D/ref"; fi
ua_ready
sanitize=
case ${PACKAGE_SANITIZE:-0} in 0) ;; 1) sanitize=--sanitize;; *) exit 2;; esac
bound 50 python3 exec/c/packagecheck.py --ua "$UA" $sanitize
