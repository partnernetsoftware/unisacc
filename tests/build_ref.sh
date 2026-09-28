#!/bin/sh
# Build unisacc.c with the system compiler, for differential checking.
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
source=${1:-/tmp/ua_ref.c}
flat="$source.flat.$$"
trap 'rm -f "$flat" "$source.$$"' EXIT HUP INT TERM
"$R/tests/export_ref.sh" "$flat"
cat "$R/tests/refshim.h" "$flat" "$R/tests/reffoot.h" > "$source.$$"
mv -f "$source.$$" "$source"
# -O2, and it is not a detail: the reference binary is what every suite
# runs, thousands of times.  Unoptimised it compiles unisacc.c in 3.86s,
# at -O2 in 0.46s, and the whole suite is dominated by it.  The output is
# a compiler's output -- deterministic and independent of how the compiler
# itself was built -- so `closure` staying at 540/540 across the change is
# the evidence that nothing about it depends on the optimiser (and would
# be a loud way to discover latent UB if it ever did).
cc -w -std=c99 ${CFLAGS:--O2} -o "${2:-/tmp/ua_ref}.$$" "${1:-/tmp/ua_ref.c}"
mv -f "${2:-/tmp/ua_ref}.$$" "${2:-/tmp/ua_ref}"
