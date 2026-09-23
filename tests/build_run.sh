#!/bin/sh
# Build unisaccrun.c (the run tool) the way build_ref.sh builds unisacc.c:
# the same kernel, the same front end, the same back end -- one different
# main.  UNISACC_NO_MAIN drops unisacc's own.
set -e
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
{ echo "#define UNISACC_NO_MAIN 1"
  cat kernel/unisa_core.c src/unisacc_main.c src/unisacc_back.c \
      src/unisaccrun_main.c; } > unisaccrun.c.$$
mv -f unisaccrun.c.$$ unisaccrun.c
cat tests/refshim.h unisaccrun.c tests/reffoot.h > "${1:-/tmp/ua_run.c}.$$"
mv -f "${1:-/tmp/ua_run.c}.$$" "${1:-/tmp/ua_run.c}"
cc -w -std=c99 -o "${2:-/tmp/ua_run}.$$" "${1:-/tmp/ua_run.c}"
mv -f "${2:-/tmp/ua_run}.$$" "${2:-/tmp/ua_run}"
