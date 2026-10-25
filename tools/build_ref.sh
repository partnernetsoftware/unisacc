#!/bin/sh
# Build unisacc.c with the system compiler, for differential checking.
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
source=${1:-/tmp/ua_ref.c}
flat="$source.flat.$$"
# deletions go through the guard (tools/safedel.cx; without unisacc.com tools/cxbin builds it with the system cc);
# a refused cleanup (a path outside every whitelisted root) is reported by the guard and does not fail the build
trap 'sh "$R/tools/cxbin" "$R/tools/safedel.cx" --purge -- "${flat:?}" "$flat.source.json" "${source:?}.$$" || :' EXIT HUP INT TERM
"$R/tools/export_ref.sh" "$flat"
cat "$R/tools/refshim.h" "$flat" "$R/tools/reffoot.h" > "$source.$$"
mv -f "$source.$$" "$source"
# Optional third argument: emit the identical wrapped C source without an unused compiler.
[ "${3:-}" != --source-only ] || exit 0
# -O2, and it is not a detail: the reference binary is what every suite
# runs, thousands of times.  Unoptimised it compiles unisacc.c in 3.86s,
# at -O2 in 0.46s, and the whole suite is dominated by it.  The output is
# a compiler's output -- deterministic and independent of how the compiler
# itself was built -- so `closure` staying at 540/540 across the change is
# the evidence that nothing about it depends on the optimiser (and would
# be a loud way to discover latent UB if it ever did).
# P7': link under the final basename in a private dir -- the ad-hoc codesign Identifier is the
# output's basename, so a PID-bearing temp name made same-source UAs differ by ~53 bytes
out="${2:-/tmp/ua_ref}"; tdir=$(mktemp -d "$(dirname "$out")/.ua_ref.XXXXXX") || exit 2
cc -w -std=c99 ${CFLAGS:--O2} -o "$tdir/$(basename "$out")" "${1:-/tmp/ua_ref.c}" || { sh "$R/tools/cxbin" "$R/tools/safedel.cx" --purge -- "${tdir:?}" || :; exit 1; }
mv -f "$tdir/$(basename "$out")" "$out"; sh "$R/tools/cxbin" "$R/tools/safedel.cx" --purge -- "${tdir:?}" || :
