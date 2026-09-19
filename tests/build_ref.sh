#!/bin/sh
# Build unisacc.c with the system compiler, for differential checking.
set -e
cat kernel/unisa_core.c src/unisacc_main.c > unisacc.c
cat tests/refshim.h unisacc.c tests/reffoot.h > "${1:-/tmp/ua_ref.c}"
cc -w -std=c99 -o "${2:-/tmp/ua_ref}" "${1:-/tmp/ua_ref.c}"
