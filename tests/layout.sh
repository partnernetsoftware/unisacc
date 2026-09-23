#!/bin/bash
# The data layout, enumerated rather than sampled. [A-41] [P-3] [S-13]
#
# `unisa acc` discharges the model by enumerating its FULL gold.  This does
# the same for the one hand-ported pure function the compiler's images
# depend on: lower.zero_last, which puts initialised blobs first and zero
# blobs after, each keeping its address mod 8.
#
# tests/gen_layout.py writes every tape of one, two and three data
# definitions over {bss 1, bss 8, str 1, str 7, str 8, str of NULs}, plus
# the same tapes with one symbol defined TWICE -- 762 tapes.  Each is
# compiled by both back ends for all six targets and the bytes must match.
#
# It costs about ten seconds, because a tape goes straight to the back end
# (`unisacc x.tape -b os/arch`) and the Python side stays in one process.
# Two bugs it found on the day it was written: a symbol defined twice was
# allocated twice by the C back end and once by the Python one, and the
# blob starts were not sorted.
set -u
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
. "$R/tests/lib.sh"; ua_ready
T=$(scratch)
n=$(python3 tests/gen_layout.py "$T" "${DEPTH:-3}") || { echo "  FAIL generator"; exit 1; }
[ "$n" -gt 0 ] || { echo "  FAIL the generator wrote no tapes"; exit 1; }
perl -e 'alarm 900; exec @ARGV' python3 tests/layout_check.py "$UA" "$T" \
    lnx/x86_64 lnx/arm64 osx/x86_64 osx/arm64 win/x86_64 win/arm64
