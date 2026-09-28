#!/bin/sh
# One output contract per bounded job; never merge plain/located/token formats.
set -eu
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
case ${1:-plain} in plain) flag=;; located) flag=--locations;; tokens) flag=; E2_AUTOINC=0;export E2_AUTOINC;; *) echo 'usage: sharede2.sh [plain|located|tokens]' >&2;exit 2;; esac
T=$(mktemp -d);trap 'rm -rf "$T"' EXIT
python3 tests/bound.py 55 python3 exec/pp/sharedcheck.py $flag --output "$T"
