#!/bin/sh
# HOST SHIM ONLY. This is not evidence for the unisacc native call bridge.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
task_tmp=$(mktemp -d "${TMPDIR:-/tmp}/unisacc-ffi-host.XXXXXX")
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
cd "$root"
python3 tests/bound.py 30 cc -DUFFI_HOST_SHIM -Wall -Wextra -Wno-unused-function exec/ffi/probe.c -lffi -o "$task_tmp/probe"
python3 tests/bound.py 10 "$task_tmp/probe" > "$task_tmp/out"
cat "$task_tmp/out"
test "$(grep -c '^PASS ' "$task_tmp/out")" -eq 19
test "$(grep -c '^END failures=0$' "$task_tmp/out")" -eq 1
! grep '^FAIL ' "$task_tmp/out"
