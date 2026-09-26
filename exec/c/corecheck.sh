#!/bin/sh
# The kernel is a separate C translation unit, not an estimate obtained by
# subtracting unrelated executables. No source edits or cached model answers.
set -eu
R=$(cd "$(dirname "$0")/../.." && pwd); cd "$R"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
b() { perl -e 'alarm 60; exec @ARGV' "$@"; }
b env CORE_EXTERNAL=1 python3 exec/c/netcheck.py
if [ "$(uname -s)" = Darwin ]; then
    for arch in arm64 x86_64; do
        b cc -arch "$arch" -Os -Wall -Wextra -c exec/c/core.c -o "$T/$arch.o"
        b size -m "$T/$arch.o"
        b nm -u "$T/$arch.o" > "$T/imports"
        b python3 - "$T/imports" <<'PY'
import pathlib,sys
names=set(pathlib.Path(sys.argv[1]).read_text().split())
allowed={'___stack_chk_fail','___stack_chk_guard','_abort','_calloc','_free',
         '_memcmp','_memcpy','_realloc','_strlen','_core_host_fetch','_core_host_panic'}
assert names<=allowed, ('unexpected core dependency',names-allowed)
assert {'_core_host_fetch','_core_host_panic'}<=names, ('missing host boundary',names)
PY
    done
else
    b cc -Os -Wall -Wextra -c exec/c/core.c -o "$T/core.o"
    b size -A "$T/core.o"
fi
printf '%s\n' 'isolated C core: external linkage checks passed; reported text includes every generic action/storage helper, excludes libc/host'
