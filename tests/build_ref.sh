#!/bin/sh
# Build unisacc.c with the system compiler, for differential checking.
set -e
# Every step writes a private file and renames it into place: suites run
# concurrently and several of them rebuild, so nobody may ever see half a
# file -- the same source gives the same bytes, so the last rename wins
# harmlessly.
# unisacc.c is ONE file -- no include path to depend on -- so the model goes
# in where unisa_core.c's `#include` names it
{ cat kernel/unisa_model.inc
  grep -v '^#include "unisa_' kernel/unisa_core.c
  cat src/unisacc_main.c src/unisacc_back.c; } > unisacc.c.$$
mv -f unisacc.c.$$ unisacc.c
cat tests/refshim.h unisacc.c tests/reffoot.h > "${1:-/tmp/ua_ref.c}.$$"
mv -f "${1:-/tmp/ua_ref.c}.$$" "${1:-/tmp/ua_ref.c}"
cc -w -std=c99 -o "${2:-/tmp/ua_ref}.$$" "${1:-/tmp/ua_ref.c}"
mv -f "${2:-/tmp/ua_ref}.$$" "${2:-/tmp/ua_ref}"
