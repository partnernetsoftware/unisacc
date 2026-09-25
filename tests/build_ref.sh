#!/bin/sh
# Build unisacc.c with the system compiler, for differential checking.
set -e
# Every step writes a private file and renames it into place: suites run
# concurrently and several of them rebuild, so nobody may ever see half a
# file -- the same source gives the same bytes, so the last rename wins
# harmlessly.
# unisacc.c is ONE file -- no include path to depend on -- so the model goes
# in where unisa_core.c's `#include` names it
{ cat kernel/unisa_model.inc kernel/unisa_headers.inc
  grep -v '^#include "unisa_' kernel/unisa_core.c
  cat src/front_pp.c src/front_parse.c src/opt.c src/main.c src/back_lower.c src/back_encode.c src/back_image.c; } > unisacc.c.$$
mv -f unisacc.c.$$ unisacc.c
cat tests/refshim.h unisacc.c tests/reffoot.h > "${1:-/tmp/ua_ref.c}.$$"
mv -f "${1:-/tmp/ua_ref.c}.$$" "${1:-/tmp/ua_ref.c}"
# -O2, and it is not a detail: the reference binary is what every suite
# runs, thousands of times.  Unoptimised it compiles unisacc.c in 3.86s,
# at -O2 in 0.46s, and the whole suite is dominated by it.  The output is
# a compiler's output -- deterministic and independent of how the compiler
# itself was built -- so `closure` staying at 540/540 across the change is
# the evidence that nothing about it depends on the optimiser (and would
# be a loud way to discover latent UB if it ever did).
cc -w -std=c99 ${CFLAGS:--O2} -o "${2:-/tmp/ua_ref}.$$" "${1:-/tmp/ua_ref.c}"
mv -f "${2:-/tmp/ua_ref}.$$" "${2:-/tmp/ua_ref}"
